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
//    Copyright (C) 2016 by Tom Lechner
//



#include <lax/interfaces/perspectiveinterface.h>

#include <lax/interfaces/somedatafactory.h>
#include <lax/laxutils.h>
#include <lax/language.h>


//You need this if you use any of the Laxkit stack templates in lax/lists.h
//The few templates Laxkit provides are divided into header.h/implementation.cc.
#include <lax/lists.cc>


using namespace Laxkit;


#include <iostream>
using namespace std;
#define DBG 


namespace LaxInterfaces {





//---------------------------------- Perspective Transform Utils -----------------------------
//https://github.com/jlouthan/perspective-transform
//
//The MIT License (MIT)
//
//Copyright (c) 2015 Jenny Louthan
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.


//--------------- class Matrix -------------------
class Matrix
{
  public:
	int rows, cols;
	double **d;

	Matrix(const Matrix &matrix);
	Matrix(int nrows, int ncols);
	~Matrix();

	void Allocate(int nrows, int ncols);
	void Identity();
	void Transpose();
	void Invert();

	double m(int r, int c) { return d[r][c]; }
	double m(int r, int c, double nvalue) { return d[r][c]=nvalue; }
};

Matrix::Matrix(const Matrix &matrix)
{
	rows=matrix.rows;
	cols=matrix.cols;

	d=new double*[rows];
	for (int r=0; r<rows; r++) {
		d[r]=new double[cols];
		for (int c=0; c<cols; c++) {
			d[r][c]=matrix.d[r][c];
		}
	}
}

Matrix::Matrix(int nrows, int ncols)
{
	rows=nrows;
	cols=ncols;

	d=new double*[nrows];
	for (int r=0; r<rows; r++) d[r]=new double[cols];
}

Matrix::~Matrix()
{
	for (int r=0; r<rows; r++) delete[] d[r];
	delete[] d;
}

void Matrix::Identity()
{
	for (int r=0; r<rows; r++) {
		for (int c=0; c<cols; c++) {
			if (r==c) d[r][c]=1;
			else d[r][c]=0;
		}
	}
}

void Matrix::Allocate(int nrows, int ncols)
{
	if (nrows==rows && cols==ncols) return;

	for (int r=0; r<rows; r++) delete[] d[r];
	delete[] d;

	d=new double*[nrows];
	for (int r=0; r<rows; r++) d[r]=new double[cols];
}

//--------------- end class Matrix -------------------


double **AllocateMatrix(int nrows, int ncols)
{
	double **d;

	d=new double*[nrows];
	for (int r=0; r<nrows; r++) d[r]=new double[ncols];

	return d;
}

void DeallocateMatrix(int rows, double **d)
{
	if (!d) return;
	for (int r=0; r<rows; r++) delete[] d[r];
	delete[] d;
}

/*! Copy a vector. Returns ret.
 */
double *Numeric_cloneV(int n, double *x, double *ret)
{
	if (!ret) ret=new double[n];

	for (int i=n-1; i!=-1; --i) {
		ret[i] = (x[i]);
	}
	return ret;
}

/*! Copy matrix
 */ 
double **Numeric_clone(int rows, int cols, double **x, double**ret)
{
	if (!ret) ret=AllocateMatrix(rows,cols);

	for (int r=0; r<rows; r++) {
		for (int c=0; c<cols; c++) {
			ret[r][c]=x[r][c];
		}
	}

	return ret;
}

/*! Make diagonal matrix.
 */
double **Numeric_diag(int n, double *d, double **ret)
{
	if (!ret) ret=AllocateMatrix(n,n);

	for (int r=0; r<n; r++) {
		for (int c=0; c<n; c++) {
			if (r==c) ret[r][c]=d[r];
			else ret[r][c]=0;
		}
	}

	return ret;
}

/*! Create identity matrix.
 */
double **Numeric_identity(int n, double **d)
{
	if (!d) d=AllocateMatrix(n,n);

	for (int r=0; r<n; r++) {
		for (int c=0; c<n; c++) {
			if (r==c) d[r][c]=1;
			else d[r][c]=0;
		}
	}

	return d;
}

/*! Compute inverse of matrix.
 */
double **Numeric_inv(int rows, int cols, double **a)
{
	int m=rows, n=cols;
	double **A = AllocateMatrix(m,n);
	Numeric_clone(m,n, a, A);
	double *Ai, *Aj;

	double **I = AllocateMatrix(m,m);
	Numeric_identity(m, I);
	double *Ii, *Ij;
	int i,j,k;
	double x;

	for (j=0; j<n; ++j) {
		int i0 = -1;
		int v0 = -1;

		for(i=j; i!=m; ++i) {
			k = fabs(A[i][j]);
			if(k>v0) { i0 = i; v0 = k; }
		}

		if (i0<0) throw(1);

		Aj = A[i0]; A[i0] = A[j]; A[j] = Aj;
		Ij = I[i0]; I[i0] = I[j]; I[j] = Ij;
		x = Aj[j];

		for (k=j; k!=n; ++k)    Aj[k] /= x; 
		for (k=n-1; k!=-1; --k) Ij[k] /= x;
		for (i=m-1; i!=-1; --i) {
			if (i!=j) {
				Ai = A[i];
				Ii = I[i];
				x = Ai[j];
				for (k=j+1; k!=n; ++k)  Ai[k] -= Aj[k]*x;
				for (k=n-1; k>0; --k) { Ii[k] -= Ij[k]*x; --k; Ii[k] -= Ij[k]*x; }
				if (k==0) Ii[0] -= Ij[0]*x;
			}
		}
	}

	DeallocateMatrix(m, A);

	return I;
}

/*! Matrix times matrix.
 * ret should be double[rows of x][cols of y].
 * If ret==NULL, return a new double**.
 */
double **Numeric_dotMMsmall(int numxrows, int numxcols, int numycols, double **x, double **y, double **ret)
{
	int i,j,k,p,q,r;
	double *foo, *bar;
	double woo;
	int i0;

	p = numxrows; q = numxcols; r = numycols;
	double **rret=NULL;
	if (!ret) rret = new double*[p];

	for (i=p-1; i>=0; i--) {
		if (ret) foo=ret[i];
		else foo = new double[r];

		bar = x[i];
		for (k=r-1; k>=0; k--) {
			woo = bar[q-1]*y[q-1][k];
			for (j=q-2; j>=1; j-=2) {
				i0 = j-1;
				woo += bar[j]*y[j][k] + bar[i0]*y[i0][k];
			}
			if (j==0) { woo += bar[0]*y[0][k]; }
			foo[k] = woo;
		}

		if (rret) rret[i] = foo;
	}

	if (ret) return ret;
	return rret;
}

/*! Vector dot product.
 */
double Numeric_dotVV(int n, double *x, double *y)
{
	double ret=0;
	for(int i=0; i<n; i++) {
		ret += x[i]*y[i];
	}
	return ret;
};

/*! Matrix times column v = column v. Just returns ret. If ret==NULL, return new double[rows].
 */
double *Numeric_dotMV(int rows,int cols, double **x, double *y, double *ret)
{
	if (!ret) ret=new double[rows];

	for (int i=0; i<rows; i++) {
		ret[i] = Numeric_dotVV(cols, x[i],y);
	}
	return ret;
};

/*! Matrix transpose. ret should be double[cols][rows], and is returned new if ret==NULL.
 */
double **Numeric_transpose(int rows, int cols, double **x, double **ret)
{
	if (ret==NULL) ret=AllocateMatrix(cols, rows);

	for (int r=0; r<cols; r++) {
		for (int c=0; c<rows; c++) {
			ret[r][c] = x[c][r];
		}
	}

	return ret;
}


//----------------------------- PerspectiveTransform ------------------------------

/*! \class PerspectiveTransform
 *
 * Holds info about transforming vie perspective by matching 4 points before and after.
 * The original 4 don't have to be rectangular.
 */

PerspectiveTransform::PerspectiveTransform()
{
	memset(srcPts, 0, 8*sizeof(double));
	memset(dstPts, 0, 8*sizeof(double));

	from_ll.set(0,0);
	from_lr.set(1,0);
	from_ul.set(0,1);
	from_ur.set(1,1);

	ResetTransform();
}

PerspectiveTransform::PerspectiveTransform(flatpoint *nsrcPts, flatpoint *ndstPts)
{ 
	for (int c=0; c<4; c++) {
		srcPts[2*c  ] = nsrcPts[c].x;
		srcPts[2*c+1] = nsrcPts[c].y;
		dstPts[2*c  ] = ndstPts[c].x;
		dstPts[2*c+1] = ndstPts[c].y;
	}

	//memcpy(this.srcPts, srcPts, 4*sizeof(flatpoint));
	//memcpy(this.dstPts, dstPts, 4*sizeof(flatpoint));

	ComputeTransform();
}

void PerspectiveTransform::ResetTransform()
{
	to_ll=from_ll;
	to_lr=from_lr;
	to_ul=from_ul;
	to_ur=from_ur;

	coeffs[0]=coeffs[4]=coeffs[8]=1;
	coeffs[1]=coeffs[2]=coeffs[3]=coeffs[5]=coeffs[6]=coeffs[7]=0;

	coeffsInv[0]=coeffsInv[4]=coeffsInv[8]=1;
	coeffsInv[1]=coeffsInv[2]=coeffsInv[3]=coeffsInv[5]=coeffsInv[6]=coeffsInv[7]=0;
}

void PerspectiveTransform::ComputeTransform()
{
	srcPts[0]=from_ll.x;
	srcPts[1]=from_ll.y;
	srcPts[2]=from_lr.x;
	srcPts[3]=from_lr.y;
	srcPts[4]=from_ul.x;
	srcPts[5]=from_ul.y;
	srcPts[6]=from_ur.x;
	srcPts[7]=from_ur.y;

	dstPts[0]=  to_ll.x;
	dstPts[1]=  to_ll.y;
	dstPts[2]=  to_lr.x;
	dstPts[3]=  to_lr.y;
	dstPts[4]=  to_ul.x;
	dstPts[5]=  to_ul.y;
	dstPts[6]=  to_ur.x;
	dstPts[7]=  to_ur.y;

	getNormalizationCoefficients(srcPts, dstPts, false);
	getNormalizationCoefficients(srcPts, dstPts, true);
}

double *PerspectiveTransform::getNormalizationCoefficients(double *src, double *dst, bool isInverse)
{
	double *matX;

	if(isInverse){
		matX=coeffsInv;
		double *tmp = dst;
		dst = src;
		src = tmp;
	} else {
		matX=coeffs;
	}

	double r[8][8] = {
		{src[0], src[1], 1, 0, 0, 0, -1*dst[0]*src[0], -1*dst[0]*src[1]},
		{0, 0, 0, src[0], src[1], 1, -1*dst[1]*src[0], -1*dst[1]*src[1]},
		{src[2], src[3], 1, 0, 0, 0, -1*dst[2]*src[2], -1*dst[2]*src[3]},
		{0, 0, 0, src[2], src[3], 1, -1*dst[3]*src[2], -1*dst[3]*src[3]},
		{src[4], src[5], 1, 0, 0, 0, -1*dst[4]*src[4], -1*dst[4]*src[5]},
		{0, 0, 0, src[4], src[5], 1, -1*dst[5]*src[4], -1*dst[5]*src[5]},
		{src[6], src[7], 1, 0, 0, 0, -1*dst[6]*src[6], -1*dst[6]*src[7]},
		{0, 0, 0, src[6], src[7], 1, -1*dst[7]*src[6], -1*dst[7]*src[7]}
	};

	double **matA = AllocateMatrix(8,8);
	for (int i=0; i<8; i++) {
		memcpy(matA[i], r[i], sizeof(double[8]));
	}

	double *matB = dstPts;
	double **matC=NULL;
	double **Atranspose = Numeric_transpose(8,8, matA, NULL);

	int err=0;

	try {
		double **tempmat = Numeric_dotMMsmall(8,8,8, Atranspose, matA, NULL);
		matC = Numeric_inv(8,8, tempmat);
		DeallocateMatrix(8, tempmat);

	} catch(exception e) {
		err=1;
		cerr << "Error! "<<e.what()<<endl;
		matX[0]=matX[4]=matX[8]=1;
		matX[1]=matX[2]=matX[3]=matX[5]=matX[6]=matX[7]=0;

	} catch(int e) {
		err=1;
		cerr << "Error! "<<e<<endl;
		matX[0]=matX[4]=matX[8]=1;
		matX[1]=matX[2]=matX[3]=matX[5]=matX[6]=matX[7]=0;
	}

	if (!err) {
		double **matD = Numeric_dotMMsmall(8,8,8, matC, Atranspose, NULL);
		Numeric_dotMV(8,8, matD, matB, matX);
		DeallocateMatrix(8, matD);

		//for(var i = 0; i < matX.length; i++) {
		//    matX[i] = round(matX[i]);
		//}

		matX[8] = 1;
	}

	DeallocateMatrix(8, Atranspose);
	DeallocateMatrix(8, matC);
	return matX;
}

flatpoint PerspectiveTransform::transform(double x,double y)
{
	flatpoint p;
	p.x = (this->coeffs[0]*x + this->coeffs[1]*y + this->coeffs[2]) / (this->coeffs[6]*x + this->coeffs[7]*y + 1);
	p.y = (this->coeffs[3]*x + this->coeffs[4]*y + this->coeffs[5]) / (this->coeffs[6]*x + this->coeffs[7]*y + 1);
	return p;
}

flatpoint PerspectiveTransform::transform(flatpoint p)
{
	flatpoint pp;
	pp.x = (this->coeffs[0]*p.x + this->coeffs[1]*p.y + this->coeffs[2]) / (this->coeffs[6]*p.x + this->coeffs[7]*p.y + 1);
	pp.y = (this->coeffs[3]*p.x + this->coeffs[4]*p.y + this->coeffs[5]) / (this->coeffs[6]*p.x + this->coeffs[7]*p.y + 1);
	return pp;
}

flatpoint PerspectiveTransform::transformInverse(double x,double y)
{
	flatpoint p;
	p.x = (this->coeffsInv[0]*x + this->coeffsInv[1]*y + this->coeffsInv[2]) / (this->coeffsInv[6]*x + this->coeffsInv[7]*y + 1);
	p.y = (this->coeffsInv[3]*x + this->coeffsInv[4]*y + this->coeffsInv[5]) / (this->coeffsInv[6]*x + this->coeffsInv[7]*y + 1);
	return p;
}

flatpoint PerspectiveTransform::transformInverse(flatpoint p)
{
	flatpoint pp;
	pp.x = (this->coeffsInv[0]*p.x + this->coeffsInv[1]*p.y + this->coeffsInv[2]) / (this->coeffsInv[6]*p.x + this->coeffsInv[7]*p.y + 1);
	pp.y = (this->coeffsInv[3]*p.x + this->coeffsInv[4]*p.y + this->coeffsInv[5]) / (this->coeffsInv[6]*p.x + this->coeffsInv[7]*p.y + 1);
	return pp;
}




//---------------------------------- End Perspective Transform Utils -----------------------------






////--------------------------- PerspectiveData -------------------------------------
//
///*! \class PerspectiveData
// * \ingroup interfaces
// * \brief Data that PerspectiveInterface can use.
// */
//
//PerspectiveData::PerspectiveData()
//{
//}
//
//PerspectiveData::~PerspectiveData()
//{
//}
//


//--------------------------- PerspectiveInterface -------------------------------------

/*! \class PerspectiveInterface
 * \ingroup interfaces
 * \brief Interface to easily adjust mouse pressure map for various purposes.
 */


PerspectiveInterface::PerspectiveInterface(anInterface *nowner, int nid, Displayer *ndp)
  : anInterface(nowner,nid,ndp)
{
	interface_flags=0;

	hover=PERSP_None;
	showdecs=1;
	needtodraw=1;
	needtoremap=1;

	dataoc=NULL;
	data=NULL;

	sc=NULL; //shortcut list, define as needed in GetShortcuts() 
}

PerspectiveInterface::~PerspectiveInterface()
{
	if (dataoc) delete dataoc;
	if (data) { data->dec_count(); data=NULL; }
	if (sc) sc->dec_count();
}

const char *PerspectiveInterface::whatdatatype()
{ 
	//return "PerspectiveData";
	return NULL; // NULL means this tool is creation only, it cannot edit existing data automatically
}

/*! Name as displayed in menus, for instance.
 */
const char *PerspectiveInterface::Name()
{ return _("Perspective tool"); }


//! Return new PerspectiveInterface.
/*! If dup!=NULL and it cannot be cast to PerspectiveInterface, then return NULL.
 */
anInterface *PerspectiveInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new PerspectiveInterface(NULL,id,NULL);
	else if (!dynamic_cast<PerspectiveInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

//! Use the object at oc if it is an PerspectiveData.
int PerspectiveInterface::UseThisObject(ObjectContext *oc)
{
	if (!oc) return 0;

	//SomeData *ndata=dynamic_cast<PerspectiveData *>(oc->obj);
	SomeData *ndata=oc->obj;
	if (!ndata) return 0;

	if (data && data!=ndata) {
		Clear(NULL);
	}
	if (dataoc) delete dataoc;
	dataoc=oc->duplicate();

	if (data!=ndata) {
		data=ndata;
		data->inc_count();
	}
	
	 //1,2,3,4  ->  ll,lr,ul,ur
	flatpoint v(data->boxwidth()*.1, data->boxheight()*.1);
	transform.from_ll = transform.to_ll = data->transformPoint(flatpoint(data->minx,data->miny)+v);
	transform.from_lr = transform.to_lr = data->transformPoint(flatpoint(data->maxx,data->miny)+flatpoint(-v.x,v.y));
	transform.from_ul = transform.to_ul = data->transformPoint(flatpoint(data->minx,data->maxy)+flatpoint(v.x,-v.y));
	transform.from_ur = transform.to_ur = data->transformPoint(flatpoint(data->maxx,data->maxy)+flatpoint(-v.x,-v.y));
	//----
	//Affine a=ndata->GetTransformToContext(false, 0);
	//transform.from_ll = transform.to_ll = a.transformPoint(flatpoint(data->minx,data->miny));
	//transform.from_lr = transform.to_lr = a.transformPoint(flatpoint(data->maxx,data->miny));
	//transform.from_ul = transform.to_ul = a.transformPoint(flatpoint(data->minx,data->maxy));
	//transform.from_ur = transform.to_ur = a.transformPoint(flatpoint(data->maxx,data->maxy));

	ComputeTransform();


	needtodraw=1;
	return 1;
}

/*! 
 * <pre>
 *  | 00  01  02 | |x|
 *  | 10  11  12 | |y|
 *  | 20  21  22 | |1|
 * </pre>
 */
flatpoint PerspectiveInterface::ComputePoint(double x,double y)
{
	return transform.transform(x,y);
}

//void PerspectiveInterface::SetPoints(flatpoint nfrom_ll, flatpoint nfrom_lr, flatpoint nfrom_ul, flatpoint nfrom_ur,
//									 flatpoint nto_ll,   flatpoint nto_lr,   flatpoint nto_ul,   flatpoint nto_ur)
//{
//	transform.to_ll=nto_ll;
//	transform.to_lr=nto_lr;
//	transform.to_ul=nto_ul;
//	transform.to_ur=nto_ur;
//
//	transform.from_ll=nfrom_ll;
//	transform.from_lr=nfrom_lr;
//	transform.from_ul=nfrom_ul;
//	transform.from_ur=nfrom_ur;
//}

void PerspectiveInterface::ResetTransform()
{
	transform.ResetTransform();

	needtoremap=0;
	needtodraw=1;
}

void PerspectiveInterface::ComputeTransform()
{	
	transform.ComputeTransform(); 
	needtoremap=0;
}

/*! Normally this will accept some common things like changes to line styles, like a current color.
 */
int PerspectiveInterface::UseThis(anObject *nobj, unsigned int mask)
{
//	if (!nobj) return 1;
//	LineStyle *ls=dynamic_cast<LineStyle *>(nobj);
//	if (ls!=NULL) {
//		if (mask&GCForeground) { 
//			linecolor=ls->color;
//		}
////		if (mask&GCLineWidth) {
////			linecolor.width=ls->width;
////		}
//		needtodraw=1;
//		return 1;
//	}
	return 0;
}

/*! Return the object's ObjectContext to make sure that the proper context is already installed
 * before Refresh() is called.
 */
ObjectContext *PerspectiveInterface::Context()
{
	//return NULL;
	return dataoc;
}

/*! Any setup when an interface is activated, which usually means when it is added to 
 * the interface stack of a viewport.
 */
int PerspectiveInterface::InterfaceOn()
{
	showdecs=1;
	needtodraw=1;
	return 0;
}

/*! Any cleanup when an interface is deactivated, which usually means when it is removed from
 * the interface stack of a viewport.
 */
int PerspectiveInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	return 0;
}

void PerspectiveInterface::Clear(SomeData *d)
{
	if (dataoc) { delete dataoc; dataoc=NULL; }
	if (data) { data->dec_count(); data=NULL; }
}

void PerspectiveInterface::ViewportResized()
{
	// if necessary, do stuff in response to the parent window size changed
}

Laxkit::MenuInfo *PerspectiveInterface::ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu)
{
	return menu;

//	if (no menu for x,y) return NULL;
//
//	if (!menu) menu=new MenuInfo;
//	if (!menu->n()) menu->AddSep(_("Some new menu header"));
//
//	menu->AddItem(_("Create raw points"), FREEHAND_Raw_Path, LAX_ISTOGGLE|(istyle&FREEHAND_Raw_Path)?LAX_CHECKED:0);
//	menu->AddItem(_("Some menu item"), SOME_MENU_VALUE);
//	menu->AddSep(_("Some separator text"));
//	menu->AddItem(_("Et Cetera"), SOME_OTHER_VALUE);
//	return menu;
}

int PerspectiveInterface::Event(const Laxkit::EventData *data, const char *mes)
{
//    if (!strcmp(mes,"menuevent")) {
//        const SimpleMessage *s=dynamic_cast<const SimpleMessage*>(e_data);
//        int i =s->info2; //id of menu item
//
//        if ( i==SOME_MENU_VALUE) {
//			...
//		}
//
//		return 0; 
//	}

	return 1; //event not absorbed
}


///*! Draw some data other than the current data.
// * This is called during screen refreshes.
// */
//int PerspectiveInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
//{
//	return 1;
////	if (!ndata || dynamic_cast<PerspectiveData *>(ndata)==NULL) return 1;
////
////	PerspectiveData *bzd=data;
////	data=dynamic_cast<PerspectiveData *>(ndata);
////
////	 // store any other state we need to remember
////	 // and update to draw just the temporary object, no decorations
////	int td=showdecs,ntd=needtodraw;
////	showdecs=0;
////	needtodraw=1;
////
////	Refresh();
////
////	 //now restore the old state
////	needtodraw=ntd;
////	showdecs=td;
////	data=bzd;
////	return 1;
//}


int PerspectiveInterface::Refresh()
{

	if (needtodraw==0) return 0;
	needtodraw=0;

	if (!data) return 0;

	if (needtoremap) ComputeTransform();

	int lines=8;

	Affine a(*data);
	a.Invert();

	dp->PushAndNewTransform(a.m());
	dp->LineWidthScreen(1);

	
	flatpoint from_ll(data->transformPoint(flatpoint(data->minx, data->miny)));
	flatpoint from_lr(data->transformPoint(flatpoint(data->maxx, data->miny)));
	flatpoint from_ul(data->transformPoint(flatpoint(data->minx, data->maxy)));
	flatpoint from_ur(data->transformPoint(flatpoint(data->maxx, data->maxy)));

	flatpoint to_ll=transform.transform(from_ll);
	flatpoint to_lr=transform.transform(from_lr);
	flatpoint to_ul=transform.transform(from_ul);
	flatpoint to_ur=transform.transform(from_ur);
	
	flatpoint l = from_ul - from_ll; //points botfromm from fromp
	flatpoint r = from_ur - from_lr;
	flatpoint t = from_ur - from_ul; //points left from right
	flatpoint b = from_lr - from_ll;
	//------ 
	//flatpoint l = transform.from_ul - transform.from_ll; //points bottom to top
	//flatpoint r = transform.from_ur - transform.from_lr;
	//flatpoint t = transform.from_ur - transform.from_ul; //points left to right
	//flatpoint b = transform.from_lr - transform.from_ll;

	dp->NewFG(.75,.75,.75);
	flatpoint p1,p2;
	for (int c=1; c<lines; c++) {
		p1=transform.transform(from_ll+c/(float)(lines)*l);
		p2=transform.transform(from_lr+c/(float)(lines)*r);
		dp->drawline(p1,p2); //horiz line

		p1=transform.transform(from_ll+c/(float)(lines)*b);
        p2=transform.transform(from_ul+c/(float)(lines)*t);
		dp->drawline(p1,p2); //vert line
		//----
		//p1=transform.transform(transform.from_ll+c/(float)(lines)*l);
		//p2=transform.transform(transform.from_lr+c/(float)(lines)*r);
		//dp->drawline(p1,p2); //horiz line

		//p1=transform.transform(transform.from_ll+c/(float)(lines)*b);
        //p2=transform.transform(transform.from_ul+c/(float)(lines)*t);
		//dp->drawline(p1,p2); //vert line
	}

	dp->NewFG(.75,.75,.75);
	dp->NewBG(0.6,0.6,0.6);
	dp->LineWidthScreen(2);

	//dp->drawline(transform.to_ll, transform.to_lr); //transformed corners
	//dp->drawline(transform.to_lr, transform.to_ur);
	//dp->drawline(transform.to_ur, transform.to_ul);
	//dp->drawline(transform.to_ul, transform.to_ll);
	//----------
	dp->drawline(to_ll, to_lr); //transformed corners
	dp->drawline(to_lr, to_ur);
	dp->drawline(to_ur, to_ul);
	dp->drawline(to_ul, to_ll);

	dp->drawpoint(transform.to_ll, 10, 0); //actual to points
	dp->drawpoint(transform.to_lr, 10, 0);
	dp->drawpoint(transform.to_ul, 10, 0);
	dp->drawpoint(transform.to_ur, 10, 0);

	dp->NewFG(.25,.25,.25);
	dp->LineWidthScreen(1);
	dp->drawpoint(transform.to_ll, 10, hover==PERSP_ll ? 2 : 0);
	dp->drawpoint(transform.to_lr, 10, hover==PERSP_lr ? 2 : 0);
	dp->drawpoint(transform.to_ul, 10, hover==PERSP_ul ? 2 : 0);
	dp->drawpoint(transform.to_ur, 10, hover==PERSP_ur ? 2 : 0);


	dp->PopAxes();
	return 0;
}

/*! Check for clicking down on other objects, possibly changing control to that other object.
 *
 * Return 1 for changed object to another of same type.
 * Return 2 for changed to object of another type (switched tools).
 * Return 0 for nothing found at x,y.
 */
int PerspectiveInterface::OtherObjectCheck(int x,int y,unsigned int state) 
{
	ObjectContext *oc=NULL;
	viewport->FindObject(x,y,NULL,NULL,1,&oc);
	SomeData *obj=NULL;
	if (oc && oc->obj) obj=oc->obj;

	if (obj) { 
		 // found another PerspectiveData to work on.
		 // If this is primary, then it is ok to work on other images, but not click onto
		 // other types of objects.
		UseThisObject(oc); 
		if (viewport) viewport->ChangeObject(oc,0);
		needtodraw=1;
		return 1;

	}

	return 0;
}

int PerspectiveInterface::scan(double x, double y)
{
	double grab=10;

	if (realtoscreen(transform.to_ll).distanceTo(flatpoint(x,y))<grab) return PERSP_ll;
	if (realtoscreen(transform.to_lr).distanceTo(flatpoint(x,y))<grab) return PERSP_lr;
	if (realtoscreen(transform.to_ul).distanceTo(flatpoint(x,y))<grab) return PERSP_ul;
	if (realtoscreen(transform.to_ur).distanceTo(flatpoint(x,y))<grab) return PERSP_ur;

	return 0;
}

int PerspectiveInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d) 
{
	int nhover=scan(x,y);
	if (nhover != PERSP_None) {
		buttondown.down(d->id,LEFTBUTTON, x,y, nhover);

		needtodraw=1;
		return 0;
	}


	
	 // So, was clicked outside current image or on blank space, make new one or find other one.
	int other = OtherObjectCheck(x,y,state);
	if (other==2) return 0; //control changed to some other tool
	if (other==1) {
		buttondown.down(d->id,LEFTBUTTON, x,y, PERSP_Move);
		return 0; //object changed via UseThisObject().. nothing more to do here!
	}

	 //OtherObjectCheck:
	 //  change to other type of object if not primary
	 //  change to other of same object always ok



	needtodraw=1;
	return 0; //return 0 for absorbing event, or 1 for ignoring
}

int PerspectiveInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{
	buttondown.up(d->id,LEFTBUTTON);
	return 0; //return 0 for absorbing event, or 1 for ignoring
}


int PerspectiveInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d)
{
	if (!buttondown.any()) {
		// update any mouse over state

		int nhover=scan(x,y);
		if (nhover!=hover) {
			hover=nhover;
			needtodraw=1;
		}
		return 0;
	}

	 //else deal with mouse dragging...
	if (hover == PERSP_None) return 0;
	
	int lx,ly;
	int h;
	buttondown.move(d->id, x,y, &lx,&ly);
	buttondown.getextrainfo(d->id, LEFTBUTTON, &h);

	flatpoint dv = screentoreal(x,y) - screentoreal(lx,ly);

	if (h == PERSP_ll || h == PERSP_Move) transform.to_ll += dv;
	if (h == PERSP_lr || h == PERSP_Move) transform.to_lr += dv;
	if (h == PERSP_ul || h == PERSP_Move) transform.to_ul += dv;
	if (h == PERSP_ur || h == PERSP_Move) transform.to_ur += dv;

	needtoremap=1;
	needtodraw=1;
	return 0;
}

int PerspectiveInterface::send()
{
//	if (owner) {
//		RefCountedEventData *data=new RefCountedEventData(paths);
//		app->SendMessage(data,owner->object_id,"PerspectiveInterface", object_id);
//
//	} else {
//		if (viewport) viewport->NewData(paths,NULL);
//	}

	return 0;
}

int PerspectiveInterface::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d)
{

	if (!sc) GetShortcuts();
	int action=sc->FindActionNumber(ch,state&LAX_STATE_MASK,0);
	if (action>=0) {
		return PerformAction(action);
	}

	return 1; //key not dealt with, propagate to next interface
}

Laxkit::ShortcutHandler *PerspectiveInterface::GetShortcuts()
{
	if (sc) return sc;
    ShortcutManager *manager=GetDefaultShortcutManager();
    sc=manager->NewHandler(whattype());
    if (sc) return sc;

    //virtual int Add(int nid, const char *nname, const char *desc, const char *icon, int nmode, int assign);

    sc=new ShortcutHandler(whattype());

	//sc->Add([id number],  [key], [mod mask], [mode], [action string id], [description], [icon], [assignable]);
    sc->Add(PERSP_Reset, 'z',0,0, "Reset", _("Reset transform"),NULL,0);

    manager->AddArea(whattype(),sc);
    return sc;
}

/*! Return 0 for action performed, or nonzero for unknown action.
 */
int PerspectiveInterface::PerformAction(int action)
{
	if (action==PERSP_Reset) {
		ResetTransform();
		return 0;
	}

	return 1;
}


} // namespace LaxInterfaces

