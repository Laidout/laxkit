//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2004-2007,2010-2011 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/patchinterface.h>
#include <lax/transformmath.h>
#include <lax/anxapp.h>
#include <lax/laxutils.h>
#include <lax/bezutils.h>
#include <lax/language.h>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 




namespace LaxInterfaces {

//   s-->
//t  0  1  2  3
//|  4  5  6  7
//v  8  9 10 11
//  12 13 14 15
//
// Shortcut selecting:
//  1: 5,6,9,10
//  2: 0 3 12 15
//  3: 1 2 4 8 7 11 13 14
//  4: 1 2 13 14
//  5: 4 8 7 11


//#define PATCH_LINEAR   (1<<0)
//#define PATCH_SMOOTH   (1<<1)


//-----------------------for debugging: showmat ------------------------


/*! ***for debugging, pops up a window showing an xs by ys matrix
 * of flatpoints.
 */
class showmat : public anXWindow
{
	public:
	flatpoint *M;
	int x,y;
	showmat(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder, flatpoint *MM,int xs,int ys);
	~showmat();
	virtual void Refresh();
};
//			new showmat(app,NULL,"Matrix",0,0,0, 600,500, 0, data->points,data->xsize,data->ysize));


showmat::showmat(anXWindow *parnt,const char *ntitle,unsigned long nstyle,
		int xx,int yy,int ww,int hh,int brder, flatpoint *MM,int xs,int ys)
		: anXWindow(parnt,ntitle,ntitle,nstyle,xx,yy,ww,hh,brder, NULL,None,NULL)
{
	M=new flatpoint[xs*ys];
	for (int c=0; c<xs*ys; c++) M[c]=MM[c];
	x=xs;
	y=ys;
	needtodraw=1;
}

showmat::~showmat() { delete[] M; }

void showmat::Refresh()
{
	clear_window(this);
	int r,c;
	char str[100];
	flatpoint f;
	for (r=0; r<y; r++)
		for (c=0; c<x; c++) {
			f=M[r*x+c];
			sprintf(str,"%.2f,%.2f",f.x,f.y);
			textout(this,str,-1,c*100,r*20,LAX_TOP|LAX_LEFT);
//void textout(Drawable drawable,const char *thetext,int len,int x,int y,unsigned long align);
		}
	needtodraw=0;
}

//---------------------------- 4x4 Matrix Utilities -----------------------------------


// P  =  S^t B^t G^t B T
// transformed patch, with v=n*t, u=m*s:
// U = U^t B^t ((B^-1 M B) G^t (B N B^-1)) B V
// 
// u=m*(s-s0), s=u/m + s0, 
// v=n*(t-t0), t=v/n + t0, 
//   
//   V = N T
//
//    [ 1/n^3  3*t0/n^2  3*t0^2/n   t0^3 ] 
//  N=[   0     1/n^2     2*t0/n    t0^2 ], M is like N, but is transposed
//    [   0       0         1/n      t0  ]
//    [   0       0          0        1  ]
//    
// 
//    [ 0  4  8 12 ]
// Gt=[ 1  5  9 13 ]
//    [ 2  6 10 14 ]
//    [ 3  7 11 15 ]
//   
//   [ t^3 ]
// T=[ t^2 ], S,U,V are simliarly for s,u,v
//   [  t  ]
//   [  1  ]


//! The bezier matrix.
/*! \ingroup math
 *  See <a href="lotsamath.html">here</a> for what this is.
 */
double B[16]={	-1,  3, -3,  1,
				 3, -6,  3,  0,
				-3,  3,  0,  0,
				 1,  0,  0,  0
			};

//! The inverse of the bezier matrix.
/*! \ingroup math
 *  See <a href="lotsamath.html">here</a> for what this is.
 */
double Binv[16]={ 0,    0,    0,   1,
				  0,    0,  1./3,  1,
				  0,  1./3, 2./3,  1,
				  1,    1,    1,   1
			};

//! v=[ t^3, t^2, t, 1 ]
/*! \ingroup math
 */
void getT(double *v,double t) 
{ 
	v[3]=1; 
	v[2]=t; 
	v[1]=t*t; 
	v[0]=v[1]*t; 
}

//! Returns a*b, a and b are double[4]..
/*! \ingroup math
 */
double dot(double *a,double *b)
{
	double f=0;
	for (int c=0; c<4; c++) f+=a[c]*b[c];
	return f;
}

//! v = m b (b is the vector), m is 4x4 matrix, b and v are double[4]
/*! \ingroup math
 */
void m_times_v(double *m,double *b,double *v)
{
	int c,r;
	for (r=0; r<4; r++) {
		v[r]=0;
		for (c=0; c<4; c++) v[r]+=m[r*4+c]*b[c];
	}
}

//! m = a x b, a and b are 4x4 matrices
/*! \ingroup math
 */
void m_times_m(double *a,double *b,double *m)
{
	int c,c2,r;
	for (r=0; r<4; r++) 
		for (c=0; c<4; c++) {
			m[r*4 + c]=0;
			for (c2=0; c2<4; c2++) m[r*4 + c]+=a[r*4 + c2] * b[c2*4 + c];
		}
}

//! Fill I with 4x4 Identity matrix
/*! \ingroup math
 */
void getI(double *I)
{
	for (int c=0; c<16; c++) I[c]=0;
	I[0]=I[5]=I[10]=I[15]=1;
}

//! Fill 4x4 I with x,y,z,w scaled by a,b,c,d
/*! \ingroup math
 */
void getScaledI(double *I,double a,double b,double c,double d)
{
	for (int c1=0; c1<16; c1++) I[c1]=0;
	I[0]=a;
	I[5]=b;
	I[10]=c;
	I[15]=d;
}

//! For debugging: cout a 4x4 matrix G[16].
/*! \ingroup math
 */
void printG(const char *ch,double *G)
{
	cout <<endl<<ch<<':'<<endl;
	int r,c;
	for (r=0; r<4; r++) {
		for (c=0; c<4; c++) {
			cout << G[r*4+c]<< "  ";
		}
		cout <<endl;
	}
}

//! Transpose the 4x4 matrix.
/*! \ingroup math
 */
void m_transpose(double *M)
{
	double t;
	int r,c;
	for (r=0; r<4; r++) {
		for (c=0; c<4; c++) {
			if (r<=c) continue;
			//DBG cerr <<"Mrc="<<M[r*4 + c]<<" Mcr="<<M[c*4 + r]<<":";
			t=M[r*4 + c];
			M[r*4 + c]=M[c*4 + r];
			M[c*4 + r]=t;
			//DBG cerr <<"Mrc="<<M[r*4 + c]<<" Mcr="<<M[c*4 + r]<<"  ";
		}
	}
}


//! This makes a polynomial column vector T:
/*! \ingroup math
 *  <pre>
 *    [ t^3 ]
 *  T=[ t^2 ], v=n*(t-t0), t=v/n + t0, 
 *    [  t  ]
 *    [  1  ]
 * 
 *    V = N T
 * 
 *     [ 1/n^3  3*t0/n^2  3*t0^2/n   t0^3 ] 
 *   N=[   0     1/n^2     2*t0/n    t0^2 ]
 *     [   0       0         1/n      t0  ]
 *     [   0       0          0        1  ]
 * </pre>  
 *
 * \todo this could be optimized a little
 */
void getPolyT(double *N, double n, double t0)
{
	N[0]=1./n/n/n;
	N[1]=3*t0/n/n;
	N[2]=3*t0*t0/n;
	N[3]=t0*t0*t0;
	
	N[5]=1./n/n;
	N[6]=2*t0/n;
	N[7]=t0*t0;
	
	N[10]=1./n;
	N[11]=t0;
	
	N[14]=N[13]=N[12]=N[9]=N[8]=N[4]=0;
	N[15]=1;
}

//------------------------------------- PatchRenderContext ------------------------
/*! \class PatchRenderContext
 * \brief Holds threadsafe data concerning computations with PatchData like objects.
 *
 * See ImagePatchData and ColorPatchData particularly.
 */

//! Return the point (S Cx T,S Cy T).
/*! assumes Cx,Cy already set right.
 * 
 * Called from rpatchpoint().
 */
flatpoint PatchRenderContext::getPoint(double *S,double *T)
{
	flatpoint p;
	m_times_v(Cx,T,V); 
	p.x=dot(S,V);
	m_times_v(Cy,T,V);
	p.y=dot(S,V);
	return p;
}

//-------------------------------------- PatchData -----------------------

/*! \class PatchData
 * \ingroup interfaces
 * \brief Plain old ordinary cubic tensor product patches, the base for mesh gradients and some image warping.
 *
 * See PatchInterface.
 */
/*! \var int PatchData::renderdepth
 * \brief How and how much to render in renderToBuffer.
 *
 * If renderdepth>=0, then render recursively to full pixel depth of render buffer.
 * If renderdepth<0, then subdivide each subpatch by -renderdepth number of
 * single color quadrilateral blocks.
 */


//! Creates a patch with points=NULL, size=0.
PatchData::PatchData()
{
	renderdepth=0;
	style=0; 
	controls=Patch_Full_Bezier;
	points=NULL;
	griddivisions=10;
	xsize=ysize=0;
}

//! Creates a new patch in rect xx,yy,ww,hh with nr rows and nc columns.
PatchData::PatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	points=NULL;
	Set(xx,yy,ww,hh,nr,nc,stle);
}

PatchData::~PatchData()
{
	if (points) delete[] points; 
}

SomeData *PatchData::duplicate(SomeData *dup)
{
	PatchData *p=dynamic_cast<PatchData*>(dup);
	if (!p && !dup) return NULL; //was not PatchData!

	char set=1;
	if (!dup && somedatafactory) {
		dup=somedatafactory->newObject(LAX_PATCHDATA,this);
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			set=0;
		}
		p=dynamic_cast<PatchData*>(dup);
	} 
	if (!p) {
		p=new PatchData();
		dup=p;
	}
	if (set) {
		p->renderdepth=renderdepth;
		p->griddivisions=griddivisions;
		if (points) {
			p->points=new flatpoint[xsize*ysize];
			memcpy(p->points,points,xsize*ysize*sizeof(flatpoint));
		}
		p->xsize=xsize;
		p->ysize=ysize;
		p->style=style;
		p->linestyle=linestyle;
		p->controls=controls;
	}

	 //somedata elements:
	dup->bboxstyle=bboxstyle;
	dup->m(m());
	return dup;
}

/*! \ingroup interfaces
 * Dump out a PatchData:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  griddivisions 10
 *  xsize 4
 *  ysize 4
 *  points \\
 *    0   0
 *    1.5 0
 *    ...
 * </pre>
 * 
 * If what==-1, then output a pseudocode mockup of the format. Otherwise
 * output the format as above.
 */
void PatchData::dump_out(FILE *f,int indent,int what,Laxkit::anObject *context)
{
	char spc[indent+3]; memset(spc,' ',indent); spc[indent]='\0'; 
	if (what==-1) {
		fprintf(f,"%smatrix 1 0 0 1 0 0 #the affine matrix affecting the patch\n",spc);
		fprintf(f,"%sgriddivisions 10   #number of grid lines to display\n",spc);
		fprintf(f,"%sxsize 4            #number of points in the x direction\n",spc);
		fprintf(f,"%sysize 4            #number of points in the y direction\n",spc);
		fprintf(f,"%sstyle smooth       #when dragging controls do it so patch is still smooth\n",spc);
		fprintf(f,"%scontrols full      #can also be linear, coons, or border\n",spc);
		fprintf(f,"%spoints \\           #all xsize*ysize points, a list by rows of: x y\n",spc);
		
		fprintf(f,"%s  1.0 1.0\n",spc);
		fprintf(f,"%s  2.0 1.0\n",spc);
		fprintf(f,"%s  1.0 2.0\n",spc);
		fprintf(f,"%s  2.0 2.0\n",spc);
		fprintf(f,"%s  #etc... there are 16 points in the smallest patch\n",spc);

		return;
	}
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,m(0),m(1),m(2),m(3),m(4),m(5));
	fprintf(f,"%sgriddivisions %d\n",spc,griddivisions);
	if (style&PATCH_SMOOTH) fprintf(f,"%sstyle smooth\n",spc);
	
	if (controls==Patch_Linear)           fprintf(f,"%scontrols linear\n",spc);
	else if (controls==Patch_Coons)       fprintf(f,"%scontrols coons\n",spc);
	else if (controls==Patch_Border_Only)  fprintf(f,"%scontrols border\n",spc); 
	else if (controls==Patch_Full_Bezier) fprintf(f,"%scontrols full\n",spc);
	
	fprintf(f,"%sxsize %d\n",spc,xsize);
	fprintf(f,"%sysize %d\n",spc,ysize);
	fprintf(f,"%spoints \\ #%dx%d\n",spc, xsize,ysize);
	for (int c=0; c<xsize*ysize; c++) {
		fprintf(f,"%s  %.10g %.10g",spc,points[c].x,points[c].y);
		if (c%xsize==0) fprintf(f," #row %d\n",c/xsize);
		else fprintf(f,"\n");
	}
}

//! Reverse of the dump_out.
void PatchData::dump_in_atts(Attribute *att,int flag,Laxkit::anObject *context)
{
	if (!att) return;
	char *name,*value;
	int p=-1,c;
	SomeData::dump_in_atts(att,0,context);
	for (c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"griddivisions")) {
			IntAttribute(value,&griddivisions);
		} else if (!strcmp(name,"xsize")) {
			IntAttribute(value,&xsize);
		} else if (!strcmp(name,"ysize")) {
			IntAttribute(value,&ysize);
		} else if (!strcmp(name,"points")) {
			p=c;
		} else if (!strcmp(name,"style")) {
			int s;
			IntAttribute(value,&s);
			style=s;
		} else if (!strcmp(name,"controls")) {
			if (!strcmp(value,"full"))        controls=Patch_Full_Bezier;
			else if (!strcmp(value,"linear")) controls=Patch_Linear;
			else if (!strcmp(value,"coons"))  controls=Patch_Coons;
			else if (!strcmp(value,"border")) controls=Patch_Border_Only;
		}
	}
	 // read in points after all atts initially parsed, so as to retrieve xsize and ysize.
	if (p>-1) {
		double x;
		name=value=att->attributes.e[p]->value;
		if (points) delete[] points;
		points=new flatpoint[xsize*ysize];
		for (c=0; c<xsize*ysize; c++) {
			DoubleAttribute(value,&x,&name);
			if (name!=value) {
				points[c].x=x;
				DoubleAttribute(name,&points[c].y,&value);
			}
		}
	}

	FindBBox();
}

/*! \todo should do actual bounds checking, not bounding box
 */
int PatchData::pointin(flatpoint pp,int pin)
{
	return SomeData::pointin(pp,pin);
}

//! Find bbox.
/*! \todo ***this currently only does bounds for control points
 */
void PatchData::FindBBox()
{
	if (xsize*ysize<=0) return;
	minx=maxx=points[0].x;
	miny=maxy=points[0].y;
	for (int c=0; c<xsize*ysize; c++) {
		if (points[c].x<minx) minx=points[c].x;
		else if (points[c].x>maxx) maxx=points[c].x;
		if (points[c].y<miny) miny=points[c].y;
		else if (points[c].y>maxy) maxy=points[c].y;
	}
}

/*! Copies mesh point data only, not matrix.
 */
void PatchData::CopyMeshPoints(PatchData *patch)
{
	if (patch->xsize*patch->ysize!=xsize*ysize) {
		delete[] points;
		points=new flatpoint[patch->xsize*patch->ysize];
	}
	xsize=patch->xsize;
	ysize=patch->ysize;

	memcpy(points,patch->points, patch->xsize*patch->ysize*sizeof(flatpoint));
}

//! Set in rect xx,yy,ww,hh with nr rows and nc columns. Removes old info.
void PatchData::Set(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	style=stle; 
	xsize=nc*3+1;
	ysize=nr*3+1;
	if (points) delete[] points;
	points=new flatpoint[xsize*ysize];
	zap(flatpoint(xx,yy),flatpoint(ww,0),flatpoint(0,hh));
	griddivisions=10;
}

//! Return the point corresponding to (s,t), where s and t are in range [0..1]. s for column, t for row.
/*! Please note that this is useful only for one time lookup. The matrices involved are
 * not cached for repeated use.
 */
flatpoint PatchData::getPoint(double s,double t)
{
	 // getpoint for s,t, which is:
	 //   S^t * B * Gt * B * T     (remember B==B^t)
	double Gx[16],Gy[16],Tr[16],T[4],S[4],tmp[16],tv[4];
	flatpoint pp;
	double ss,tt;
	int c,r;
	resolveToSubpatch(s,t,c,ss,r,tt);
	//DBG cerr<<" resolve to patch: c,ss:"<<c<<":"<<ss<<"  r,tt:"<<r<<':'<<tt;

	getGt(Gx,r*3,c*3,0);
	getGt(Gy,r*3,c*3,1);

	getT(T,tt);
	getT(S,ss);

	 //x component:
	m_times_m(B,Gx,tmp);
	m_times_m(tmp,B,Tr);
	m_times_v(Tr,T,tv);
	pp.x=dot(S,tv);

	 //y component:
	m_times_m(B,Gy,tmp);
	m_times_m(tmp,B,Tr);
	m_times_v(Tr,T,tv);
	pp.y=dot(S,tv);

	//DBG cerr<<"   found point: "<<pp.x<<','<<pp.y<<endl;
	return pp;
}

//! From a point s,t (range 0..1), return the subpatch r,c plus offset into that subpatch.
/*! If s,t are bad, then the returned values will be bad!
 */
void PatchData::resolveToSubpatch(double s,double t,int &c,double &ss,int &r,double &tt)
{
	int nc=xsize/3, //num of patches horizontally
		nr=ysize/3; //num of patches vertically

	c=(int) (s*nc);
	if (c>=nc) c=nc-1;
	ss=s*nc-c;

	r=(int) (t*nr);
	if (r>=nr) r=nr-1;
	tt=t*nr-r;
}

//! From subpatch (r,c) and offset, return a point s,t with range [0..1],[0..1].
/*! c and r are subpatch indices, not point indices. (Point column)=3*(subpatch column).
 * If c,ss,r,tt don't exist in this patch, then a faulty s,t will be returned!
 */
void PatchData::resolveFromSubpatch(int c,double ss,int r,double tt,double &s,double &t)
{
	s=(float)(c+ss)*3/(xsize-1);
	t=(float)(r+tt)*3/(ysize-1);
}

//! Return points[r*xsize+c].
flatpoint PatchData::getControlPoint(int r,int c)
{
	return points[r*xsize+c];
}

//! Return a bezier curve corresponding to the row (row==1) or column (row==0) i.
/*! This is similar to bezCrossSection(), but only for subpatch edges.
 * 
 *  Returns a list v-c-c-v-c-c-v-...-v. If p==NULL, return a new flatpoint[]. Otherwise, if
 * row==0, p must be at least ysize long, and if row!=0, it must be at least xsize long.
 *
 * i==1,row==0 corresponds to point column 3.
 *
 * If i is out of range, NULL is returned.
 */
flatpoint *PatchData::bezAtEdge(flatpoint *p,int i,int row)
{
	if (row) {
		 //construct horizontal bezier section 
		int r=i*3;
		if (r<0 || r>=ysize) return NULL;
		if (!p) p=new flatpoint[xsize];
		memcpy(p,points+r*xsize,xsize*sizeof(flatpoint));
		return p;
	}
	
	 //construct vertical bezier section 
	int c=i*3;
	if (c<0 || c>=xsize) return NULL;
	if (!p) p=new flatpoint[ysize];
	for (int r=0; r<ysize; r++) p[r]=points[c+r*xsize];
	return p;
}

//! Return the cross section bezier curve at subpatch row (or column) i, and parameter t.
/*! Returns a list v-c-c-v-c-c-v-...-v. If p==NULL, return a new flatpoint[]. Otherwise, if
 * row==0, p must be at least ysize long, and if row!=0, it must be at least xsize long.
 *
 * i==1 corresponds to point column 3.
 *
 * You must not pass an i at a far edge.
 */
flatpoint *PatchData::bezCrossSection(flatpoint *p,int i,double t,int row)
{
	if (row) {
		 //construct horizontal bezier section 
		if (!p) p=new flatpoint[xsize];
		int r=i*3;
		for (int c=1; c<xsize; c+=3) {
			if (c==1) p[c-1]=bez_point(t,points[c-1+r*xsize],points[c-1+(1+r)*xsize],points[c-1+(2+r)*xsize],points[c-1+(3+r)*xsize]);
			p[c  ]=bez_point(t,points[c+  r*xsize],points[c+  (1+r)*xsize],points[c+  (2+r)*xsize],points[c+  (3+r)*xsize]);
			p[c+1]=bez_point(t,points[c+1+r*xsize],points[c+1+(1+r)*xsize],points[c+1+(2+r)*xsize],points[c+1+(3+r)*xsize]);
			p[c+2]=bez_point(t,points[c+2+r*xsize],points[c+2+(1+r)*xsize],points[c+2+(2+r)*xsize],points[c+2+(3+r)*xsize]);
		}
		return p;
	}
	 //construct vertical bezier section 
	if (!p) p=new flatpoint[ysize];
	int c=i*3;
	//flatpoint v1,c1,c2,v2;
	for (int r=1; r<ysize-1; r+=3) {
		if (r==1) p[r-1]=
			   bez_point(t,points[c+(r-1)*xsize],points[c+1+(r-1)*xsize],points[c+2+(r-1)*xsize],points[c+3+(r-1)*xsize]);
		p[r  ]=bez_point(t,points[c+(r  )*xsize],points[c+1+(r  )*xsize],points[c+2+(r  )*xsize],points[c+3+(r  )*xsize]);
		p[r+1]=bez_point(t,points[c+(r+1)*xsize],points[c+1+(r+1)*xsize],points[c+2+(r+1)*xsize],points[c+3+(r+1)*xsize]);
		p[r+2]=bez_point(t,points[c+(r+2)*xsize],points[c+1+(r+2)*xsize],points[c+2+(r+2)*xsize],points[c+3+(r+2)*xsize]);
	}
	return p;
}

//! Remap so the patch corresponds to its rectangular bounding box.
/*! \todo generally for somedatas, perhaps shouldn't really FindBBox() all the time,
 *    but instead do it on a check for modified, so as to not find bbox at every change?
 */
void PatchData::zap()
{
	if (xsize*ysize<=0) return;
	FindBBox();
	zap(flatpoint(minx,miny),flatpoint(maxx,miny)-flatpoint(minx,miny),flatpoint(minx,maxy)-flatpoint(minx,miny));
}

//! Remap to be in parallelogram of p,x,y
/*! Assumes xsize,ysize,points already set and allocated properly, x and y are vectors
 */
void PatchData::zap(flatpoint p,flatpoint x,flatpoint y) 
{
	 // align all existing points to be even spaced in x,y,w,h
	int r,c;
	maxx=minx-1;
	for (r=0; r<ysize; r++)
		for (c=0; c<xsize; c++) {
			points[r*xsize+c]=p + c*x/xsize + r*y/ysize;
			addtobounds(points[r*xsize+c]);
	}
	touchContents();
}

//! Interpolate control points according to whichcontrols.
/*! This will not change the controls that exist for each type,
 * but will change all the others as best as it knows how.
 */
void PatchData::InterpolateControls(int whichcontrols)
{
	if (whichcontrols==Patch_Full_Bezier || xsize==0 || ysize==0) return;
	
	if (whichcontrols==Patch_Linear) {
		 //redo all but the outermost corners
		flatpoint p00,p30,p03,p33;
		p00=points[0];
		p03=points[xsize-1];
		p30=points[(ysize-1)*xsize];
		p33=points[(ysize-1)*xsize+xsize-1];
		int c,r,i;
		double s,t;
		for (r=0; r<ysize; r++) {
			for (c=0; c<xsize; c++) {
				 //if point is a corner point, then skip
				if ((c==0 && r==0)
					  || (c==0 && r==ysize-1)
					  || (c==xsize-1 && r==0 )
					  || (c==xsize-1 && r==ysize-1))
					continue;

				i=r*xsize+c;
				s=(double)c/(xsize-1);
				t=(double)r/(ysize-1);
				points[i]=s*(t*p33+(1-t)*p03) + (1-s)*(t*p30+(1-t)*p00);
			}
		}
	} else if (whichcontrols==Patch_Border_Only) {
		 //redo all interior points
		int c,r,i;
		double s,t;
		flatpoint pt,pb,pl,pr;
		for (r=1; r<ysize-1; r++) {
			for (c=1; c<xsize-1; c++) {
				i=r*xsize+c;
				 
				 //point is weight average of the following 4 points
				pt=points[c];
				pb=points[c+(ysize-1)*xsize];
				pl=points[r*xsize];
				pr=points[xsize-1+r*xsize];

				s=(double)c/(xsize-1);
				t=(double)r/(ysize-1);
				points[i]=((1-s)*pl + s*pr + (1-t)*pt + t*pb)/2;
			}
		}
	} else if (whichcontrols==Patch_Coons) {
		 //redo all 4 interior points per subpatch:
		 //  p11=1./9*(-4*p00+6*(p01+p10)-2*(p03+p30)+3*(p31+p13)-p33)
		 //  p12=1./9*(-4*p03+6*(p02+p13)-2*(p00+p33)+3*(p32+p10)-p30)
		 //  p21=1./9*(-4*p30+6*(p31+p20)-2*(p33+p00)+3*(p01+p23)-p03)
		 //  p22=1./9*(-4*p33+6*(p32+p23)-2*(p30+p03)+3*(p02+p20)-p00)
		int c,r,i;
		flatpoint p00,p30,p03,p33, p11,p12,p21,p22, p01,p02,p31,p32,p10,p13,p20,p23;
		for (r=0; r<ysize-1; r+=3) {
			for (c=0; c<xsize-1; c+=3) {
				i=r*xsize+c; //points p00 corner of subpatch
				
				p00=points[i];
				p01=points[i+1];
				p02=points[i+2];
				p03=points[i+3];
				
				p10=points[i+    xsize];
				p13=points[i+3+  xsize];
				
				p20=points[i+  2*xsize];
				p23=points[i+3+2*xsize];
				
				p30=points[i+  3*xsize];
				p31=points[i+1+3*xsize];
				p32=points[i+2+3*xsize];
				p33=points[i+3+3*xsize];

				p11=(-4*p00+6*(p01+p10)-2*(p03+p30)+3*(p31+p13)-p33)/9; 
				p12=(-4*p03+6*(p02+p13)-2*(p00+p33)+3*(p32+p10)-p30)/9; 
				p21=(-4*p30+6*(p31+p20)-2*(p33+p00)+3*(p01+p23)-p03)/9; 
				p22=(-4*p33+6*(p32+p23)-2*(p30+p03)+3*(p02+p20)-p00)/9; 
				
				points[i+  xsize +1]=p11;
				points[i+  xsize +2]=p12;
				points[i+2*xsize +1]=p21;
				points[i+2*xsize +2]=p22;
				
				//points[i+  xsize +1]=(-4*p00+6*(p01+p10)-2*(p03+p30)+3*(p31+p13)-p33)/9; //p11
				//points[i+  xsize +2]=(-4*p03+6*(p02+p13)-2*(p00+p33)+3*(p32+p10)-p30)/9; //p12
				//points[i+2*xsize +1]=(-4*p30+6*(p31+p20)-2*(p33+p00)+3*(p01+p23)-p03)/9; //p21
				//points[i+2*xsize +2]=(-4*p33+6*(p32+p23)-2*(p30+p03)+3*(p02+p20)-p00)/9; //p22
			}
		}
	}
}

//! Return the bezier outline of a subsection of the patch.
/*! The returned path will enclose a section rl subpatches wide and cl subpatches tall.
 * r, rl, c, and cl are subpatch indices, not indices into points. So the point row of
 * the start of r is 3*r. If rl<1 then use the maximum size from row r. Similarly for cl.
 *
 * If p==NULL, then return the number of points that must be allocated in p.
 * Otherwise, p must have enough points for the requested data.
 *
 * Returns a closed path, with the first vertex at p[1], whose associated control points
 * are p[0] and p[2].
 */
int PatchData::bezOfPatch(flatpoint *p,int r,int rl,int c,int cl)
{
	if (r<0 || r>=ysize/3+1) { r=0; rl=ysize/3+1; }
	if (rl<1) rl=100000;
	if (r+rl>ysize/3+1) rl=ysize/3+1-r;
	
	if (c<0 || c>=xsize/3+1) { c=0; cl=xsize/3+1; }
	if (cl<1) cl=100000;
	if (c+cl>xsize/3+1) cl=xsize/3+1-c;
	
	int n=2*(rl*3+cl*3);
	if (p==NULL) return n;

	 // x:c..c+cl,  y:r..r+rl
	int cc,rr;
	int i=1;
	
	 //bottom horizontal row
	for (cc=c; cc<c+cl; cc++) {
		p[i++]=points[r*3*xsize+cc*3];
		p[i++]=points[r*3*xsize+cc*3+1];
		p[i++]=points[r*3*xsize+cc*3+2];
	}
	
	 //rightmost vertical
	for (rr=r; rr<r+rl; rr++) {
		p[i++]=points[(rr*3  )*xsize+cc*3];
		p[i++]=points[(rr*3+1)*xsize+cc*3];
		p[i++]=points[(rr*3+2)*xsize+cc*3];
	}
	
	 //top horizontal row
	for (cc=c+cl; cc>c; cc--) {
		p[i++]=points[rr*3*xsize+cc*3];
		p[i++]=points[rr*3*xsize+cc*3-1];
		p[i++]=points[rr*3*xsize+cc*3-2];
	}
	
	 //leftmost vertical
	for (rr=r+rl; rr>r; rr--) {
		p[i++]=points[(rr*3  )*xsize+cc*3];
		p[i++]=points[(rr*3-1)*xsize+cc*3];
		if (rr!=r+1) p[i++]=points[(rr*3-2)*xsize+cc*3];
		else p[0]=points[(rr*3-2)*xsize+cc*3];
	}
	
	return n;
}

//! Return which subpatch the point seems to be in.
/*! If t_ret!=NULL and s_ret!=NULL, then also find the s_ret (x direction) and t_ret
 * (y direction) within the subpatch (range [0..1] the point is at, and put
 * those values in t_ret and s_ret.
 *
 * r_ret and c_ret can alsa be NULL, if the specific row and column are not needed.
 *
 * The return value is 0 for point not found to be within the boundary of any subpatch, or nonzero if it is.
 * Note that this only checks against the bezier boundary points, not the actual visual boundary
 * of the patch, which might go past the bez boundary if the inner controls are dragged way out.
 *
 * \todo *** could have inSubPatchBBox, to more easily search for point over a control point
 */
int PatchData::inSubPatch(flatpoint p,int *r_ret,int *c_ret,double *t_ret,double *s_ret,double d)
{
	//DBG cerr <<"inSubPatch p="<<p.x<<','<<p.y<<":"<<endl;
	flatpoint pts[12];
	for (int c=0; c<xsize/3; c++) {
		for (int r=0; r<ysize/3; r++) {
			bezOfPatch(pts,r,1,c,1);
			if (point_is_in_bez(p,pts,4)) {
				if (r_ret) *r_ret=r;
				if (c_ret) *c_ret=c;
				if (t_ret || s_ret) {
					 //find the s,t!
					if (coordsInSubPatch(p,r,c,d,s_ret,t_ret)) {
						 //default find! error finding a point
						if (s_ret) *s_ret=.5;
						if (t_ret) *t_ret=.5;
					}
					//DBG cerr <<" found (s,t)=";
					//DBG if (s_ret && t_ret) cerr <<*s_ret<<','<<*t_ret<<endl; else cerr <<endl;
				}
				return 1;
			}
		}
	}
	if (r_ret) *r_ret=-1;
	if (c_ret) *c_ret=-1;
	
	//DBG cerr <<" no (s,t) found"<<endl;
	return 0;
}

//! Find an approximate (s,t) point to p.
/*! Assumes that it is known that point p is in the subpatch r,c.
 *
 * This works by computing 16 points within the range (s0..s1,t0..t1), and 
 * comparing their distance to p. Using the point with the minimum distance
 * to p, try the same thing with the range (s-ds..s+ds,t-dt..t+dt), where dt and
 * ds are a fraction of the original range.
 *
 * Once a point is found that is less than
 * maxd from p, then return that point.
 *
 * Returns 0 for s_ret and t_ret set, else nonzero.
 */
int PatchData::coordsInSubPatch(flatpoint p,int r,int c,double maxd, double *s_ret,double *t_ret)
{
	int nump=6;
	double s,t, ds,dt, ps=0,pt=0;
	double s0=0,s1=1,t0=0,t1=1;
	int ss,tt;
	double d,oldmind=0,mind=1e+10,dist;
	d=maxd*maxd;
	flatpoint pp;
	
	double Gx[16],Gy[16],Tr[16],T[4],S[4],tmp[16],tv[4];
	getGt(Gx,r*3,c*3,0);
	getGt(Gy,r*3,c*3,1);

	int recurse=0;
	while (recurse<20 && oldmind!=mind) {
		recurse++;
		oldmind=mind;
		s=s0;
		ds=(s1-s0)/(nump-1);
		dt=(t1-t0)/(nump-1);
		for (ss=0; ss<nump; s+=ds,ss++) {
			t=t0;
			for (tt=0; tt<nump; t+=dt,tt++) {
				 // getpoint for s,t, which is:
				 //   S^t * B * Gt * B * T     (remember B==B^t)
				getT(T,t);
				getT(S,s);
				 //x component:
				m_times_m(B,Gx,tmp);
				m_times_m(tmp,B,Tr);
				m_times_v(Tr,T,tv);
				pp.x=dot(S,tv);
				 //y component:
				m_times_m(B,Gy,tmp);
				m_times_m(tmp,B,Tr);
				m_times_v(Tr,T,tv);
				pp.y=dot(S,tv);
	
				dist=(pp-p)*(pp-p);
				//DBG cerr <<" ----point:"<<pp.x<<','<<pp.y<<"  dist:"<<dist<<endl;
				if (dist<d) {
					*s_ret=s;
					*t_ret=t;
					//DBG cerr <<"---return coords: "<<*s_ret<<","<<*t_ret<<endl;
					return 0;
				}
				if (dist<mind) {
					mind=dist;
					ps=s; 
					pt=t;
					//DBG cerr <<"---coords: d:"<<d<<"  dist:"<<dist<<"  s,t="<<s<<","<<t<<endl;
				}
			}
		}
		if (mind==oldmind) {
			 //widen search
			s0-=ds; if (s0<0) s0=0;
			s1+=ds; if (s1>1) s1=1;
			t0-=dt; if (t0<0) t0=0;
			t1+=dt; if (t1>1) t1=1;
			oldmind++;
			nump++;
		}
		if (ps-ds>s0) s0=ps-ds;
		if (ps+ds<s1) s1=ps+ds;
		if (pt-dt>t0) t0=pt-dt;
		if (pt-dt<t1) t1=pt+dt;
		
	}
	*s_ret=ps;
	*t_ret=pt;
	return 1;
}

//! Grow the patch off an edge.
/*! If where==0, add a column to the left.
 *  If where==1, add a row to the top.
 *  If where==2, add a column to the right.
 *  If where==3, add a row to the bottom.
 *
 * The new edge is the tr(oldedge), and the intervening controls are interpolated.
 * tr is a 6 member affine transform.
 */
void PatchData::grow(int where, double *tr)
{
	if (where==0) {
		 //add to the left
		flatpoint v;
		flatpoint *np=new flatpoint[(xsize+3)*ysize];
		int nxs=xsize+3;
		for (int r=0; r<ysize; r++) {
			 // interpolate controls between old edge and new points
			memcpy(np+r*nxs+3,points+r*xsize,xsize*sizeof(flatpoint));
			np[  r*nxs]=transform_point(tr,points[r*xsize]);
			v=np[r*nxs]-points[r*xsize];
			np[2+r*nxs]=points[r*xsize]+v/3;
			np[1+r*nxs]=points[r*xsize]+v*2/3;
		}
		delete[] points;
		points=np;
		xsize+=3;
		touchContents();
		FindBBox();

	} else if (where==1) { 
		 //add to the top
		flatpoint v;
		flatpoint *np=new flatpoint[xsize*(ysize+3)];
		memcpy(np,points,xsize*ysize*sizeof(flatpoint));
		for (int i=xsize*(ysize-1); i<ysize*xsize; i++) {
			 // interpolate controls between old edge and new points
			np[i+3*xsize]=transform_point(tr,points[i]);
			v=np[i+3*xsize]-points[i];
			np[i+  xsize]=points[i]+v/3;
			np[i+2*xsize]=points[i]+v*2/3;
		}
		delete[] points;
		points=np;
		ysize+=3;
		touchContents();
		FindBBox();

	} else if (where==2) {
		 //add to the right
		flatpoint v;
		flatpoint *np=new flatpoint[(xsize+3)*ysize];
		int nxs=xsize+3;
		for (int r=0; r<ysize; r++) {
			 // interpolate controls between old edge and new points
			memcpy(np+r*nxs,points+r*xsize,xsize*sizeof(flatpoint));
			np[(r+1)*nxs-1]=transform_point(tr,points[(r+1)*xsize-1]);
			v=np[(r+1)*nxs-1]-points[(r+1)*xsize-1];
			np[(r+1)*nxs-3]=points[(r+1)*xsize-1]+v/3;
			np[(r+1)*nxs-2]=points[(r+1)*xsize-1]+v*2/3;
		}
		delete[] points;
		points=np;
		xsize+=3;
		touchContents();
		FindBBox();

	} else {
		 //add to the bottom
		flatpoint v;
		flatpoint *np=new flatpoint[xsize*(ysize+3)];
		memcpy(np+3*xsize,points,xsize*ysize*sizeof(flatpoint));
		for (int i=0; i<xsize; i++) {
			 // interpolate controls between old edge and new points
			np[i        ]=transform_point(tr,points[i]);
			v=np[i]-points[i];
			np[i+2*xsize]=points[i]+v/3;
			np[i+  xsize]=points[i]+v*2/3;
		}
		delete[] points;
		points=np;
		ysize+=3;
		FindBBox();
		touchContents();
	}
}

//! Merge (delete) rows, and/or columns.
/*! r and c are subpatch indices, not point indices. So to merge the first column
 * of patches and the next column of patches, then pass c=1. 
 *
 * If c==1 then the first column is removed. Same goes for when c is the last column. 
 * Similar for rows. 
 *
 * If c<0, then no columns are deleted. Similarly for when r<0.
 * 
 * If r>=0 and c>=0, then columns are merged first, then rows.
 */
void PatchData::collapse(int rr,int cc)
{
	int r,c;
	cc*=3;
	rr*=3;
	 
	 //collapse column
	if (cc>=0 && xsize>4 && cc<xsize) {
		int nxs,nys;
		nxs=xsize-3;
		nys=ysize;
		flatpoint *np=new flatpoint[nxs*nys];
		if (cc==0) {
			for (r=0; r<ysize; r++) {
				memcpy(np+r*nxs,points+3+r*xsize,nxs*sizeof(flatpoint));
			}
		} else if (cc==xsize-1) {
			for (r=0; r<ysize; r++) {
				memcpy(np+r*nxs,points+r*xsize,nxs*sizeof(flatpoint));
			}
		} else {
			for (r=0; r<ysize; r++) {
				for (c=0; c<cc-2; c++) {
					np[c+r*nxs]=points[c+r*xsize];
				}
				 //guess at new middle controls
				np[c+r*nxs]=2*points[cc-2+r*xsize]-points[cc-3+r*xsize];
				c++;
				np[c+r*nxs]=2*points[cc+2+r*xsize]-points[cc+3+r*xsize];
				
				for (c=cc+3; c<xsize; c++) {
					np[c-3+r*nxs]=points[c+r*xsize];
				}
			}
		}
		delete[] points;
		points=np;
		xsize=nxs;
		ysize=nys;
		
		//if (cc>0 && xsize>4 && cc<xsize-1) {

	}
	
	 //collapse row
	if (rr>=0 && ysize>4 && rr<ysize) {
		int nxs,nys;
		nxs=xsize;
		nys=ysize-3;
		flatpoint *np=new flatpoint[nxs*nys];
		if (rr==0) {
			memcpy(np,points+3*xsize,xsize*nys*sizeof(flatpoint));
		} else if (rr==ysize-1) {
			memcpy(np,points,xsize*nys*sizeof(flatpoint));
		} else {
			for (c=0; c<xsize; c++) {
				 //copy below
				for (r=0; r<rr-2; r++) {
					np[c+r*nxs]=points[c+r*xsize];
				}
				 //guess at new middle controls
				np[c+r*nxs]=2*points[c+(rr-2)*xsize]-points[c+(rr-3)*xsize];
				r++;
				np[c+r*nxs]=2*points[c+(rr+2)*xsize]-points[c+(rr+3)*xsize];
				 //copy above
				for (r=rr+3; r<ysize; r++) {
					np[c+(r-3)*nxs]=points[c+r*xsize];
				}
			}
		}
		delete[] points;
		points=np;
		xsize=nxs;
		ysize=nys;
	}
}

//! Subdivide a single row number r, position rt, and a single column c, position ct.
/*! If r<0, then do not subdivide on a row. Same for when c<0. if r or c are
 * to large, then do not subdivide.
 *
 * r and c are border numbers, so r*3 is index int points. rt and ct are in
 * the range [0..1], with rt==0 being row r, and rt==1 being row r+1.
 *
 * Return 0 for success, nonzero for failure (nothing changed).
 *
 * \todo *** clean me up! simplify me! there is probably an efficient way to extract bits of
 *   this and the other subdivide() to shorten the code...
 */
int PatchData::subdivide(int r,double rt,int c,double ct) 
{
	if (r<0 && c<0) return 1;
	
	int nxs,nys;
	r*=3;
	c*=3;
	if (r>=ysize-1) r=-1;
	if (c>=xsize-1) c=-1;
	if (r<0 && c<0) return 1;
	
	nxs=xsize+(c>=0?3:0);
	nys=ysize+(r>=0?3:0);

	 //reallocate the points array.. the new array gets shifted around below
	int rr,r2,r3, cc,c2,c3, i;
	flatpoint *np=new flatpoint[nxs*nys];
	for (rr=0; rr<ysize; rr++) memcpy(np+rr*nxs, points+rr*xsize, xsize*sizeof(flatpoint));

	double newGx[16],newGy[16], oldGx[16],oldGy[16], oldM[16],Mt[16], oldN[16],N[16], C[16];
	if (r>=0) {
		 //subdivide a row
		if (r<ysize-4) {
			 //make room for new row by shifting the unaffected rows up
			//DBG cerr <<endl<<" --- r:"<<r<<", move "<<xsize*(ysize-r-3)<<" flatpoints from "<<(r+3)*xsize<<" to "<<(r+6)*xsize<<endl;
			//memmove(np+(r+6)*xsize,np+(r+3)*xsize,xsize*(ysize-r-3)*sizeof(flatpoint));
			memmove(np+(r+6)*nxs, np+(r+3)*nxs, nxs*(ysize-r-3)*sizeof(flatpoint));
		}
		for (cc=0; cc<xsize-1; cc+=3) {
			 //for each subpatch along the row
			getGt(oldGx,r,cc,0); //note: grabs from points, not np
			getGt(oldGy,r,cc,1);
			 
			 // divide that patch into 1*2 patches
			 // r2,c2 count over the new subpatches (from 0) in patch r,c
			for (r2=0; r2<2; r2++) {
				c2=0;
				
				 //	v=n*(t-t0), t=v/n + t0,  u=m*(s-s0), s=u/m + s0, 
					
				 // mult  B * N * Binv
				//getPolyT(oldN, n, t0);
				getPolyT(oldN, (r2==0?1/rt:(1/(1-rt))), (r2==0?0:rt));
				//DBG printG("should be N:",oldN);
				
				m_times_m(B,oldN,C);
				m_times_m(C,Binv,N);
		
				 // find newG = (Binv*Mt*B) * oldG * (B*N*Binv),  Mt==I
				m_times_m(oldGx,N,newGx); //*** newGx = C*N == oldGx*N
				m_times_m(oldGy,N,newGy); //*** newGy = C*N == oldGx*N
				
				 //write out the new points, found from newG
				for (r3=0; r3<4; r3++) {
					for (c3=0; c3<4; c3++) {
						i=(r + r2*3 + r3)*nxs + (cc + c3);
						np[i].x=newGx[c3*4+r3];
						np[i].y=newGy[c3*4+r3];
					}
				}
			}
		}
	}
	if (c>=0) {
		if (r>=0) {
			 //reassign points before doing columns only if rows were changed!
			 //  please not this is really kind of sloppy:
			ysize=nys;
			delete[] points;
			points=new flatpoint[xsize*ysize];
			for (rr=0; rr<ysize; rr++) memcpy(points+rr*xsize, np+rr*nxs, xsize*sizeof(flatpoint));
		}
		 //subdivide a column
		if (c<xsize-4) {
			 //make room for new row by shifting the unaffected columns to the right
			for (rr=0; rr<nys; rr++) {
				//DBG cerr <<endl<<" --- c:"<<c<<", move "<<(xsize-c-3) <<" flatpoints from "<<rr*xsize+c+3 <<" to "<<rr*xsize+c+6<<endl;
				memmove(np+rr*nxs+c+6, np+rr*nxs+c+3, (xsize-c-3)*sizeof(flatpoint));
			}
		}
		for (rr=0; rr<nys-1; rr+=3) {
			 //for each subpatch along the row
			getGt(oldGx,rr,c,0); //note: grabs from points, not np
			getGt(oldGy,rr,c,1); //  depends on xsize and ysize being accurate for points
			 
			 // divide that patch into 2 patches
			 // r2,c2 count over the new subpatches (from 0) in patch r,c
			for (c2=0; c2<2; c2++) {
				
				 // find newG = (Binv*Mt*B) * oldG * (B*N*Binv),  N==I
				 //	v=n*(t-t0), t=v/n + t0,  u=m*(s-s0), s=u/m + s0, 
				 
				 // mult  Binv * Mt * B
				//getPolyT(result, n, t0);
				getPolyT(oldM, (c2==0?1/ct:(1/(1-ct))), (c2==0?0:ct));
				//DBG printG("should be M:",oldM);
				
				m_transpose(oldM);
				//DBG printG("should be Mt:",oldM);
				
				m_times_m(Binv,oldM,C); //C = Binv*oldM 
				m_times_m(C,B,Mt);     //Mt = Binv*oldM*B 

				 //find newG
				m_times_m(Mt,oldGx,newGx); // newGx = Mt*oldGx*N == Mt*oldGx
				m_times_m(Mt,oldGy,newGy); // newGy = Mt*oldGy*N == Mt*oldGy
				
				 //write out the new points, found from newG
				for (r3=0; r3<4; r3++) {
					for (c3=0; c3<4; c3++) {
						i=(rr + r3)*nxs + (c + c2*3 + c3);
						np[i].x=newGx[c3*4+r3];
						np[i].y=newGy[c3*4+r3];
					}
				}
			}
		}
	}

	xsize=nxs;
	ysize=nys;
	delete[] points;
	points=np;
	return 0;
}
	
//! Break each patch into subpatches.
/*! Break down into xn*yn subsubpatches.
 * Say xn=2 and yn=3, then each subpatch gets split into 2 columns and 3 rows.
 * 
 * <pre>
 *  each subpatch is geared for values of s,t from 0 to 1.
 *  
 *  transformed patch, with T=N*V, S=M*U:
 *   U = Ut Bt ((Bt^-1 Mt Bt) Gt (B N B^-1)) B V
 * </pre>
 *
 * Return 0 for success, nonzero for failure (nothing changed).
 */
int PatchData::subdivide(int xn,int yn) //xn,yn=2
{
	if (xn<1 || yn<1 || (xn==1 && yn==1)) return 1;
	int nxs,nys;
	nxs=(xsize-1)*xn+1;
	nys=(ysize-1)*yn+1;
	flatpoint *np=new flatpoint[nxs*nys];

	//DBG cerr <<"-+++- Subdivide xn:"<<xn<<" yn:"<<yn<<"  nxs"<<nxs<<"  nys"<<nys<<endl;
	
	double newGx[16],newGy[16], oldGx[16],oldGy[16], oldM[16],Mt[16], oldN[16],N[16], C[16];
	double xnd=xn,ynd=yn;
	int r,r2,r3, c,c2,c3;
	
	 // for each original patch
	for (r=0; r<ysize-1; r+=3) {
		for (c=0; c<xsize-1; c+=3) { // r,c point to up left corner (s=t=0) of the patch
			getGt(oldGx,r,c,0);
			getGt(oldGy,r,c,1);
			 
			 // divide that patch into xn*yn patches
			 // r2,c2 count over the new subpatches (from 0) in patch r,c
			for (r2=0; r2<yn; r2++) {
				for (c2=0; c2<xn; c2++) {
					
					 //	v=n*(t-t0), t=v/n + t0,  u=m*(s-s0), s=u/m + s0, 
					 // mult  Binv * Mt * B
					getPolyT(oldM, xnd, ((double)c2)/xnd);
					//DBG printG("should be M:",oldM);
					
					m_transpose(oldM);
					//DBG printG("should be Mt:",oldM);
					
					m_times_m(Binv,oldM,C);
					m_times_m(C,B,Mt);
						
					 // mult  B * N * Binv
					getPolyT(oldN, ynd, r2/ynd);
					//DBG printG("should be N:",oldN);
					
					m_times_m(B,oldN,C);
					m_times_m(C,Binv,N);
			
					 // find newG = (Binv*Mt*B) * oldG * (B*N*Binv)
					m_times_m(Mt,oldGx,C);
					m_times_m(C,N,newGx);
					
					m_times_m(Mt,oldGy,C);
					m_times_m(C,N,newGy);
					
					 //write out the new points, found from newG
					for (r3=0; r3<4; r3++) {
						//DBG cerr <<"writing row:"<<(r*yn + r2*3 + r3) <<":  ";
						for (c3=0; c3<4; c3++) {
							//DBG cerr <<(c*xn + c2*3 + c3)<<"(index="<<(r*yn + r2*3 + r3)*nxs + (c*xn + c2*3 + c3) <<")  ";
							np[(r*yn + r2*3 + r3)*nxs + (c*xn + c2*3 + c3)].x=newGx[c3*4+r3];
							np[(r*yn + r2*3 + r3)*nxs + (c*xn + c2*3 + c3)].y=newGy[c3*4+r3];
						}
						//DBG cerr <<endl;
					}
				}
			}
		}
	}
	delete[] points;
	points=np;
	xsize=nxs;
	ysize=nys;
	return 0;
}

//! See subdivide() for what Gt is. 
/*! This Gt refers only to the one 4x4 coordinate section starting at (roffset,coffset).
 *
 * roffset and coffset are point indices, not subpatch indices.
 */
void PatchData::getGt(double *Gt,int roffset,int coffset,int isfory) 
{
	//DBG cerr <<endl;
	int r,c;
	if (!isfory) {
		for (r=0; r<4; r++)
			for (c=0; c<4; c++) 
				Gt[r*4+c]=points[(c+roffset)*xsize+(r+coffset)].x;
	} else {
		for (r=0; r<4; r++)
			for (c=0; c<4; c++) 
				Gt[r*4+c]=points[(c+roffset)*xsize+(r+coffset)].y;
	}
	//DBG if (isfory)  cerr<<"Gy:"<<endl; else cerr<<"Gx:"<<endl;
	//DBG for (r=0; r<4; r++) {
	//DBG 	for (c=0; c<4; c++)  cerr << Gt[r*4+c]<< "  ";
	//DBG 	cerr <<endl;
	//DBG }
}

//! Make data wrap inside a ring between radius 1 and 2, start angle s, end angle e.
/*! Y will be mapped to the radii, and x the rings.
 *
 * Return 0 for success, or non-negative for not enough points to warp. 
 */
int PatchData::warpPatch(flatpoint center, double r1,double r2, double s,double e)
{
	if (xsize==0 || ysize==0) return 1;
	setIdentity();
	int numverts=xsize/3+1;
	double theta=(e-s)/(numverts-1);
	flatpoint vt;

	//DBG cerr << "WarpPatch: max i="<<xsize*ysize<<endl;
	
	double rr,v,a;
	int r,c,i;
	for (r=0; r<ysize; r++) {
		rr=r1+(r2-r1)*r/(ysize-1);
		 // for angle theta between vertices, bezier control rods must be length v:
		v=4*rr*(2*sin(theta/2)-sin(theta))/3/(1-cos(theta));
		
		for (c=0; c<xsize; c+=3) {
			i=r*xsize+c;
			//DBG cerr << i<<"  ";
			
			a=s+c/3*theta;
		 	 // do vertex points
			points[i]=center+flatpoint(rr*cos(a), rr*sin(a));

			 //compute control points that approximate a circle
			vt=transpose(points[i]-center);
			vt=vt*(v/norm(vt));
			 // do previous control point
			if (c>0) {
				//DBG cerr <<"-"<< i-1<<"  ";
				points[i-1]=points[i]-vt;
			}
			 // do next control point
			if (c<xsize-1) {
				//DBG cerr <<"+"<< i+1<<"  ";
				points[i+1]=points[i]+vt;
			}
		}
		//DBG cerr <<endl;
	} 
	//DBG cerr << endl;
	FindBBox();
	return 0;
}

//! This is called from renderToBuffer, before rendering to see if there is anything to render.
/*! This is useful, for instance, in an ImagePatchdata, which will not want to render
 * if there is no loaded image.
 *
 * Subclasses will redefine this. Default here is to return 0.
 */
int PatchData::hasColorData()
{ return 0; }

//! Put the color at (s,t) into color_ret.
/*! Return 0 for color returned, or nonzero for not returned, as when the patch
 * has no associated color data.
 */
int PatchData::WhatColor(double s,double t,ScreenColor *color_ret)
{ return 1; }

//! Write to buffer assuming samples are 8 bit ARGB.
/*! Blanks out the buffer, then renders the patch into it, if any.
 * 
 * Currently, buffer is width x height pixels, and must be 8bit ARGB.
 *
 * Subclasses need not redefine this function, rpatchpoint(), or patchpoint(). They need only
 * redefine WhatColor().
 *
 * \todo *** this is EXTREMELY innefficient
 */
int PatchData::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{
	if (!buffer) return 1;

	//DBG cerr <<"...Render "<<whattype()<<" to buffer, w,h:"<<bufw<<','<<bufh<<" rdepth="<<renderdepth<<endl;

	if (bufdepth!=8 || bufchannels!=4) {
		cerr <<" *** must implement ImagePatchData::renderToBuffer() for non 8 bit rgba!!"<<endl;
		return 1;
	}
	if (bufstride==0) bufstride=bufw*bufdepth/8*bufchannels;
	memset(buffer,0,bufw*bufh*4); //make it totally transparent

	if (!hasColorData()) return 0; //blank out the buffer, but don't try to render if there is no color data!!
	
	PatchRenderContext context;

	context.buffer=buffer;
	context.bufferwidth=bufw;
	context.bufferheight=bufh;
	context.stride=bufstride;
	

	int r,c,roff,coff;
	flatpoint fp;
	double C[16],Gty[16],Gtx[16];
	DoubleBBox bufbox(0,bufw,0,bufh), 
			   bbox;

	 //create transform taking object space to buffer space
	double a=(maxx-minx)/bufw,
		   d=(miny-maxy)/bufh;
	double m[6]; //takes points from i to buffer
	m[0]=1/a;
	m[1]=0;
	m[2]=0;
	m[3]=1/d;
	m[4]=-minx/a;
	m[5]=-maxy/d;
	
	 //for each patch, create the proper matrices, then call rpatchpoint()
	for (roff=0; roff<ysize/3; roff++) {
		for (coff=0; coff<xsize/3; coff++) {
			getGt(Gtx,roff*3,coff*3,0);
			getGt(Gty,roff*3,coff*3,1);
			bbox.clear();
			for (r=0; r<4; r++) {
				for (c=0; c<4; c++) {
					fp=flatpoint(Gtx[c*4+r],Gty[c*4+r]);//fp is in object space, not buffer space
					fp=transform_point(m,fp); //transform to buffer space
					Gtx[c*4+r]=fp.x;
					Gty[c*4+r]=fp.y;
					bbox.addtobounds(fp);
				}
			}
			//***is this check necessary?? this version of render assumes whole patch on buffer....
			if (!bufbox.intersect(&bbox,0)) continue;
			
			m_times_m(B,Gty,C);
			m_times_m(C,B,context.Cy);
			m_times_m(B,Gtx,C);
			m_times_m(C,B,context.Cx);  //Cx = B Gtx B
			
			context.s0=coff*3./(xsize-1);
			context.ds=3./(xsize-1);
			context.t0=roff*3./(ysize-1);
			context.dt=3./(ysize-1);
			//DBG cerr <<" draw patch s:"<<context.s0<<','<<context.ds<<"  t:"<<context.t0<<','<<context.dt<<endl;
				
			//DBG d=0;
			//DBG cerr <<(renderdepth==0?"rpatchpoint:":"patchpoint:")<<endl;

			
			//if (renderdepth>=0) rpatchpoint(&context,flatpoint(),flatpoint(),flatpoint(),flatpoint(), 0.,0.,1.,1.,15);
			//else if (renderdepth<0) patchpoint(&context, context.s0,context.ds,context.t0,context.dt, -renderdepth);
			rpatchpoint(&context,flatpoint(),flatpoint(),flatpoint(),flatpoint(), 0.,0.,1.,1.,15);
		}
	}
	//DBG cerr <<"...done rendering to buffer"<<endl;
	return 0;
}

#define UL  1
#define UR  2
#define LL  4
#define LR  8
#define cUL 16
#define cUR 32
#define cLL 64
#define cLR 128

//DBG static int d=0;

//! Recursive render. Used by renderToBuffer().
/*! Use Cx and Cy to find points: P = St C T.
 * which is which point needs to be computed and colored still.
 * 
 * Also assumes s0,ds,t0, and dt set, which refer to the image section corresponding to
 * the current subpatch. s1,t1, s2,t2 are between 0 and 1 for the current subpatch.
 *
 * \todo *** this needs some serious optimizing. it is currently in the HEINOUS HACK stage right now..
 */
void PatchData::rpatchpoint(PatchRenderContext *context,
									flatpoint ul,flatpoint ur,flatpoint ll,flatpoint lr,
									double s1,double t1, double s2,double t2,int which)
{
	//DBG d++;
	//DBG cerr <<"rpatchpoint d="<<d<<"  which="<<which<<"   s1,t1:"<<s1<<','<<t1<<"  "<<endl;
	//DBG if (d>150) { cerr <<"RECURSION YARG!!"<<d<<endl;  exit(0); }
	
	flatpoint um,mm,lm,ml,mr;
	static double T1[4],S1[4], T2[4],S2[4], Tm[4],Sm[4];
	
	getT(S1,s1);
	getT(T1,t1);
	getT(S2,s2);
	getT(T2,t2);
	
	ScreenColor color;
	int i,r,c;
	
	 //first paint the corner points into the buffer

	 // computes (S Cx T,S Cy T), is already in buffer coords
	if (which&UL) {
		ul=context->getPoint(S1,T1);
		if (ul.x>=0 && ul.x<context->bufferwidth && ul.y>=0 && ul.y<context->bufferheight) {
			//app->colorrgb(ipdata->WhatColor(s0+ds*s1,t0+dt*t1),&r,&g,&b);
			r=int(ul.y+.5);
			c=int(ul.x+.5);
			if (c<0) c=0; else if (c>=context->bufferwidth) c=context->bufferwidth-1;
			if (r<0) r=0; else if (r>=context->bufferheight) r=context->bufferheight-1;
			i=r*context->stride + c*4;
			//i=r*4*context->bufferwidth + c*4;

			WhatColor(context->s0+context->ds*s1, context->t0+context->dt*t1, &color);
			context->buffer[i+3]=(color.alpha&0xff00)>>8;
			context->buffer[i+2]=(color.red&0xff00)  >>8;
			context->buffer[i+1]=(color.green&0xff00)>>8;
			context->buffer[i+0]=(color.blue&0xff00) >>8;
			//-----------------
			//col=WhatColor(context->s0+context->ds*s1, context->t0+context->dt*t1);
			//a=(col&0xff000000)>>24;
			//context->buffer[i+3]=a;
			//context->buffer[i+2]=(col&0xff0000)>>16;
			//context->buffer[i+1]=(col&0xff00)>>8;
			//context->buffer[i+0]=(col&0xff);
			//-----------------

			//DBG fprintf(stderr,"\n --%d:%3d,%3d  %x%x%x%x ",i,c,r, buffer[ i ],buffer[i+1],buffer[i+2],buffer[i+3]);
		}
	}
	if (which&UR) {
		ur=context->getPoint(S2,T1);
		if (ur.x>=0 && ur.x<context->bufferwidth && ur.y>=0 && ur.y<context->bufferheight) {
			//app->colorrgb(ipdata->WhatColor(s0+ds*s2,t0+dt*t1),&r,&g,&b);
			r=int(ur.y+.5);
			c=int(ur.x+.5);
			if (c<0) c=0; else if (c>=context->bufferwidth) c=context->bufferwidth-1;
			if (r<0) r=0; else if (r>=context->bufferheight) r=context->bufferheight-1;
			i=r*context->stride + c*4;

			WhatColor(context->s0+context->ds*s2, context->t0+context->dt*t1,&color);
			context->buffer[i+3]=(color.alpha&0xff00)>>8;
			context->buffer[i+2]=(color.red&0xff00)  >>8;
			context->buffer[i+1]=(color.green&0xff00)>>8;
			context->buffer[i+0]=(color.blue&0xff00) >>8;
			//-----------------
			//col=WhatColor(context->s0+context->ds*s2, context->t0+context->dt*t1);
			//a=(col&0xff000000)>>24;
			//context->buffer[i+3]=a;
			//context->buffer[i+2]=(col&0xff0000)>>16;
			//context->buffer[i+1]=(col&0xff00)>>8;
			//context->buffer[i+0]=(col&0xff);
			//-----------------

			//DBG fprintf(stderr,"\n --%d:%3d,%3d  %x%x%x%x ",i,c,r, buffer[ i ],buffer[i+1],buffer[i+2],buffer[i+3]);
		}
	}
	if (which&LL) {
		ll=context->getPoint(S1,T2);
		if (ll.x>=0 && ll.x<context->bufferwidth && ll.y>=0 && ll.y<context->bufferheight) {
			//app->colorrgb(ipdata->WhatColor(s0+ds*s1,t0+dt*t2),&r,&g,&b);
			r=int(ll.y+.5);
			c=int(ll.x+.5);
			if (c<0) c=0; else if (c>=context->bufferwidth) c=context->bufferwidth-1;
			if (r<0) r=0; else if (r>=context->bufferheight) r=context->bufferheight-1;
			i=r*context->stride + c*4;

			WhatColor(context->s0+context->ds*s1, context->t0+context->dt*t2, &color);
			context->buffer[i+3]=(color.alpha&0xff00)>>8;
			context->buffer[i+2]=(color.red&0xff00)  >>8;
			context->buffer[i+1]=(color.green&0xff00)>>8;
			context->buffer[i+0]=(color.blue&0xff00) >>8;
			//col=WhatColor(context->s0+context->ds*s1, context->t0+context->dt*t2);
			//DBG fprintf(stderr," -- %lx ",col);
			//a=(col&0xff000000)>>24;
			//context->buffer[i+3]=a;
			//context->buffer[i+2]=(col&0xff0000)>>16;
			//context->buffer[i+1]=(col&0xff00)>>8;
			//context->buffer[i+0]=(col&0xff);

			//DBG fprintf(stderr,"\n --%d:%3d,%3d  %x%x%x%x ",i,c,r, buffer[ i ],buffer[i+1],buffer[i+2],buffer[i+3]);
		}
	}
	if (which&LR) {
		lr=context->getPoint(S2,T2);
		if (lr.x>=0 && lr.x<context->bufferwidth && lr.y>=0 && lr.y<context->bufferheight) {
			//app->colorrgb(ipdata->WhatColor(s0+ds*s2,t0+dt*t2),&r,&g,&b);
			r=int(lr.y+.5);
			c=int(lr.x+.5);
			 //if out of buffer, should it not simply not draw, rather than clamp?? ***
			if (c<0) c=0; else if (c>=context->bufferwidth) c=context->bufferwidth-1;
			if (r<0) r=0; else if (r>=context->bufferheight) r=context->bufferheight-1;
			i=r*context->stride + c*4;

			WhatColor(context->s0+context->ds*s2, context->t0+context->dt*t2, &color);
			context->buffer[i+3]=(color.alpha&0xff00)>>8;
			context->buffer[i+2]=(color.red&0xff00)  >>8;
			context->buffer[i+1]=(color.green&0xff00)>>8;
			context->buffer[i+0]=(color.blue&0xff00) >>8;
			//col=WhatColor(context->s0+context->ds*s2, context->t0+context->dt*t2);
			//DBG fprintf(stderr," -- %lx ",col);
			//a=(col&0xff000000)>>24;
			//context->buffer[i+3]=a;
			//context->buffer[i+2]=(col&0xff0000)>>16;
			//context->buffer[i+1]=(col&0xff00)>>8;
			//context->buffer[i+0]=(col&0xff);

			//DBG fprintf(stderr,"\n --%d:%3d,%3d  %x%x%x%x ",i,c,r, buffer[ i ],buffer[i+1],buffer[i+2],buffer[i+3]);
		}
	}
	//DBG cerr<<endl;
	
	//DBG cerr <<"ul:"<<ul.x<<", "<<ul.y<<"  "<<
	//DBG 	"ur:"<<ur.x<<", "<<ur.y<<	"  "<<
	//DBG 	"ll:"<<ll.x<<", "<<ll.y<<	"  "<<
	//DBG 	"lr:"<<lr.x<<", "<<lr.y<<endl;	
	//DBG cerr <<"int "<<int(ul.x+.5)<<", "<<int(ul.y+.5)<<"  "<<
	//DBG 	int(ur.x+.5)<<", "<<int(ur.y+.5)<<	"  "<<
	//DBG 	int(ll.x+.5)<<", "<<int(ll.y+.5)<<	"  "<<
	//DBG 	int(lr.x+.5)<<", "<<int(lr.y+.5)<<endl;	

	//DBG cerr <<"  fabs(ul.x-ur.x):" << fabs(ul.x-ur.x) << "  fabs(ll.x-lr.x):" << fabs(ll.x-lr.x) 
	//DBG 		<< "  fabs(ul.y-ll.y):" << fabs(ul.y-ll.y) << "  fabs(ur.y-lr.y):" << fabs(ur.y-lr.y) << endl;
		
//	 // if same pixels:
//	if (int(ul.x+.5)==int(ur.x+.5) && int(ul.x+.5)==int(ll.x+.5) && int(ur.x+.5)==int(lr.x+.5) &&
//	    int(ul.y+.5)==int(ur.y+.5) && int(ul.y+.5)==int(ll.y+.5) && int(ur.y+.5)==int(lr.y+.5)) {
//		
//		//DBG cerr <<"return\n";
//		//DBG d--;
//		return;
//	}
	
	 // if we are at adjacent pixels, then stop recursing..
	//DBG double ul_ur=fabs(ul.x-ur.x);
	//DBG double ll_lr=fabs(ll.x-lr.x);
	//DBG double ul_ll=fabs(ul.x-ll.x);
	//DBG double ur_lr=fabs(ur.x-lr.x);
	//DBG cerr <<"dist ul_ur:"<<ul_ur<<"  ll_lr:"<<ll_lr<<"  ul_ll:"<<ul_ll<<"  ur_lr:"<<ur_lr<<endl;
	if (fabs(ul.x-ur.x)<.8 && fabs(ll.x-lr.x)<.8 && fabs(ul.x-ll.x)<.8 && fabs(ur.x-lr.x)<.8 &&
	    fabs(ul.y-ur.y)<.8 && fabs(ll.y-lr.y)<.8 && fabs(ul.y-ll.y)<.8 && fabs(ur.y-lr.y)<.8) {
		//DBG cerr <<"return from "<<d<<endl;
		//DBG d--;
		return;
	}

	 //figure out mid points along edges and at middle of current patch area
	 //and recursively call this function to draw those sub points
	getT(Sm,(s1+s2)/2);
	getT(Tm,(t1+t2)/2);
	um=context->getPoint(Sm,T1);
	mm=context->getPoint(Sm,Tm);
	lm=context->getPoint(Sm,T2);
	ml=context->getPoint(S1,Tm);
	mr=context->getPoint(S2,Tm);

	rpatchpoint(context, ul,um,ml,mm,    s1    ,    t1,     (s1+s2)/2,(t1+t2)/2,  UR|LL|LR);
	rpatchpoint(context, um,ur,mm,mr, (s1+s2)/2,    t1,         s2   ,(t1+t2)/2,  LR);
	rpatchpoint(context, ml,mm,ll,lm,    s1    ,(t1+t2)/2,  (s1+s2)/2,    t2,     LR);
	rpatchpoint(context, mm,mr,lm,lr, (s1+s2)/2,(t1+t2)/2,      s2,       t2,     0);

	//DBG cerr <<"return after subdivide, d="<<d<<endl;
	//DBG d--;
}

/*! \todo implement me! */
void renderTriangleToBuffer(unsigned char *buf, int bw,int bh,
							flatpoint p1,flatpoint p2,flatpoint p3,
							ScreenColor *col)
{
	cerr <<"Need to implement renderTriangleToBuffer() for patch rendering!!"<<endl; 
}

//! Called from renderToBuffer(). No recursion, just draw a bunch of rects for patch.
/*! The parameters refer to the color space. They are not coordinates. 
 *
 * n is the number of areas to divide the patch into. s0, t0, ds, dt all refer
 * to the color space.
 *
 * Finds coords via context->getPoint().
 *
 * \todo *** optimize this baby! If going beyond resolution of what is in the color source, should be able to
 *   back up and draw larger rect...
 */
void PatchData::patchpoint(PatchRenderContext *context,double s0,double ds,double t0,double dt,int n)
{
	cerr <<"**** WARNING! ImagePatchData::patchpoint() is not implemented properly!!!"<<endl;
	//----------------------

	int r,c;
	int i;
	flatpoint c1,c2,c3,c4;
	double T[4],S[4],s,t, // s and t vary [0..1] and are for patch 
		   ss,tt;        // ss and tt refer to the image space
	
//	if (ds*iwidth<n) { //if patch area takes up less than n pixels of color data...
//		n=int(ds*iwidth);
//		if (n==0) n=1;
//	}
//	if (dt*iheight<n) {
//		n=int(dt*iheight);
//		if (n==0) n=1;
//	}
	ds/=n;
	dt/=n;
	
	getT(T,0);
	double d=1.0/n;
	flatpoint pp[(n+1)*2];

	ScreenColor col[2][n+1];
	//unsigned long col[2][n+1];
	int a=1;
	for (c=0,ss=s0; c<=n; c++,ss+=ds) WhatColor(ss,0, &col[0][c]);
		
	WhatColor(0,0, &col[a][0]);

	//ImlibPolygon polygon;
	for (r=0,t=0,tt=t0; r<=n; r++,t+=d,tt+=dt) {
		getT(T,t);
		for (c=0,s=0,ss=s0; c<=n; c++,s+=d,ss+=ds) {
			//col[a][c+1]=WhatColor(ss+ds,tt+dt);
			WhatColor(ss+ds,tt+dt,&col[a][c+1]);


			getT(S,s);               //****tom in 2009 looks at this work from 2006, and says, WTF??
			if (r%2==0) 
				if (c%2==0) i=2*c;    // row even, column even
				else i=2*c+1;         // row even, column odd
			else if (c%2==0) i=2*c+1; // row odd,  column even
				else i=2*c;           // row odd,  column odd

			pp[i]=context->getPoint(S,T); // computes (S Cx T,S Cy T), is already in screen coords
			
			if (r>0 && c>0) {
				//XSetForeground(app->dpy,app->gc(),col[a][c]);
				//XFillPolygon(app->dpy,dp->GetWindow(),dp->GetGC(),pp+(c-1)*2,4,Convex,CoordModeOrigin);
				//-------
				renderTriangleToBuffer(context->buffer, context->bufferwidth, context->bufferheight,
									   pp[i],pp[i+1],pp[i+2],
									   &col[a][c]);
				renderTriangleToBuffer(context->buffer, context->bufferwidth, context->bufferheight,
									   pp[i+3],pp[i+1],pp[i+2],
									   &col[a][c]);
//				imlib_context_set_color((col[a][c]&0xff0000)  >>16,
//										(col[a][c]&0xff00)    >>8,
//										(col[a][c]&0xff),
//										(col[a][c]&0xff000000)>>24
//									   );
//				polygon=imlib_polygon_new();
//				imlib_polygon_add_point(polygon,  pp[i  ].x, pp[i  ].y);
//				imlib_polygon_add_point(polygon,  pp[i+1].x, pp[i+1].y);
//				imlib_polygon_add_point(polygon,  pp[i+2].x, pp[i+2].y);
//				imlib_polygon_add_point(polygon,  pp[i+3].x, pp[i+3].y);
//				imlib_image_fill_polygon(polygon);
//				imlib_polygon_free(polygon);
			}
		}
		a^=1;
		WhatColor(0,tt+dt, &col[a][0]);
	}
}
//----------------------------------- PatchInterface ------------------------------------------


/*! \class PatchInterface
 * \ingroup interfaces
 * \brief Class to deal with PatchData objects.
 *
 * This class allows manipulation of cubic tensor bezier patches.
 * A patch here starts as a square, and it can be
 * subdivided, and the points grouped and moved around easily.
 * That is, easily enough as tensor product patches go, anyway.
 *
 * \todo *** add warping controls, such as control how to warp to circle, 3x3 matrix warp.
 *    maybe add some kind of generic TransformInterface to allow skewing,flipping of selected points
 * \todo *** This does not implement the free form patch setup found in Postscript.
 *   that functionality might go in a separate class, as the needs are a little too
 *   different perhaps.
 */
/*! \var int PatchInterface::showdecs
 *
 * Show points, grid, and/or edges.
 */
/*! \var int PatchInterface::overstate
 * \brief Whether hovering over an edge is for adding (0) or cutting (1).
 *
 * See overh and overv.
 */
/*! \var int PatchInterface::dragmode
 * \brief What to do on a button down-drag-up..
 *
 * If dragmode==1, then the mouse was clicked down on an edge, and moving the mouse will show
 * where a new edge will appear, and will result in a new row or column being added to the patch.
 *
 * If dragmode==2, then the mouse was clicked on an edge, but the intention is to move the existing
 * edge without creating a new column or row.
 */
/*! \var int PatchInterface::rendermode
 * \brief How to draw the overall patch.
 * 
 * In PatchInterface, you can only draw grid lines, but in subclasses like ColorPatchInterface
 * and ImagePatchInterface, you can draw grid lines only, or the color data instead.
 *
 * rendermode==0 means draw only the basic grid lines. 
 * rendermode==1 means use data->preview for the patch, and draw the grid if no preview available. 
 * rendermode==2 means use drawpatches(), which usually would call drawpatch() once for each subpatch in a patch. This would
 *  be used when you don't want to render previews, but instead draw the patch over each time.
 * rendermode==3 means don't actually draw anything but control stuff.
 */

#define All_Controls     0
#define Border_Controls  1
#define Outline_Controls 2


#define DRAG_ADD_EDGE   1
#define DRAG_SHIFT_EDGE 2


PatchInterface::PatchInterface(int nid,Displayer *ndp) : anInterface(nid,ndp)
{
	linestyle.Color(0xffff,0,0,0xffff);
	controlcolor=rgbcolor(200,200,200);
	rimcolor=rgbcolor(200,100,0);
	handlecolor=rgbcolor(100,100,0);
	gridcolor=rgbcolor(255,0,0);
	oldshowdecs=showdecs=SHOW_Points|SHOW_Edges;
	style=0;
	xs=1;
	ys=1;
	rdiv=2;
	cdiv=2;

	data=NULL;
	poc=NULL;
	
	overv=overh=overch=overcv=-1;
	overstate=-1;
	cuth=cutv=NULL;
	movepts=NULL;
	dragmode=0;
	hoverpoint=-1;
	rendermode=1;
	recurse=0;
	
	constrain=0;
	bx=by=0;
	whichcontrols=Patch_Full_Bezier;
	
	needtodraw=1;

	sc=NULL;
}

PatchInterface::~PatchInterface() 
{ 
	if (cutv) delete[] cutv;
	if (cuth) delete[] cuth;
	
	//DBG cerr<<"-----"<<whattype()<<","<<" destructor"<<endl;
	deletedata();

	if (sc) sc->dec_count();
}

//! Return duplicate of this.
anInterface *PatchInterface::duplicate(anInterface *dup)//dup=NULL
{
	if (dup==NULL) dup=new PatchInterface(id,NULL);
	else if (!dynamic_cast<PatchInterface *>(dup)) return NULL;
	PatchInterface *dupp=dynamic_cast<PatchInterface *>(dup);
	dupp->linestyle=linestyle;
	dupp->controlcolor=controlcolor;
	dupp->rimcolor=rimcolor;
	dupp->handlecolor=handlecolor;
	dupp->gridcolor=gridcolor;
	dupp->xs=xs;
	dupp->ys=ys;
	return anInterface::duplicate(dup);
}

const char *PatchInterface::Name()
{ return _("Patch Tool"); }

int PatchInterface::InterfaceOn()
{//*** deal with app better in interfaces
	showdecs=oldshowdecs;
	needtodraw=1;
	return 0;
}

//! Flush curpoints.
int PatchInterface::InterfaceOff()
{ //*** do decs with xor always?
	Clear();
	oldshowdecs=showdecs;
	showdecs=0;
	needtodraw=1;
	curpoints.flush();
	return 0;
}

//! Return a new local PatchData.
/*! Usually, subclasses would redefine this, which gets called from LBDown().
 * Returns object with count of 1.
 */
PatchData *PatchInterface::newPatchData(double xx,double yy,double ww,double hh,int nr,int nc,unsigned int stle)
{
	PatchData *ndata=NULL;
	if (somedatafactory) {
		ndata=dynamic_cast<PatchData *>(somedatafactory->newObject(LAX_PATCHDATA));
		if (ndata) ndata->Set(xx,yy,ww,hh,nr,nc,stle);
	} 
	if (!ndata) ndata=new PatchData(xx,yy,ww,hh,nr,nc,stle);//creates 1 count
	ndata->FindBBox();
	return ndata;
}

//! Delete data, and flush curpoints.
void PatchInterface::deletedata()
{
	if (data) data->dec_count();
	data=NULL;
	if (poc) { delete poc; poc=NULL; }
	curpoints.flush();
}

//! Delete data, and flush curpoints. Make needtodraw=1.
/*! *** this might not be necessary, default Clear() calls deletedata() which flushes... 
 */
void PatchInterface::Clear(SomeData *d)
{
	//if (d && d!=somedata) return;
	deletedata();
	curpoints.flush();
	hoverpoint=-1;
	overh=overv=overch=overcv=-1;
	needtodraw=1;
}

/*! Takes magic number: id==2 is xs, and id==3 is ys.
 */
int PatchInterface::UseThis(int id,int ndata)
{
	char blah[100];
	// id==1 usually is the linewidth
	if (id==2) {
		xs=ndata;
		if (xs<1) xs=1;
		sprintf(blah,"New Patch %d,%d: ",xs,ys);
		app->postmessage(blah);
		return 1;
	} else if (id==3) {
		ys=ndata;
		if (ys<1) ys=1;
		sprintf(blah,"New Patch %d,%d: ",xs,ys);
		app->postmessage(blah);
		return 1;
	}
	return 0;
}

int PatchInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	PatchData *ndata=dynamic_cast<PatchData *>(oc->obj);
	if (!ndata) return 0;

	if (data && data!=ndata) deletedata();
	if (poc) delete poc;
	poc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}

	curpoints.flush();
	needtodraw=1;
	return 1;
}

/*! Takes a PatchData, or LineStyle.
 */
int PatchInterface::UseThis(anObject *nobj,unsigned int mask) // assumes not use local
{
    if (!nobj) return 0;

	if (dynamic_cast<LineStyle *>(nobj)) {
		//DBG cerr <<"PatchInterface: new linestyle"<<endl;
		LineStyle *nlinestyle=dynamic_cast<LineStyle *>(nobj);
		if (mask&GCForeground) {
			if (data) data->linestyle.color=nlinestyle->color;
			else linestyle.color=nlinestyle->color;
		}
		if (mask&GCLineWidth) {
			if (data) data->linestyle.width=nlinestyle->width; 
			else linestyle.width=nlinestyle->width;
		}
		needtodraw=1;
		return 1;

//	} else if (dynamic_cast<PatchData *>(nobj)) { 
//	    deletedata();
//	    PatchData *ndata=dynamic_cast<PatchData *>(nobj);
//		if (viewport) viewport->NewData(ndata);
//		data=ndata;
//		data->inc_count();
//		curpoints.flush();
//		needtodraw=1;
//	    return 1;
	}

	return 0;
}


////! Get a point and tangents at point (s,t).
//void PatchInterface::get_p_ds_dt(double s,double t,flatpoint &p,flatpoint &ds,flatpoint &dt)
//{
//	double psx=1./(xsize/3),
//		   psy=1./(ysize/3); // keep the /3 as integers
//	int xoff=(int)(s/psx),
//		yoff=(int)(t/psy);
//	s=s-xoff*psx;
//	t=t-yoff*psy;
//	double C[16],Cx[16],Cy[16],Gx[16],Gy[16],S[4],T[4],v[4];
//	getGt(Gx,yoff*3,xoff*3,0);
//	getGt(Gy,yoff*3,xoff*3,1);
//	m_times_m(B,Gy,C);
//	m_times_m(C,B,Cy);
//	m_times_m(B,Gx,C);
//	m_times_m(C,B,Cx);
//	S[3]=1;
//	S[2]=s;
//	S[1]=s*s;
//	S[0]=s*s*s;
//	T[3]=1;
//	T[2]=t;
//	T[1]=t*t;
//	T[0]=t*t*t;
//	m_times_v(Cx,T,v);
//	p.x=dot(S,v);
//	m_times_v(Cy,T,v);
//	p.y=dot(S,v);
//
//	T[3]=0;
//	T[2]=1;
//	T[1]=2*t;
//	T[0]=3*t*t;
//	m_times_v(Cx,T,v);
//	dt.x=dot(S,v);
//	m_times_v(Cy,T,v);
//	dt.y=dot(S,v);
//	
//	S[3]=0;
//	S[2]=1;
//	S[1]=2*s;
//	S[0]=3*s*s;
//	T[3]=1;
//	T[2]=t;
//	T[1]=t*t;
//	T[0]=t*t*t;
//	m_times_v(Cx,T,v);
//	ds.x=dot(S,v);
//	m_times_v(Cy,T,v);
//	ds.y=dot(S,v);
//}

int PatchInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info) // info=0
{
	if (!ndata || dynamic_cast<PatchData *>(ndata)==NULL) return 1;
	PatchData *bzd=data;
	data=dynamic_cast<PatchData *>(ndata);
	int td=showdecs,ntd=needtodraw;
	showdecs=0;
	needtodraw=1;
	
	Refresh();

	needtodraw=ntd;
	showdecs=td;
	data=bzd;
	return 1;
}

//! Draw a grid over patch, lines every 1/griddivisions for s and t directions.
/*! roff,coff is which patch, point start is == roff*3
 */
void PatchInterface::drawpatch(int roff,int coff)
{
	//DBG cerr <<" - - - PatchInterface::drawpatch()"<<endl;

	dp->LineAttributes(linestyle.width,LineSolid,linestyle.capstyle,linestyle.joinstyle);
	dp->NewFG(&data->linestyle.color);
	dp->DrawReal();

	//DBG cerr <<"Draw Patch: roff:"<<roff<<"  coff:"<<coff<<endl;
	double s,t;
	double C[16],Cx[16],Cy[16],Gty[16],Gtx[16],S[4],T[4],v[4];
	S[3]=T[3]=1;
	data->getGt(Gtx,roff*3,coff*3,0);
	data->getGt(Gty,roff*3,coff*3,1);
	m_times_m(B,Gty,C);
	m_times_m(C,B,Cy);
	m_times_m(B,Gtx,C);
	m_times_m(C,B,Cx);
//	DBG printG("Cx",Cx);
//	DBG printG("Cy",Cy);

	flatpoint p[51];
	flatpoint fp;
	double div=1./data->griddivisions;
	for (s=0; s<=1; s+=div) {
		S[3]=1;
		S[2]=s;
		S[1]=s*s;
		S[0]=s*s*s;
		for (int c=0; c<=50; c++) {
			t=c*1.0/50.0;
			T[3]=1;
			T[2]=t;
			T[1]=t*t;
			T[0]=t*t*t;

			m_times_v(Cx,T,v);
			fp.x=dot(S,v);
			m_times_v(Cy,T,v);
			fp.y=dot(S,v);
			p[c]=fp;
		}
		dp->drawlines(p,51,0,0);
	}
	for (t=0; t<=1; t+=div) {
		T[3]=1;
		T[2]=t;
		T[1]=t*t;
		T[0]=t*t*t;
		for (int c=0; c<=50; c++) {
			s=c*1.0/50.0;
			S[3]=1;
			S[2]=s;
			S[1]=s*s;
			S[0]=s*s*s;

			m_times_v(Cx,T,v);
			fp.x=dot(S,v);
			m_times_v(Cy,T,v);
			fp.y=dot(S,v);
			p[c]=fp;
		}
		dp->drawlines(p,51,0,0);
	}
}

//! Draw the whole patch.
/*! This calls drawpatch() for each subpatch. Called from Refresh().
 */
void PatchInterface::drawpatches()
{
	//DBG cerr <<" - - - PatchInterface::drawpatches()"<<endl;
	int r,c;
	for (r=0; r<data->ysize/3; r++) {
		for (c=0; c<data->xsize/3; c++) {
			drawpatch(r,c);
		}
	}
}

/*! Draw in grid form. Calls drawpatch(r,c) for each subpatch.
 * Derived classes need not redo Refresh. They can just redo drawpatch().
 *
 * \todo *** maybe should break off drawing of various decorations to other functions,
 *   so for instance, the colorpatch can draw controls as little color bubbles, which is
 *   much more useful when in "outline" mode, if that ever gets implemented
 */
int PatchInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	if (!data) {
		if (needtodraw) needtodraw=0;
		return 1;
	}

	//data->renderdepth=-recurse;
	//DBG cerr <<"  PatchRefresh- rendermode="<<rendermode<<", depth="<<data->renderdepth<<endl;

				
	 // draw patches
	int d=1;
	if (rendermode==3) d=0;

	if (rendermode==1) {
		LaxImage *preview=data->GetPreview();
		if (preview) {
			d=dp->imageout(preview,data->minx,data->miny, data->maxx-data->minx, data->maxy-data->miny);
			//DBG if (d<0) cerr <<"- - - Patch do not use preview"<<endl; else cerr <<"- - - Patch using preview"<<endl;
			if (d<0) d=1; else d=0; //draw lines if problem with image
		}
	}

	if (d) {
		//if (rendermode==2 && data->renderdepth==0) ...
		drawpatches();
	}

	 // draw control points;
	if (showdecs) { 
		flatpoint p;

		 // draw patch borders
		if (showdecs&SHOW_Edges) {
			dp->NewFG(rimcolor);
			dp->LineAttributes(1,LineSolid,linestyle.capstyle,linestyle.joinstyle);

			flatpoint bez[(data->xsize>data->ysize?data->xsize:data->ysize)+1];
			for (int c=0; c<data->xsize/3+1; c++) {
				data->bezAtEdge(bez+1,c,0);
				
				//DBG cerr <<"---- drawedge: "<<c<<endl;
				//DBG for (int cc=0; cc<data->ysize; cc++)
				//DBG    cerr <<" "<<bez[cc].x<<','<<bez[cc].y;
				//DBG cerr <<endl;

				dp->drawbez(bez,data->ysize/3+1,0,0);
			}
			for (int r=0; r<data->ysize/3+1; r++) {
				data->bezAtEdge(bez+1,r,1);
				dp->drawbez(bez,data->xsize/3+1,0,0);
			}
		}

		//DBG cerr <<"dragmode="<<dragmode<<endl;
		//DBG cerr <<"in refresh: overv:"<<overv<<"  overh:"<<overh<<endl;
		
		 // draw hover indicator
		if (overv>=0) {
			//DBG cerr <<"hovering over a vertical..."<<endl;
			dp->LineAttributes(5,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			//int o=0;
			//flatpoint p[data->ysize+1 + (dragmode==DRAG_ADD_EDGE?6:0)];
			flatpoint p[data->ysize+1];
			//flatpoint lb=dp->screentoreal(mx,my);

//			if (dragmode==DRAG_ADD_EDGE) {
//			**** draw connecting lines to transformed edge and old edge
//				if (overv==0) {
//					 //**** please note that this extra stuff adds curvy connectors to
//					 //     extended patch, but it's rather badly written... down below
//					 //     just draws straight lines, it's simple, and does the job..
//					flatpoint v=lb-lbdown, p0,p1;
//					o=3;
//					p[1]=data->points[0];
//					p[2]=(4*data->points[0]-data->points[1])/3;
//					p[3]=p[1]+v;
//
//					p[data->ysize+4]=p[data->ysize+6]=data->points[(data->ysize-1)*data->xsize];
//					p0=data->points[(data->ysize-1)*data->xsize],
//					p1=data->points[1+(data->ysize-1)*data->xsize];
//					//p[data->ysize+5]=p[data->ysize+4];
//					//p[data->ysize+5]=(4*p0-p1)/3;
//					p[data->ysize+5]=p0+(v||(-p0+p1))/3;
//					p[data->ysize+4]+=v;
//				} else {
//					o=3;
//					p[1]=data->points[data->xsize-1];
//					p[2]=(4*data->points[data->xsize-1]-data->points[data->xsize-2])/3;
//					p[3]=p[1]+(lb-lbdown);
//
//					int Y=data->ysize, X=data->xsize;
//					p[4+Y]=p[6+Y]=data->points[Y*X-1];
//					flatpoint p0=data->points[Y*X-1],
//							  p1=data->points[Y*X-2];
//					p[Y+5]=p[Y+4];
//					//p[Y+5]=(4*p0-p1)/3;
//					//p[Y+5]=p0+((lb-lbdown)||(p0-p1))/3;
//					p[Y+4]+=(lb-lbdown);
//				}
//			}
			for (int r=0; r<data->ysize; r++) {
				if (dragmode==DRAG_ADD_EDGE || dragmode==DRAG_SHIFT_EDGE) 
					p[r+1]=transform_point(movetransform,data->points[overv*3 + r*data->xsize]);
				else p[r+1]=data->points[overv*3 + r*data->xsize];
				//p[o+r+1]=data->points[overv*3 + r*data->xsize]+(dragmode==DRAG_ADD_EDGE?(lb-lbdown):flatpoint(0,0));
			}
			if (overstate==0) dp->NewFG(controlcolor);
			else dp->NewFG(rgbcolor(255,0,0));
			//dp->drawbez(p,(data->ysize/3+1+(dragmode==DRAG_ADD_EDGE?2:0)), 0,0);
			dp->drawbez(p,(data->ysize/3+1),0,0);
		}
		if (overh>=0) {
			dp->LineAttributes(5,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			flatpoint p[data->xsize+1 + (dragmode==DRAG_ADD_EDGE?6:0)];
			//flatpoint lb=dp->screentoreal(mx,my);
			for (int c=0; c<data->xsize; c++) {
				//p[c+1]=data->points[c + 3*overh*data->xsize]+(dragmode==DRAG_ADD_EDGE?(lb-lbdown):flatpoint(0,0));
				if (dragmode==DRAG_ADD_EDGE || dragmode==DRAG_SHIFT_EDGE) 
					p[c+1]=transform_point(movetransform,data->points[c + 3*overh*data->xsize]);
				else p[c+1]=data->points[c + 3*overh*data->xsize];
			}
			if (overstate==0) dp->NewFG(controlcolor);
			else dp->NewFG(rgbcolor(255,0,0));
			dp->drawbez(p, data->xsize/3+1, 0,0);

//			if (dragmode==DRAG_ADD_EDGE) {
//				//***wrong now:
//				dp->drawrline(p[1],p[1]-(lb-lbdown));
//				dp->drawrline(p[data->xsize],p[data->xsize]-(lb-lbdown));
//			}
		}
		 //draw potential subdividing marks
		if (overch>=0) {
			dp->LineAttributes(3,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			dp->NewFG(0,255,0);
			dp->drawbez(cuth,data->xsize/3+1,0,0);
		}
		if (overcv>=0) {
			dp->LineAttributes(3,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			dp->NewFG(0,255,0);
			dp->drawbez(cutv,data->ysize/3+1,0,0);
		}
		
		if (dragmode==DRAG_SHIFT_EDGE) {
			dp->LineAttributes(2,LineSolid,linestyle.capstyle,linestyle.joinstyle);
			dp->NewFG(100,100,255);
			dp->drawline(lbdown,dp->screentoreal(mx,my));
		}

		
		dp->LineAttributes(0,LineSolid,linestyle.capstyle,linestyle.joinstyle);
	
		 // control points
		if (showdecs&SHOW_Points) {
			 // draw patch handle bars
			dp->NewFG(handlecolor);
			int r,c;
			for (r=0; r<data->ysize; r++) {
				if (r%3==1) continue;
				for (c=0; c<data->xsize; c++) {
					if (c%3==1) continue;
					if (c<data->xsize-1 && r%3==0)  // draw horiz handles
						dp->drawline(data->points[r*data->xsize+c],  data->points[r*data->xsize+c+1]);
						//*** for drawing other control lines as dashes:
						//	dp->LineAttributes(0,LineDoubleDash,linestyle.capstyle,linestyle.joinstyle);
					
					if (c%3==0 && r%3!=1 && r<data->ysize-1) // draw vert handles
						dp->drawline(data->points[r*data->xsize+c], data->points[(r+1)*data->xsize+c]);
				}
			}
		
			 // the actual points
			drawControlPoints();

			 // draw hoverpoint
			drawControlPoint(hoverpoint);
			
		}

	}
	//DBG cerr <<endl;
	needtodraw=0;
	return 0;
}

//! Draw a single point. This is for the hoverpoint, and is called from Refresh().
void PatchInterface::drawControlPoint(int i)
{
	if (!data || i<0 || i>=data->xsize*data->ysize) return;

	flatpoint p;
	//p=dp->realtoscreen(data->points[i]);
	p=data->points[i];
	
	dp->NewFG(controlcolor);
	dp->drawpoint(p.x,p.y,5,1);
	dp->NewFG(~0);
	dp->drawpoint(p.x,p.y,5,0);
	dp->NewFG(0,0,0);
	dp->drawpoint(p.x,p.y,6,0);
}

/*! \todo in the future someday, might be useful to only show control points for "active" subpatches,
 *    or when shift-hovering
 */
void PatchInterface::drawControlPoints()
{
	if (!data) return;

	int r,c;
	flatpoint p;
	int rr;
	for (int c=0; c<data->xsize*data->ysize; c++) {
		//p=dp->realtoscreen(data->points[c]);
		p=data->points[c];
		if (selectablePoint(c)) {
			 //draw points with outer black circle, and just inside that a white circle
			if ((c/data->xsize)%3==0 && (c%data->xsize)%3==0) rr=4; else rr=3;
			dp->NewFG(controlcolor);
			dp->drawpoint(p.x,p.y,rr,0);

			if ((c/data->xsize)%3==0 && (c%data->xsize)%3==0) dp->NewFG(50,50,50); //is vertex point
			else dp->NewFG((unsigned long)0);//is control point
			dp->drawpoint(p.x,p.y,rr+1,0);
		} else {
			//DBG cerr <<" Nope"<<endl;
			// //draw an x
			//dp->NewFG(controlcolor);
			//dp->draw((int)p.x,(int)p.y, 7,7, 0, 8);
		}
		//DBG else cerr <<" Yep"<<endl;
		//DBG sprintf(blah,"%d",c);
		//DBG dp->textout((int)p.x+10,(int)p.y,blah,strlen(blah),LAX_CENTER);
	}
	dp->NewFG(controlcolor);
	 //draw little arrows for inner controls to point to outer corners
	if (whichcontrols==Patch_Full_Bezier) {
	  for (r=0; r<data->ysize-1; r+=3) {
		for (c=0; c<data->xsize-1; c+=3) {
			dp->drawarrow(data->points[(r+1)*data->xsize+c+1],
						  data->points[r*data->xsize+c]-data->points[(r+1)*data->xsize+c+1],0,.2,2);
			dp->drawarrow(data->points[(r+1)*data->xsize+c+2],
						  data->points[r*data->xsize+c+3]-data->points[(r+1)*data->xsize+c+2],0,.2,2);
			dp->drawarrow(data->points[(r+2)*data->xsize+c+1],
						  data->points[(r+3)*data->xsize+c]-data->points[(r+2)*data->xsize+c+1],0,.2,2);
			dp->drawarrow(data->points[(r+2)*data->xsize+c+2],
						  data->points[(r+3)*data->xsize+c+3]-data->points[(r+2)*data->xsize+c+2],0,.2,2);
		}
	  }
	}
	
	 // curpoints
	if (curpoints.n) {
		//dp->DrawScreen();
		for (int c=0; c<curpoints.n; c++) {
			//p=dp->realtoscreen(data->points[curpoints.e[c]]);
			p=data->points[curpoints.e[c]];
			dp->drawpoint(p.x,p.y,3,1);  // draw curpoint
		}
		//dp->DrawReal();
	}
}

/*! *** need better scanning, to start at point after currently selected
 * // no pick wild,start,end,  picks closest within a distance
 * // *** only checks points, no online jazz
 */
int PatchInterface::scan(int x,int y)
{
	if (!data) return -1;
	flatpoint p,p2;
	p=screentoreal(x,y);
	double d=5/dp->Getmag(),dd;//*** Getmag or dp->Getmag??
	//DBG cerr <<"scan d="<<d<<"(x,y)="<<p.x<<','<<p.y<<endl;
	
	d*=d;
	int closest=-1;
	 // scan for control points
	int c;
	for (c=0; c<data->xsize*data->ysize; c++) {
		p2=transform_point(data->m(),data->points[c]);
		dd=(p2.x-p.x)*(p2.x-p.x)+(p2.y-p.y)*(p2.y-p.y);
		//DBG cerr <<"  scan "<<c<<", d="<<dd<<"  ";
		if (dd<d) {
			d=dd;
			closest=c;
		}
	}
	//DBG cerr <<" found:"<<closest<<endl;
	//DBG cerr <<" scan found closest:"<<closest<<"  at d="<<d<<" (x,y)="<<p.x<<','<<p.y<<endl;
	return closest; // scan never checks a wildpoint
}

//! Return whether point c is ok to select.
/*! For instance, in a Coons patch, the inner controls for each
 * subsection are not selectable.
 */
int PatchInterface::selectablePoint(int i)
{
	if (!data) return 0;
	
	int c,r;
	c=i%data->xsize;
	r=i/data->xsize;
	//DBG cerr <<"============== check r,c:"<<r<<','<<c<<": ";
	if (whichcontrols==Patch_Border_Only) {
		 // controls on outer edges only
		return c==0 || r==0 || r==data->ysize-1 || c==data->xsize-1;
	} else if (whichcontrols==Patch_Coons) {
		 // all but the inner 4 controls of each subsection
		return c%3==0 || r%3==0;
	} else if (whichcontrols==Patch_Linear) {
		 // the outer 4 corners only
		return (c==0 && r==0)
			  || (c==0 && r==data->ysize-1)
			  || (c==data->xsize-1 && r==0 )
			  || (c==data->xsize-1 && r==data->ysize-1);
	} 
	return 1;
}

/*! Returns num of points selected.
 */
int PatchInterface::SelectPoint(int c,unsigned int state)
{
	if (!data) return 0;
	if (c<0 || c>=data->xsize*data->ysize) return 0;
	if (!(state&ShiftMask)) curpoints.flush();
	for (int cc=0; cc<curpoints.n; cc++) {
		if (c==curpoints.e[cc]) {
			curpoints.pop(cc);
			needtodraw|=2;
			return curpoints.n;
		}
	}
	curpoints.push(c);
	needtodraw|=2;
	return curpoints.n;
}

int PatchInterface::LBDown(int x,int y,unsigned int state,int count,const Laxkit::LaxMouse *d)
{
	//DBG cerr << "  in patch lbd..";
	if (buttondown.any()) return 0;
	mx=bx=x;
	my=by=y;
	mousedragged=0;
	dragmode=0;
	hoverpoint=-1;
	
	 // straight click
	int c=scan(x,y);
	if (c>=0) { // scan found one...
		SelectPoint(c,state);
		if (overv>=0 || overh>=0) {
			overv=overh=-1;
			needtodraw=1;
		}
		buttondown.down(d->id,LEFTBUTTON);
		return 0;
	}
	//DBG cerr <<"  no patch point found  ";

	buttondown.down(d->id,LEFTBUTTON);

	 // click on nothing deselects any selected points
	if (curpoints.n) {
		if ((state&LAX_STATE_MASK)==0) {
			curpoints.flush();
			needtodraw=1;
		}
		return 0;
	}
	
//	if (data) { // is going to select points within rect***
//		if (!(state&ShiftMask)) { curpoints.flush(); needtodraw=1; }
//		drawselrect();
//		return 0; 
//	}

	transform_identity(movetransform);
	if (data && overstate==0 && (overh>=0 || overv>=0)) {
		if ((state&LAX_STATE_MASK)==0) {
			int i;
			if (overh>=0) {
				for (int c=0; c<data->xsize; c++) {
					i=3*overh*data->xsize+c;
					SelectPoint(i,ShiftMask);
					if (data->style&PATCH_SMOOTH) { // add adjacent control points
						if (3*overh<data->ysize-1) SelectPoint(i+data->xsize,ShiftMask);
						if (overh>0) SelectPoint(i-data->xsize,ShiftMask);
					}
				}
			}
			if (overv>=0) {
				for (int r=0; r<data->ysize; r++) {
					i=r*data->xsize+3*overv;
					SelectPoint(i,ShiftMask);
					if (data->style&PATCH_SMOOTH) { // add adjacent control points
						if (3*overv<data->xsize-1) SelectPoint(i+1,ShiftMask);
						if (overv>0) SelectPoint(i-1,ShiftMask);
					}
				}
			}
			overv=overh=-1;
			dragmode=0;
			needtodraw=1;
			return 0;
		} else if ((state&LAX_STATE_MASK)==ShiftMask) {
			 //extend off outer edge
			//DBG cerr <<"   changing dragmode "<<endl;
			dragmode=DRAG_ADD_EDGE;
			lbdown=transform_point_inverse(data->m(),screentoreal(x,y));
			transform_identity(movetransform);
//***better to use plain transform, compute points in refresh?
//			if (movepts) { delete[] movepts;  movepts=NULL; }
//			if (overh==0) {
//				movepts=new flatpoint[data->xsize+2];
//				for (int c=0; c<data->xsize; c++) movepts[c+1]=data->points[c];
//			} else if (overh>0) {
//				movepts=new flatpoint[data->xsize+2];
//				for (int c=0; c<data->xsize; c++) movepts[c+1]=data->points[(data->ysize-1)*data->xsize+c];
//			} else if (overv>0) {
//				movepts=new flatpoint[data->ysize+2];
//				for (int c=0; c<data->ysize; c++) movepts[c+1]=data->points[(data->ysize-1)*data->xsize+c];
//			} else {
//				movepts=new flatpoint[data->ysize+2];
//			}
			overcv=overch=-1;
			needtodraw=1;
			return 0;
		}
	}
	
	if (viewport) {
		SomeData *obj=NULL;
		ObjectContext *oc=NULL;
		int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
		if (oc) obj=oc->obj;
		if (c>0) {
			 //found another patch object to work on
			viewport->ChangeObject(oc,0);
			deletedata();
			data=dynamic_cast<PatchData*>(obj);
			poc=oc->duplicate();
			data->inc_count();
			needtodraw=1;
			return 0;
		}
		if (!primary) {
			 //clicked on some other object, switch to that one
			if (obj && c==-1 && viewport->ChangeObject(oc,1)) {
				buttondown.up(d->id,LEFTBUTTON);
				deletedata(); 
				return 0; 
			}
		}
	}
	
	if (data) {
		deletedata();
		needtodraw=1;
		return 0;
	}
	
	 // make new one
	if (viewport) viewport->ChangeContext(x,y,NULL);
	flatpoint p=screentoreal(x,y);
	double w=250/dp->Getmag();
	
	PatchData *ndata=newPatchData(p.x,p.y,w,w,ys,xs,0); //starts with count 1
	if (viewport) {
		ObjectContext *oc=NULL;
		viewport->NewData(ndata,&oc);
		if (oc) poc=oc->duplicate();
	}
	data=ndata;
	data->linestyle=linestyle;
	data->FindBBox();
	curpoints.flush();
	
	needtodraw=1;
	return 0;
	//DBG cerr <<"..patchlbd done   ";
}

int PatchInterface::LBUp(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	buttondown.up(d->id,LEFTBUTTON);
	if (dragmode==DRAG_ADD_EDGE || dragmode==DRAG_SHIFT_EDGE) {
		 //add off to side
		if (overv==0) data->grow(0,movetransform);
		else if (overv>0) data->grow(2,movetransform);
		else if (overh==0) data->grow(3,movetransform);
		else if (overh>0) data->grow(1,movetransform);
		overv=overh=-1;
		overstate=0;
		needtodraw=1;
		data->touchContents();

	} else if (data && (overh>=0 || overv>=0) && (state&LAX_STATE_MASK)==ControlMask) {
		 //cut those things
		if (data) {
			data->collapse(overh,-1);
			data->collapse(-1,overv);
		}
		overh=overv=-1;
		needtodraw=1;
		data->touchContents();

	} else if (data && (overch>=0 || overcv>=0) && (state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
		 // subdivide
		data->subdivide(overch,cutatrt,overcv,cutatct);
		overcv=overch=overh=overv=-1;
		needtodraw=1;
		data->touchContents();
	}
	
	if (data && viewport && dragmode==0) viewport->ObjectMoved(poc,1);
	dragmode=0;
	//needtodraw=1;
	constrain=0;
	return 0;
}

//! Return the column number corresponding to the first patch edge within d of fp.
/*! This goes by vertex column, so for data->xsize==4, there are columns 0 and 1.
 */
int PatchInterface::findNearVertical(flatpoint fp,double d,double *t_ret,int *i_ret)
{
	int r,c;
	//DBG cerr <<"findv: d="<<d<<"  fp="<<fp.x<<","<<fp.y<<"  d="<<d<<"  ";
	
	flatpoint p[data->ysize];
	double dd,ddd=1e+10;
	DoubleBBox bbox;
	int resolution=2,rrr;
	for (c=0; c<data->xsize/3+1; c++) {
		bbox.clear();
		for (r=0; r<data->ysize; r++) {
			p[r]=data->points[c*3 + r*data->xsize];
			bbox.addtobounds(p[r]);
			if (r && r%3==0) {
				rrr=(int)(MAX(bbox.maxx-bbox.minx,bbox.maxy-bbox.miny)/d*2);
				if (rrr>resolution) resolution=rrr;
				bbox.clear();
				bbox.addtobounds(p[r]);
			}
		}
		dd=bez_near_point(fp,p,data->ysize,resolution,t_ret,i_ret);
		if (dd<ddd) { ddd=dd; }
		
		//DBG cerr <<"c:"<<c<<"  distance:"<<dd<<endl;
		
		if (dd<d) return c;
	}
	//DBG cerr <<" closest:"<<c<<"  ddd:"<<ddd<<endl;
	return -1;
}

//! Return the row number corresponding to the first patch edge within d of fp.
int PatchInterface::findNearHorizontal(flatpoint fp,double d,double *t_ret,int *i_ret)
{
	int r,c;
	//DBG cerr <<"findh: d="<<d<<"  fp="<<fp.x<<","<<fp.y<<"  d="<<d<<"  ";
	
	flatpoint p[data->xsize];
	DoubleBBox bbox;
	int dd;
	int resolution=2;
	for (r=0; r<data->ysize/3+1; r++) {
		bbox.clear();
		for (c=0; c<data->xsize; c++) {
			p[c]=data->points[c + 3*r*data->xsize];
			bbox.addtobounds(p[c]);
			if (c && c%3==0) {
				dd=(int)(MAX(bbox.maxx-bbox.minx,bbox.maxy-bbox.miny)/d*2);
				if (dd>resolution) resolution=dd;
				bbox.clear();
				bbox.addtobounds(p[c]);
			}
		}
		if (bez_near_point(fp,p,data->xsize,resolution,t_ret,i_ret)<d) return r;
	}
	return -1;
}

/*! \todo revamp controls:
 *    plain click in, not on point, be like image
 */
int PatchInterface::MouseMove(int x,int y,unsigned int state,const Laxkit::LaxMouse *d) 
{
	if (!data) { return 1; }

	//if (!buttondown.isdown(d->id,LEFTBUTTON) && !curpoints.n) {
	if (!buttondown.isdown(d->id,LEFTBUTTON)) {
		flatpoint fp=transform_point_inverse(data->m(),screentoreal(x,y));

		//DBG int rrr,ccc;
		//DBG cerr <<" ----------------inSubPatch:"<<data->inSubPatch(fp,&rrr,&ccc, NULL,NULL, 1)<<endl;
		//DBG cerr <<" -----------------r:"<<rrr<<" c:"<<ccc<<endl;
		//DBG cerr <<"2/mag:"<<2/Getmag()<<"  2*mag:"<<2*Getmag()<<endl;
				
		int v=-1,
			h=-1,
			hr=-1,
			hc=-1;
		int at_c,c;
		double at_t,at_s;
		mx=x; my=y;

		 // if hover over points, do not do edge or subdivide operations
		c=scan(x,y);
		if (c<0 && c!=hoverpoint) { hoverpoint=-1; needtodraw=1; }
		if (c>=0 && selectablePoint(c)) {
			if (c!=hoverpoint) { hoverpoint=c; needtodraw=1; }
			v=h=hr=hc=-1;

		} else if ((state&LAX_STATE_MASK)==0 
				|| (state&LAX_STATE_MASK)==ShiftMask
				|| (state&LAX_STATE_MASK)==ControlMask) {

			//DBG cerr <<"---ONE---"<<endl;
			
			flatpoint d1=transform_point_inverse(data->m(),flatpoint(0,0)),
					  d2=transform_point_inverse(data->m(),flatpoint(1,1));
			double d=2/Getmag() *  norm(d1-d2)/(sqrt(2));
			//double d; //******** clear this up:
			//if (data->maxx-data->minx>data->maxy-data->miny) d=(data->maxx-data->minx);
			//else d=(data->maxy-data->miny);
			//if (data->xsize>data->ysize) d/=data->xsize*5; else d/=data->ysize*5;
			//DBG cerr <<"----patch---: find in a radius d:"<<d<<endl;
			
			v=  findNearVertical(fp,d,&at_t,&at_c);
			h=findNearHorizontal(fp,d,&at_t,&at_c);
			//DBG cerr <<"find near v:"<<v<<"  h:"<<h<<endl;

			if ((state&LAX_STATE_MASK)==0 || (state&LAX_STATE_MASK)==ShiftMask) {
				 //plain hover over edge for adding on rows/cols
				if (overstate!=0) { overstate=0; needtodraw=1; }
				//**********should be able to add/shift in middle of the patch
				if (v>=0 && v!=0 && v!=data->xsize/3) v=-1;
				if (h>=0 && h!=0 && h!=data->ysize/3) h=-1;
				if (v>=0 && h>=0) h=-1;
			} else {
				 //control hover for cutting
				if ((v==0 || v==data->xsize/3) && data->xsize==4) v=-1; //don't cut if nothing left afterwards!
				if ((h==0 || h==data->ysize/3) && data->ysize==4) h=-1;
				if ((v>=0 || h>=0) && overstate!=1) { overstate=1; needtodraw=1; }
				else if (overstate!=1) overstate=0;
			}

		} else if ((state&LAX_STATE_MASK)==(ControlMask|ShiftMask)) {
			//DBG cerr <<"---^+ONE---"<<endl;
			 // set up for subdivide
			flatpoint d1=transform_point_inverse(data->m(),flatpoint(0,0)),
					  d2=transform_point_inverse(data->m(),flatpoint(1,1));
			double d=2/Getmag() *  norm(d1-d2)/(sqrt(2));

			//DBG cerr <<"----patch---: find in d:"<<d<<endl;
			
			if (data->inSubPatch(fp,&hr,&hc, &at_t,&at_s, d)) {	
				if (cuth) delete[] cuth;
				cuth=new flatpoint[data->xsize+3];
				overch=hr;
				cutatrt=at_t;
				data->bezCrossSection(cuth+1,overch,at_t,1);
				
				if (cutv) delete[] cutv;
				cutv=new flatpoint[data->ysize+3];
				overcv=hc;
				cutatct=at_s;
				data->bezCrossSection(cutv+1,overcv,at_s,0);
				needtodraw=1;
				//DBG cerr <<"^+needtodraw: "<<needtodraw<<endl;

				 // if hover over edges, only cut perpendicular
				v=  findNearVertical(fp,d,&at_t,&at_c);
				if (v>=0) overcv=-1;	
				h=findNearHorizontal(fp,d,&at_t,&at_c);
				if (h>=0) overch=-1;
				
				return 0;
			} 
		}
		//DBG cerr <<"---TWO---"<<endl;
		//DBG cerr <<"subdiv: v="<<v<<"  h="<<h<<endl;
		if (v!=overv) { overv=v; needtodraw=1; }
		if (h!=overh) { overh=h; needtodraw=1; }
		if (overv<0 && overch>=0) { overch=-1; needtodraw=1; }//turn off subdivision marker line 
		if (overh<0 && overcv>=0) { overcv=-1; needtodraw=1; }
		//DBG cerr <<"needtodraw: "<<needtodraw<<endl;
		return 0; 
	}

	
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 0;
	
	if (x!=my || y!=my) mousedragged=1;

	if (dragmode==DRAG_ADD_EDGE || dragmode==DRAG_SHIFT_EDGE) {
		 // moving around outer edge in preparation for adding a row or column
		double M[6],m[6];
		if (state&ControlMask && !(state&ShiftMask)) { // scale around lbdown
			flatpoint center=transform_point(movetransform,lbdown);
			double f=1+(x-mx)/10.;
			transform_set(M,f,0,0,f,0,0);
			transform_copy(m,movetransform);
			m[4]-=center.x;
			m[5]-=center.y;
			transform_mult(movetransform,M,m);
			center-=transform_point(movetransform,lbdown);
			movetransform[4]+=center.x;
			movetransform[5]+=center.y;
		} else if (state&ShiftMask && state&ControlMask) { // rotate
			flatpoint center=transform_point(movetransform,lbdown);
			double angle=(x-mx)/70.;
			transform_rotate(movetransform,angle);
			center-=transform_point(movetransform,lbdown);
			movetransform[4]+=center.x;
			movetransform[5]+=center.y;
			
		} else { // move
			flatpoint v=  transform_point_inverse(data->m(),screentoreal(x,y))
						 -transform_point_inverse(data->m(),screentoreal(mx,my));
			movetransform[4]+=v.x;
			movetransform[5]+=v.y;
		}
		
		data->touchContents();
		mx=x; my=y;
		needtodraw=1;
		return 0;
	}

	if (curpoints.n==0) {
		flatpoint d=screentoreal(x,y)-screentoreal(mx,my);
		data->origin(data->origin()+d);
		mx=x; my=y;
		needtodraw=1;
		return 0;
	}

	 // handle cntl-move for rotations ***
	if (state&ControlMask && !(state&ShiftMask)) { // scale *** how to make scale go backwards??
		flatpoint center,p;
		int c;
		for (c=0; c<curpoints.n; c++) center+=data->points[curpoints.e[c]];
		center/=curpoints.n;
		double f=1+(x-mx)/10.;
		for (c=0; c<curpoints.n; c++) {
			p=center+(data->points[curpoints.e[c]]-center)*f;
			if (!(constrain&2)) data->points[curpoints.e[c]].y=p.y;
			if (!(constrain&1)) data->points[curpoints.e[c]].x=p.x;
		}
	} else if (state&ShiftMask && state&ControlMask) { // rotate
		flatpoint center;
		int c;
		for (c=0; c<curpoints.n; c++) center+=data->points[curpoints.e[c]];
		center/=curpoints.n;
		double angle=(x-mx);
		for (c=0; c<curpoints.n; c++) 
			data->points[curpoints.e[c]]=rotate(data->points[curpoints.e[c]],center,angle,1);
		
	} else { // move
		double m[6];
		transform_invert(m,data->m());
		flatpoint d=transform_point(m,screentoreal(x,y)) - transform_point(m,screentoreal(mx,my));
		if (data->style&PATCH_SMOOTH) {
			 // tricky movements to maintain smoothness
			int *ps=new int[curpoints.n],
				n=curpoints.n,
				r,c,c2,i,m, rr,cc,ii;
			for (c=0; c<n; c++) ps[c]=curpoints.e[c];
			
			 // procedure here is search out control points, move those + adjacent, remove all those from stack
			 // then search for edge points, move the corresponding center control points
			 // then with any remaining inner control points, move those so the corresponding
			 // 	controls of adjacent patches are moved all in a line
			for (m=0; m<n; m++) {
				if (ps[m]<0) continue;
				i=curpoints.e[m];
				r=i/data->xsize;
				c=i%data->xsize;
				if (r%3!=0 || c%3!=0) continue;
				 // now i should be a vertex point
				//data->points[i]+=d;
				for (rr=r-1; rr<r+2; rr++) {
					for (cc=c-1; cc<c+2; cc++) {
						if (rr<0 || cc<0 || rr>=data->ysize || cc>=data->xsize) continue;
						//if (rr==r && cc==c) continue;
						ii=rr*data->xsize + cc;
						data->points[ii]+=d;
						for (c2=0; c2<n; c2++) { // remove ii from ps
							if (ps[c2]==ii) { ps[c2]=-1; break; }
						}
					}
				}
			}
			 //move remaining control points
			for (m=0; m<n; m++) {
				if (ps[m]<0) continue;
				i=curpoints.e[m];
				data->points[i]+=d;
			}
			delete[] ps;
		} else for (int c=0; c<curpoints.n; c++) data->points[curpoints.e[c]]+=d;
	}
	if (whichcontrols!=Patch_Full_Bezier) data->InterpolateControls(whichcontrols);
	data->FindBBox();
	data->touchContents();
	mx=x; my=y;
	needtodraw=1;
	return 0;
}

Laxkit::ShortcutHandler *PatchInterface::GetShortcuts()
{
	if (sc) return sc;
	ShortcutManager *manager=GetDefaultShortcutManager();
	sc=manager->NewHandler(whattype());
	if (sc) return sc;

	//virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

	sc=new ShortcutHandler(whattype());

	sc->Add(PATCHA_RenderMode,              'm',0,0,          "RenderMode",  _("Render mode"),NULL,0);
	sc->Add(PATCHA_RecurseInc,              'o',0,0,          "RecurseInc",  _("Increment screen recursion"),NULL,0);
	sc->Add(PATCHA_RecurseDec,              'O',ShiftMask,0,  "RecurseDec",  _("Decrement screen recursion"),NULL,0);
	sc->Add(PATCHA_SmoothEdit,              'j',0,0,          "SmoothEdit",  _("Toggle smooth editing"),NULL,0);
	sc->Add(PATCHA_MeshType,                'b',0,0,          "MeshType",    _("Toggle mesh type"),NULL,0);
	sc->Add(PATCHA_MeshTypeR,               'B',ShiftMask,0,  "MeshTypeR,",  _("Toggle mesh type in reverse"),NULL,0);
	sc->Add(PATCHA_Select,                  'a',0,0,          "Select",      _("Select all or deselect"),NULL,0);
	sc->Add(PATCHA_ConstrainY,              'y',0,0,          "ConstrainY",  _("Constrain to vertical movements"),NULL,0);
	sc->Add(PATCHA_ConstrainX,              'x',0,0,          "ConstrainX",  _("Constrain to horizontal movements"),NULL,0);
	sc->Add(PATCHA_RowDivInc,               'R',ControlMask|ShiftMask,0,"RowDivInc",  _("Increment row subdivisions"),NULL,0);
	sc->Add(PATCHA_RowDivDec,               'r',ControlMask,          0,"RowDicDec",  _("Decrement row subdivisions"),NULL,0);
	sc->Add(PATCHA_ColDivInc,               'C',ControlMask|ShiftMask,0,"ColDivInc",  _("Increment row subdivisions"),NULL,0);
	sc->Add(PATCHA_ColDivDec,               'c',ControlMask,          0,"ColDivDec",  _("Decrement row subdivisions"),NULL,0);
	sc->Add(PATCHA_Subdivide,               's',0,0,          "Subdivide",               _("Subdivide"),NULL,0);
	sc->Add(PATCHA_SubdivideRows,           'r',0,0,          "SubdivideRows",           _("Subdivide rows"),NULL,0);
	sc->Add(PATCHA_SubdivideCols,           'c',0,0,          "SubdivideCols",           _("Subdivide columns"),NULL,0);
	sc->Add(PATCHA_Rectify,                 'z',0,0,          "Rectify",                 _("Zap back to a rectangle"),NULL,0);
	sc->Add(PATCHA_Decorations,             'd',0,0,          "Decorations",             _("Toggle decorations"),NULL,0);
	sc->Add(PATCHA_CircleWarp,              'w',0,0,          "CircleWarp",              _("Make circle"),NULL,0);
	sc->Add(PATCHA_SelectCorners,           '1',0,0,          "SelectCorners",           _("Select corners"),NULL,0);
	sc->Add(PATCHA_SelectMids,              '2',0,0,          "SelectMids",              _("Select middle controls"),NULL,0);
	sc->Add(PATCHA_SelectEdgeMids,          '3',0,0,          "SelectEdgeMids",          _("Select edge controls"),NULL,0);
	sc->Add(PATCHA_SelectVerticalEdgeMids,  '4',0,0,          "SelectVerticalEdgeMids",  _("Select vertical edge controls"),NULL,0);
	sc->Add(PATCHA_SelectHorizontalEdgeMids,'5',0,0,          "SelectHorizontalEdgeMids",_("Select horizontal edge controls"),NULL,0);
	sc->Add(PATCHA_SelectAround,            '6',0,0,          "SelectAround",            _("Select points around current points"),NULL,0);
	sc->Add(PATCHA_SelectAllVertically,     'V',ShiftMask,0,  "SelectAllVertically",     _("Select all vertically"),NULL,0);
	sc->Add(PATCHA_SelectMoreVertically,    'v',0,0,          "SelectMoreVertically",    _("Select next points vertically"),NULL,0);
	sc->Add(PATCHA_SelectAllHorizontally,   'H',ShiftMask,0,  "SelectAllHorizontally",   _("Select all horizontally"),NULL,0);
	sc->Add(PATCHA_SelectMoreHorizontally,  'h',0,0,          "SelectMoreHorizontally",  _("Select next points horizontally"),NULL,0);

	manager->AddArea(whattype(),sc);
	return sc;
}

int PatchInterface::PerformAction(int action)
{
	if (action==PATCHA_RenderMode) {
		//plain patch has only one render mode, so return 1 here.
		return 1;

	} else if (action==PATCHA_RecurseInc) {
		recurse--;
		if (recurse<0) recurse=0;
		char blah[50];
		sprintf(blah,_("Recurse for screen %d times"),recurse);
		PostMessage(blah);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_RecurseDec) {
		recurse++;
		char blah[50];
		sprintf(blah,_("Recurse for screen %d times"),recurse);
		PostMessage(blah);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SmoothEdit) {
		if (data) {
			data->style^=PATCH_SMOOTH;
			if (viewport) {
				if (data->style&PATCH_SMOOTH) viewport->postmessage(_("Smooth edit mode"));
				else viewport->postmessage("Free edit mode");
			}
			needtodraw=1;
			return 0;
		}
		return 0;

	} else if (action==PATCHA_MeshType || action==PATCHA_MeshTypeR) {
		 // toggle which controls available to edit
		needtodraw=1;
		if (action==PATCHA_MeshType) {
			whichcontrols++;
			if (whichcontrols>=Patch_MAX) whichcontrols=Patch_Full_Bezier;
		} else {
			whichcontrols--;
			if (whichcontrols<Patch_Full_Bezier) whichcontrols=Patch_MAX;
		}
		if (viewport) {
			if (whichcontrols==Patch_Linear)           viewport->postmessage(_("Edit as a linear patches"));
			else if (whichcontrols==Patch_Coons)       viewport->postmessage(_("Edit as Coons patches" ));
			else if (whichcontrols==Patch_Border_Only)  viewport->postmessage(_("Edit with border controls only")); 
			else if (whichcontrols==Patch_Full_Bezier) viewport->postmessage(_("Edit full cubic bezier patch"));
		}
		return 0;

	} else if (action==PATCHA_Select) {
		if (curpoints.n) {
			curpoints.flush();
		} else {
			if (!data) return 0;
			for (int c=0; c<data->xsize*data->ysize; c++) curpoints.push(c);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_ConstrainY) {
		if (constrain==2) constrain=0; else constrain=2;
		return 0;

	} else if (action==PATCHA_ConstrainX) {
		if (constrain==1) constrain=0; else constrain=1;
		return 0;

	} else if (action==PATCHA_RowDivInc) {
		rdiv--;
		if (rdiv<2) rdiv=2;
		char blah[50];
		sprintf(blah,_("Divide each row into %d rows now"),rdiv);
		PostMessage(blah);
		return 0;

	} else if (action==PATCHA_RowDivDec) {
		rdiv++;
		char blah[50];
		sprintf(blah,_("Divide each row into %d rows now"),rdiv);
		PostMessage(blah);
		return 0;

	} else if (action==PATCHA_ColDivDec) {
		cdiv--;
		if (cdiv<2) cdiv=2;
		char blah[50];
		sprintf(blah,_("Divide each column into %d columns now"),cdiv);
		PostMessage(blah);
		return 0;

	} else if (action==PATCHA_ColDivInc) {
		cdiv++;
		char blah[50];
		sprintf(blah,_("Divide each column into %d columns now"),cdiv);
		PostMessage(blah);
		return 0;

	} else if (action==PATCHA_Subdivide) {
		if (!data) return 0;
		data->subdivide(rdiv,cdiv);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SubdivideRows) {
		if (!data) return 0;
		data->subdivide(rdiv,1);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SubdivideCols) {
		if (!data) return 0;
		data->subdivide(1,cdiv);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_Rectify) {
		if (!data) return 0;
		data->zap();
		needtodraw=1;
		curpoints.flush();
		return 0;

	} else if (action==PATCHA_Decorations) {
		showdecs++;
		if (showdecs==SHOW_Max) showdecs=0;
		char blah[200];
		sprintf(blah,"Show: ");
		if (showdecs&SHOW_Grid) strcat(blah,"grid ");
		if (showdecs&SHOW_Points) strcat(blah,"points ");
		if (showdecs&SHOW_Edges) strcat(blah,"edges ");
		PostMessage(blah);
		
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_CircleWarp) {
		if (!data) return 0;
		if (data->xsize==4) {
			const char *mes=_("Patches need more than 1 column of subpatches to warp.");
			PostMessage(mes);
			return 0;
		}
		 //*** need to implement clicking select of center r1 r2 s e
		flatpoint center=screentoreal(curwindow->win_w/2,curwindow->win_h/2);
		double r1,r2,s,e;
		r1=0;
		r2=norm(screentoreal(curwindow->win_w*4/5,curwindow->win_h/2)
				- screentoreal(curwindow->win_w/2,curwindow->win_h/2));
		r1=r2/2;
		s=0;
		e=2*M_PI;
		data->warpPatch(center,r1,r2,s,e);
		//data->origin(screentoreal(int(curwindow->win_w/2-(data->maxx-data->minx)/2),
		//						  int(curwindow->win_h/2-(data->maxy-data->miny)/2)));
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectCorners) {
		if (!data) return 0;
		curpoints.flush();
		curpoints.push(0);
		curpoints.push(3);
		curpoints.push(3*data->xsize);
		curpoints.push(3*data->xsize+3);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectMids) {
		if (!data) return 0;
		curpoints.flush();
		curpoints.push(1*data->xsize + 1);
		curpoints.push(1*data->xsize + 2);
		curpoints.push(2*data->xsize + 1);
		curpoints.push(2*data->xsize + 2);
		needtodraw=1; 
		return 0;

	} else if (action==PATCHA_SelectEdgeMids) {
		if (!data) return 0;
		curpoints.flush();
		curpoints.push(0*data->xsize + 1);
		curpoints.push(0*data->xsize + 2);
		curpoints.push(1*data->xsize + 0);
		curpoints.push(2*data->xsize + 0);
		curpoints.push(1*data->xsize + 3);
		curpoints.push(2*data->xsize + 3);
		curpoints.push(3*data->xsize + 1);
		curpoints.push(3*data->xsize + 2);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectVerticalEdgeMids) {
		if (!data) return 0;
		curpoints.flush();
		curpoints.push(1*data->xsize + 0);
		curpoints.push(2*data->xsize + 0);
		curpoints.push(1*data->xsize + 3);
		curpoints.push(2*data->xsize + 3);
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectHorizontalEdgeMids) {
		if (!data) return 0;
		curpoints.flush();
		curpoints.push(0*data->xsize + 1);
		curpoints.push(0*data->xsize + 2);
		curpoints.push(3*data->xsize + 1);
		curpoints.push(3*data->xsize + 2);
		needtodraw=1; 
		return 0;

	} else if (action==PATCHA_SelectAllVertically) {
		if (!data) return 0;
		int cc,n=curpoints.n,r,c;
		for (cc=0; cc<n; cc++) {
			r=curpoints.e[cc]/data->xsize;
			c=curpoints.e[cc]%data->xsize;
			while (r>0) { curpoints.pushnodup((r-1)*data->xsize+c); r--; }
			r=curpoints.e[cc]/data->xsize;
			while (r<data->ysize-1) { curpoints.pushnodup((r+1)*data->xsize+c); r++; }
		}
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectMoreVertically) {
		if (!data) return 0;
		int cc,n=curpoints.n,r,c;
		for (cc=0; cc<n; cc++) {
			r=curpoints.e[cc]/data->xsize;
			c=curpoints.e[cc]%data->xsize;
			if (r>0) curpoints.pushnodup((r-1)*data->xsize+c);
			if (r<data->ysize-1) curpoints.pushnodup((r+1)*data->xsize+c);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectAllHorizontally) {
		if (!data) return 0;
		int cc,n=curpoints.n,r,c;
		for (cc=0; cc<n; cc++) {
			r=curpoints.e[cc]/data->xsize;
			c=curpoints.e[cc]%data->xsize;
			while (c>0) { curpoints.pushnodup((r)*data->xsize+c-1); c--; }
			c=curpoints.e[cc]%data->xsize;
			while (c<data->xsize-1) { curpoints.pushnodup((r)*data->xsize+c+1); c++; }
		}
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectMoreHorizontally) {
		if (!data) return 0;
		int cc,n=curpoints.n,r,c;
		for (cc=0; cc<n; cc++) {
			r=curpoints.e[cc]/data->xsize;
			c=curpoints.e[cc]%data->xsize;
			if (c>0) curpoints.pushnodup((r)*data->xsize+c-1);
			if (c<data->xsize-1) curpoints.pushnodup((r)*data->xsize+c+1);
		}
		needtodraw=1;
		return 0;

	} else if (action==PATCHA_SelectAround) {
		if (!data) return 0;
		int cc,n=curpoints.n,i,r,c;
		for (cc=0; cc<n; cc++) {
			r=curpoints.e[cc]/data->xsize;
			c=curpoints.e[cc]%data->xsize;
			for (int rr=r-1; rr<r+2; rr++)
				for (i=c-1; i<c+2; i++)
					if (rr>=0 && rr<data->ysize && i>=0 && i<data->xsize) curpoints.pushnodup((rr)*data->xsize+i);
		}
		needtodraw=1;
		return 0;
	}

	return 1;
}

/*! 
 * <pre>
 *  'b'  toggle which control points can be shifted
 *  'B'  opposite of 'b' 
 *  'j'  toggle smooth editing mode (j for jagged)
 *  'a'  select all points, or deselect all if any are selected
 *  'y'  constrain to y changes, or release the constraint
 *  'x'  constrain to x changes, or release the constraint
 *  'R'  increase how many rows to divide each row into
 * ^'R'  decrease how many rows to divide each row into
 *  'r'  subdivide rows
 *  'c'  subdivide columns
 *  'C'  increase how many columns to divide each column into
 * ^'C'  decrease how many columns to divide each column into
 *  's'  subdivide rows and columns
 *  'z'  reset to rectangular (z for zap)
 *  'd'  toggle decorations
 *  'H'  select all points in rows of current points
 *  'V'  select all points in columns of current points
 *  'h'  select points adjacent horizontally to current points
 *  'v'  select points adjacent vertically to current points
 *  'm'  for debugging, popup a showmat
 *  '1'  select corners:  0,0  0,3  3,0  3,3
 *  '2'  select center controls: 1,1  1,2  2,1  2,2
 *  '3'  select edge controls: 0,1  0,2  1,0  2,0  1,3  2,3  3,1  3,2
 *  '4'  select top and bottom controls: 1,0  2,0  1,3  2,3
 *  '5'  select left and right controls: 0,1  0,2  3,1  3,2
 *  '8'  select a 3x3 group of points around each current point
 *  'i'  reset patch inside points to Coons like values
 *  'o'   increment how much to recurse
 *  'O'   decrement how much to recurse
 * </pre>
 *
 * \todo *** l/r/u/d to shift selected points:
 *   plain-move rolls curpoints,
 *   shift-move adds to curpoints,
 *   control-move rolls without wraparound
 * \todo whichcontrols or data->controls?
 */
int PatchInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state,const Laxkit::LaxKeyboard *d) 
{
	if (ch==LAX_Shift || ch==LAX_Control) {
		MouseMove(mx,my,state,d->paired_mouse);
		return 0;
	}

	if (ch==LAX_Esc) {
		if (curpoints.n) {
			curpoints.flush();
			needtodraw=1;
			return 0;
		}

	}
	//DBG else if (ch=='m' && (state&LAX_STATE_MASK)==(ShiftMask|ControlMask)) {
	//DBG 	if (!data) return 1;
	//DBG 	app->addwindow(new showmat(NULL,"Matrix",0, 0,0, 800,500, 0, data->points,data->xsize,data->ysize));
	//DBG 	return 0;
	//DBG }

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}
	
	return 1; 
}

int PatchInterface::KeyUp(unsigned int ch,unsigned int state,const Laxkit::LaxKeyboard *d) 
{ //*** shift/noshift toggle end/start point
	//if (!buttondown) return 1;
	if (ch==LAX_Shift || ch==LAX_Control) {
		MouseMove(mx,my,state,d->paired_mouse);
		return 0;
	}
	return 1; 
}


} // namespace LaxInterfaces

