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

#include <lax/gradientstrip.h>



namespace Laxkit {


//------------------------------ GradientStrip::GradientSpot ----------------------------------

/*! \class GradientStrip::GradientSpot
 * \ingroup interfaces
 * \brief GradientStrip keeps a stack of these.
 *
 * Keeps position t, and color. The color components are in range [0,0xffff].
 */


/*! If dup, then use a duplicate of col, else inc its count.
 */
GradientStrip::GradientSpot::GradientStrip::GradientSpot(double tt,double ss,Color *col, bool dup)
{
	t=tt;
	nt=0;
	s=ss;
	ns=0;

	if (dup) color = col->duplicate();
	else {
		color = col;
		if (color) color->inc_count();
	}

	 //for compatibility with gimp gradients:
	midpostition=.5; //0..1, is along segment of this point to next
	interpolation=0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition=0; //how to vary the color, a line in rgb or in hsv
}

GradientStrip::GradientSpot::GradientStrip::GradientSpot(double tt,double ss,ScreenColor *col)
{
	t=tt;
	nt=0;
	s=ss;
	ns=0;

	color=newColor(LAX_COLOR_RGB, col);

	 //for compatibility with gimp gradients:
	midpostition=.5; //0..1, is along segment of this point to next
	interpolation=0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition=0; //how to vary the color, a line in rgb or in hsv
}

/*! rr,gg,bb,aa in range [0..1]
 */
GradientStrip::GradientSpot::GradientStrip::GradientSpot(double tt,double ss, double rr,double gg,double bb,double aa)
{
	t=tt;
	nt=0;
	s=ss;
	ns=0;

	ScreenColor scolor(rr,gg,bb,aa);
	color=newColor(LAX_COLOR_RGB, &scolor);

	 //for compatibility with gimp gradients:
	midpostition=.5; //0..1, is along segment of this point to next
	interpolation=0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition=0; //how to vary the color, a line in rgb or in hsv
}
	
//! Dump in an attribute, then call dump_in_atts(thatatt,0,context).
/*! If Att!=NULL, then return the attribute used to read in the stuff.
 * This allows
 * holding classes to have extra attributes within the spot field to
 * exist and not be discarded.
 *
 * \todo *** allow import of Gimp, Inkscape/svg, scribus gradients
 */
void GradientStrip::GradientSpot::dump_in(FILE *f,int indent,LaxFiles::DumpContext *context, Attribute **Att)
{
	Attribute *att=new Attribute;
	att->dump_in(f,indent);
	dump_in_atts(att,0,context);
	if (Att) *Att=att;
	else delete att;
}

//! Fill the t, red, green, blue, alpha, based on the corresponding attributes.
void GradientStrip::GradientSpot::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	int c,c2=0;
	char *value,*name;

	for (c=0; c<att->attributes.n; c++) { 
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"t")) {
			DoubleAttribute(value,&t);

		} else if (!strcmp(name,"nt")) {
			DoubleAttribute(value,&nt);

		} else if (!strcmp(name,"s")) {
			DoubleAttribute(value,&s);

		} else if (!strcmp(name,"ns")) {
			DoubleAttribute(value,&ns);

		} else if (!strcmp(name,"midpoint")) {
			DoubleAttribute(value,&midposition);

		} else if (!strcmp(name,"interpolation")) {
			if (!strcmp(value, "linear")) interpolation = 0;
			else if (!strcmp(value, "curved")) interpolation = 1;
			else if (!strcmp(value, "sinusoidal")) interpolation = 2;
			else if (!strcmp(value, "sphere_inc")) interpolation = 3;
			else if (!strcmp(value, "sphere_dec")) interpolation = 4;

		} else if (!strcmp(name,"transition")) {
			transition = BooleanValue(value);

		} else if (!strcmp(name,"color")) {
			***

			if (!strcmp(name,"rgba")) {
				int i[4];
				c2=IntListAttribute(value,i,4);
				DBG if (c2!=4) cerr <<"---gradient spot not right number of color components!!"<<endl;
				for (int c3=0; c3<c2; c3++) {
					if (c3==0) color.red=i[c3];
					else if (c3==1) color.green=i[c3];
					else if (c3==2) color.blue=i[c3];
					else if (c3==3) color.alpha=i[c3];
				}
			}


		}
	}

	DBG cerr <<"spot out:"<<endl;
	DBG dump_out(stderr,2,0,NULL);
}

/*! If what==-1, then dump out a psuedocode mockup of what gets dumped.
 * Otherwise dumps out in indented data format described above.
 */
void GradientStrip::GradientSpot::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%st 1     #the spot on the x axis to place the color, customarily the spots will\n",spc);
		fprintf(f,"%s        #cover the whole range [0..1] but that range is not mandatory\n",spc);
		fprintf(f,"%snt 1    #like t, but normalized to the range [0..1]. Recomputed on reading in GradientStrip\n",spc);
		fprintf(f,"%ss 1     #the spot on the y axis to place the color, customarily the spots will\n",spc);
		fprintf(f,"%s        #cover the whole range [0..1] but that range is not mandatory\n",spc);
		fprintf(f,"%sns 1    #like s, but normalized to the range [0..1]. Recomputed on reading in GradientStrip\n",spc);
		fprintf(f,"%scolor ...  #like rgbaf(1,1,1,1) or grayf(.5)\n",spc);
		return;
	}

	fprintf(f,"%st  %.10g\n",spc,t);
	fprintf(f,"%snt %.10g\n",spc,nt);
	fprintf(f,"%ss  %.10g\n",spc,s);
	fprintf(f,"%sns %.10g\n",spc,ns);

	fprintf(f,"%smidpoint %.10g\n",spc,midposition);

	if (interpolation==0) fprintf("%sinterpolation linear\n",spc);
	else if (interpolation==1) fprintf("%sinterpolation curved\n",spc);
	else if (interpolation==2) fprintf("%sinterpolation sinusoidal\n",spc);
	else if (interpolation==3) fprintf("%sinterpolation sphere_inc\n",spc);
	else if (interpolation==4) fprintf("%sinterpolation sphere_dec\n",spc);

	fprintf(f,"%stransition %s", spc, transition ? "rgb" : "hsv");

	fprintf(f,"%scolor\n",spc);
	color->dump_out(f,indent+2,what,context);
}



//--------------------------------- GradientStrip ----------------------------

/*! \class GradientStrip
 *
 * Class to hold a sequential arrangement of colors. Used by GradientStrip.
 * GradientStrip does not specify whether linear or radial, and can be used for Palettes also.
 */



//! Create new basic gradient pp1 to pp2. Sets col1 at 0 and col2 at 1
/*! This just passes everything to Set().
 */
GradientStrip::GradientStrip(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2,unsigned int stle)
{ ***
	name=file=NULL;
	num_columns_hint=0;
	width=height=0;

	Set(pp1,pp2,rr1,rr2,col1,col2,stle);
	usepreview=1;
}

GradientStrip::~GradientStrip()
{
}

void GradientStrip::touchContents()
{
	cerr << "*** GradientStrip::touchContents(): implement me!!"<<endl;
}

SomeData *GradientStrip::duplicate(SomeData *dup)
{ ***
	GradientStrip *g=dynamic_cast<GradientStrip*>(dup);
	if (!g && !dup) return NULL; //was not GradientStrip!

	char set=1;
	if (!dup) {
		dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_GRADIENTDATA));
		if (dup) {
			dup->setbounds(minx,maxx,miny,maxy);
			//set=0;
			g=dynamic_cast<GradientStrip*>(dup);
		}
	} 
	if (!g) {
		g=new GradientStrip();
		dup=g;
	}
	if (set) {
		g->radial=radial;
		g->style=style;
		g->p1=p1;
		g->p2=p2;
		g->r1=r1;
		g->r2=r2;
		g->a=a;

		for (int c=0; c<colors.n; c++) {
			g->colors.push(new GradiantStrip::GradientSpot(colors.e[c]->t,&colors.e[c]->color));
		}
	}

	 //somedata elements:
	dup->bboxstyle=bboxstyle;
	dup->m(m());
	return dup;
}

//! Set so gradient is pp1 to pp2. Sets col1 at 0 and col2 at 1.
void GradientStrip::Set(flatpoint pp1,flatpoint pp2,double rr1,double rr2,
			ScreenColor *col1,ScreenColor *col2,unsigned int stle)
{ ***
	if (norm(pp2-pp1)<1e-5) {
		xaxis(flatpoint(1,0));
		yaxis(flatpoint(0,1));
		p1=0;
		p2=0;

	} else {
		xaxis(pp2-pp1);
		yaxis(transpose(pp2-pp1));
		p1=0;
		p2=1;
	}
	origin(pp1);

	r1=rr1;
	r2=rr2;
	a=0;

	style=stle; 
	if (style&GRADIENT_RADIAL) radial=true; else radial=false;
	colors.flush();
	if (col1) colors.push(new GradiantStrip::GradientSpot(0,col1));	
	if (col2) colors.push(new GradiantStrip::GradientSpot(1,col2)); 

	touchContents();

	DBG cerr <<"new GradientStrip:Set"<<endl;
	DBG dump_out(stderr,2,0,NULL);
}


/*! Reads in from something like:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  p1 0
 *  p2 1
 *  r1 0
 *  r2 10
 *  spot
 *    t 0
 *    rgba 255 100 50 255
 *  spot
 *    t 1
 *    rgba 0 0 0 0
 *  radial
 * </pre>
 */
void GradientStrip::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{ ***
	if (!att) return;
	char *name,*value,*e;

	SomeData::dump_in_atts(att,flag,context);

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"p1")) {
			DoubleAttribute(value,&p1,&e);

		} else if (!strcmp(name,"p2")) {
			DoubleAttribute(value,&p2,&e);

		} else if (!strcmp(name,"r1")) {
			DoubleAttribute(value,&r1,&e);

		} else if (!strcmp(name,"r2")) {
			DoubleAttribute(value,&r2,&e);

		//} else if (!strcmp(name,"a")) {
		//	DoubleAttribute(value,&a,&e);

		} else if (!strcmp(name,"spot")) {
			GradiantStrip::GradientSpot *spot=new GradiantStrip::GradientSpot(0,0,0,0,0);
			spot->dump_in_atts(att->attributes.e[c],flag,context);
			colors.push(spot);

		} else if (!strcmp(name,"linear")) {
			if (BooleanAttribute(value)) radial=false;
			else radial=true;

		} else if (!strcmp(name,"radial")) {
			if (!BooleanAttribute(value)) radial=false;
			else radial=true;
		}
	}
	a=0;
	touchContents();

	FindBBox();
}

/*! \ingroup interfaces
 * Dump out a GradientStrip. Prints matrix, p, v, and the spots.
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped. This makes it very easy
 * for programs to keep track of their file formats, that is, when the programmers remember to
 * update this code as change happens.
 * Otherwise dumps out in indented data format as described in dump_in_atts().
 */
void GradientStrip::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{ ***
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%s#Gradients lie on the x axis from p1 to p2\n",spc);
		fprintf(f,"%smatrix 1 0 0 1 0 0  #the affine transform affecting this gradient\n",spc);
		fprintf(f,"%sp1 0    #the starting x coordinate\n",spc);
		fprintf(f,"%sp2 1    #the ending x coordinate\n",spc);
		fprintf(f,"%sr1 0    #the starting radius (radial) or the +y extent (linear)\n",spc);
		fprintf(f,"%sr2 0    #the ending radius (radial) or the -y extent (linear)\n",spc);
		//fprintf(f,"%sa  0    #an offset to place the color controls of the gradient spots\n",spc);
		fprintf(f,"%sradial  #Specifies a radial gradient\n",spc);
		fprintf(f,"%slinear  #Specifies a linear gradient\n",spc);
		fprintf(f,"%sspot    #There will be at least two gradient data spots, such as this:\n",spc);
		if (colors.n) colors.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			GradiantStrip::GradientSpot g(0,0,30000,65535,65535);
			g.dump_out(f,indent+2,-1,NULL);
		}
		//colors.e[colors.n-1]->dump_out(f,indent+2,-1);//*** should probably check that there are always 2 and not ever 0!
		return;
	}

	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",
			spc,m(0),m(1),m(2),m(3),m(4),m(5));
	fprintf(f,"%sp1 %.10g\n",spc,p1);
	fprintf(f,"%sp2 %.10g\n",spc,p2);
	fprintf(f,"%sr1 %.10g\n",spc,r1);
	fprintf(f,"%sr2 %.10g\n",spc,r2);
	//fprintf(f,"%sa %.10g\n",spc,a);
	fprintf(f,"%s%s\n",spc, radial ? "radial" : "linear");

	for (int c=0; c<colors.n; c++) {
		fprintf(f,"%sspot #%d\n",spc,c);
		colors.e[c]->dump_out(f,indent+2,0,context);
	}
}

/*! If set, then make the t values be the normalized values.
 */
void GradientStrip::UpdateNormalized(bool set)
{
	if (!colors.n) return;

	double min=colors.e[colors.n]->t, max=colors.e[colors.n]->t;

	for (int c=1; c<colors.n; c++) {
		if (colors.e[c]->t<min) min=colors.e[c]->t;
		if (colors.e[c]->t>max) max=colors.e[c]->t;
	}

	double len=max-min;
	if (len==0) {
		for (int c=0; c<colors.n; c++) colors.e[c]->nt = 0;
	} else {
		for (int c=0; c<colors.n; c++) {
			colors.e[c]->nt = (colors.e[c]->t-min)/len;
		}
	}

	if (set) for (int c=0; c<colors.n; c++) colors.e[c]->t = colors.e[c]->nt;
}

/*! Return the t of colors.e[i] mapped to the range [0..1] where
 * 0 is colors.e[0] and 1 is colors.e[colors.n-1].
 *
 * i out of range returns -1.
 */
double GradientStrip::GetNormalizedT(int i)
{
	if (i<0 || i>=colors.n) return -1;
	return (colors.e[i]->t - colors.e[0]->t)/(colors.e[colors.n-1]->t - colors.e[0]->t);
}

int GradientStrip::NumColors()
{
	return colors.n;
}

GradientSpot *GradientStrip::GetColor(int index)
{
	if (index<0 || index>=colors.n) return NULL;
	return colors.e[index];
}

/*! Move the color which to a new t position, rearranging colors stack position if necessary.
 *
 *  Note this is the GradientStrip independent t value.
 */
int GradientStrip::ShiftPoint(int which,double dt)
{
	if (which<0 || which>=colors.n) return which;
	colors.e[which]->t+=dt;
	GradiantStrip::GradientSpot *tmp=colors.e[which];

	while (which>0 && tmp->t<colors.e[which-1]->t) {
		colors.e[which]=colors.e[which-1];
		which--;
		colors.e[which]=tmp;
	}
	while (which<colors.n-1 && tmp->t>colors.e[which+1]->t) {
		colors.e[which]=colors.e[which+1];
		which++;
		colors.e[which]=tmp;
	}

	touchContents();
	return which;
}

//! Flip the order of the colors.
void GradientStrip::FlipColors()
{
	GradiantStrip::GradientSpot *tt;
	double tmax=colors.e[colors.n-1]->t, tmin=colors.e[0]->t;

	for (int c=0; c<colors.n; c++) {
		colors.e[c]->t =tmax - (colors.e[c]->t-tmin);
		colors.e[c]->nt=   1 - colors.e[c]->nt;
	}

	for (int c=0; c<colors.n/2; c++) {
		tt=colors.e[c];
		colors.e[c]=colors.e[colors.n-c-1];
		colors.e[colors.n-c-1]=tt;
	}
	touchContents();
}

//! Takes pointer, does not make duplicate.
int GradientStrip::AddColor(GradiantStrip::GradientSpot *spot)
{
	int c=0;
	while (c<colors.n && spot->t>colors.e[c]->t) c++;
	colors.push(spot,1,c);

	DBG cerr <<"Gradient add color to place"<<c<<endl;
	touchContents();
	return c;
}

//! Add a spot with the given color, or interpolated, if col==NULL.
int GradientStrip::AddColor(double t,ScreenColor *col)
{ ***
	if (col) return AddColor(t,col->red,col->green,col->blue,col->alpha);
	ScreenColor c;
	WhatColor(t,&c);
	touchContents();
	return AddColor(t,c.red,c.green,c.blue,c.alpha);
}
	
//! Place new color in right spot in list.
/*! The color components are in range [0,0xffff].
 */
int GradientStrip::AddColor(double t,int red,int green,int blue,int alpha)
{ ***
	int c=0;
	if (t<colors.e[0]->t) {
		 // move p1
		double clen=colors.e[colors.n-1]->t-colors.e[0]->t;
		p1-=(colors.e[0]->t-t)/clen*(p2-p1);
	} else if (t>colors.e[colors.n-1]->t) {
		 // move p2
		double clen=colors.e[colors.n-1]->t-colors.e[0]->t;
		p2-=(colors.e[colors.n-1]->t-t)/clen*(p2-p1);
	}

	while (c<colors.n && t>colors.e[c]->t) c++;
	GradiantStrip::GradientSpot *gds=new GradiantStrip::GradientSpot(t,red,green,blue,alpha);
	colors.push(gds,1,c);
	
	DBG cerr <<"Gradient add color "<<c<<endl;

	touchContents();
	return c;
}

/*! From coordinate in data space, return the color at it.
 * Return 0 for success, or nonzero for coordinate out of range.
 */
int GradientStrip::WhatColor(flatpoint p, Laxkit::ScreenColor *col)
{ ***
	double x=p.x;
	double y=p.y;
	
	if (!radial) {
		 //linear gradient, much easier
		if (r1>r2) {
			if (y>r1 || y<r2) return 1; //out of y bounds
		} else if (y<r1 || y>r2) return 2;

		if (p1<p2) {
			if (x<p1 || x>p2) return 3;
		} else if (x>p1 || x<p2) return 4;

		return WhatColor(colors.e[0]->t + (colors.e[colors.n-1]->t-colors.e[0]->t)*(x-p1)/(p2-p1), col);
	}
	
	 //else radial gradient
//	***
//	if (p2+r2<=p1+r1 && p2-r2>=p1-r1) {
//		 //circle 2 is entirely contained in circle 1
//	} else if (p2+r2>=p1+r1 && p2-r2<=p1-r1) {
//		 //circle 1 is entirely contained in circle 2
//	}
	 // ***** HACK! just looks in plane circle 2 radius centered at p2
	return WhatColor(colors.e[0]->t + (colors.e[colors.n-1]->t-colors.e[0]->t)*norm(p-flatpoint(p2,0))/r2, col);
}

//! Figure out what color lays at coordinate t.
/*! If t is before the earliest point then the earliest point is used
 * for the color, and -1 is returned. Similarly for beyond the final point, but
 * 1 is returned. Otherwise, the color is linearly interpolated between
 * the nearest points, and 0 is returned.
 */
int GradientStrip::WhatColor(double t,ScreenColor *col)
{ ***
	int c=0;
	while (c<colors.n && t>colors.e[c]->t) c++;
	if (c==0) { *col=colors.e[0]->color; return -1; }
	if (c==colors.n)  { *col=colors.e[colors.n-1]->color; return 1; }

	ScreenColor *c1=&colors.e[c-1]->color,
				 *c2=&colors.e[c]->color;
	t=(t-colors.e[c-1]->t)/(colors.e[c]->t-colors.e[c-1]->t);
	col->red  = (unsigned short) (t*c2->red   + (1-t)*c1->red);
	col->green= (unsigned short) (t*c2->green + (1-t)*c1->green);
	col->blue = (unsigned short) (t*c2->blue  + (1-t)*c1->blue);
	col->alpha= (unsigned short) (t*c2->alpha + (1-t)*c1->alpha);
	return 0;
}

//! Figure out what color lays at coordinate t.
/*! If t is before the earliest point then the earliest point is used
 * for the color, and -1 is returned. Similarly for beyond the final point, but
 * 1 is returned. Otherwise, the color is linearly interpolated between
 * the nearest points, and 0 is returned.
 *
 * The colors are returned as doubles in range [0..1]. It is assumed that col
 * has as many channels as needed for color (with alpha). For most cases,
 * rgba (4 channels, so a double[4]) is sufficient.
 *
 * \todo warning: assumes argb for now... ultimately, should be arranged
 *   according to the color system of the colors
 */
int GradientStrip::WhatColor(double t,double *col)
{ ***
	int c=0;
	while (c<colors.n && t>colors.e[c]->t) c++;
	if (c==0) {
		col[0]= (double) (colors.e[0]->color.alpha) /65535;
		col[1]= (double) (colors.e[0]->color.red)   /65535;
		col[2]= (double) (colors.e[0]->color.green) /65535;
		col[3]= (double) (colors.e[0]->color.blue)  /65535;
		return -1; 
	}
	if (c==colors.n)  {
		col[0]= (double) (colors.e[colors.n-1]->color.alpha) /65535;
		col[1]= (double) (colors.e[colors.n-1]->color.red)   /65535;
		col[2]= (double) (colors.e[colors.n-1]->color.green) /65535;
		col[3]= (double) (colors.e[colors.n-1]->color.blue)  /65535;
		return 1; 
	}
	ScreenColor *c1=&colors.e[c-1]->color,
				 *c2=&colors.e[c]->color;
	t=(t-colors.e[c-1]->t)/(colors.e[c]->t-colors.e[c-1]->t);

	col[0]= (double) (t*c2->alpha + (1-t)*c1->alpha) /65535;
	col[1]= (double) (t*c2->red   + (1-t)*c1->red)   /65535;
	col[2]= (double) (t*c2->green + (1-t)*c1->green) /65535;
	col[3]= (double) (t*c2->blue  + (1-t)*c1->blue)  /65535;

	return 0;
}

//! Render the whole gradient to a buffer.
/*! The entire buffer maps to the gradient's bounding box.
 *
 * bufchannels must be the same number of channels as the number of channels of the colors of the gradient.
 * The last channel is assumed to be the alpha channel.
 * bufstride is the number of bytes each row takes.
 * bufdepth can be either 8 or 16.
 *
 * Currently not antialiased. Please note this is mainly for generating preview images 
 * for use on screen. 16 bit stuff should really
 * be implented with a Displayer capable of 16 bit buffers and transforms.
 *
 * Return 0 for success, or nonzero for error at some point.
 *
 * \todo must rethink about rendering to buffers! must be able to handle 16 bit per channel buffers,
 *   but to be effective, this really means being able to handle arbitrary transformations, which in turn
 *   says what actually has to be rendered...
 * \todo *** rendering radial gradients VERY inefficient here..
 * \todo radial draw assumes argb
 */
int GradientStrip::renderToBuffer(unsigned char *buffer, int bufw, int bufh, int bufstride, int bufdepth, int bufchannels)
{ ***
	DBG cerr <<"...GradientStrip::renderToBuffer()"<<endl;

	int i=0;
	int numchan=4; //***
	if (bufchannels!=numchan) return 1;
	int c,y,x;
	bufdepth/=8;
	if (bufdepth!=1 && bufdepth!=2) return 2;
	if (bufstride==0) bufstride=bufw*bufchannels*bufdepth;

	memset(buffer, 0, bufstride*bufh*bufdepth);

	double color[numchan];
	int tempcol;

	if (!radial) {
		 //linear gradient, easy!
		for (i=0,x=0; x<bufw; x++) {
			WhatColor(colors.e[0]->t+((double)x/bufw)*(colors.e[colors.n-1]->t-colors.e[0]->t), color);
			for (c=0; c<numchan; c++) { //apparently in byte order, it goes bgra
				if (bufdepth==1) {
					buffer[i]=(unsigned char)(color[3-c]*255+.5);
					//if (c==3) buffer[i]=128; else buffer[i]=255;
					i++;
				} else {
					tempcol=(int)(color[c]*65535+.5);
					buffer[i]=(tempcol&0xff00)>>8;
					i++;
					buffer[i]=(tempcol&0xff);
					i++;
				}
			}
		}
		 //now copy that row for each of the other rows
		 //*** this could be slightly sped up by copying the 1st row, then copying those 2 rows, 
		 //then those 4 rows, etc, rather than do one by one
		for (i=bufstride,y=1; y<bufh; y++, i+=bufstride) {
			memcpy(buffer+i, buffer, bufstride);//dest,src,n
		}
		return 0;
	}


	 //--- else is radial gradient
	double scalex=bufw/(maxx-minx);
	double px,py;
	double cp,O1,O2,o1,o2,v;
	double R1,R2,r1x,r2x,r,ry,cstart,clen;
	ScreenColor col,col0,col1;
	int len,c2,c3;
	int ell; //number of points to approximate circles with

	O1=(p1-minx)*scalex;
	O2=(p2-minx)*scalex;
	cstart=colors.e[0]->t;
	clen=colors.e[colors.n-1]->t - cstart;

	R1=r1;
	R2=r2;

	 //for each color segment...
	for (c=0; c<colors.n-1; c++) {
		o1=O1+(O2-O1)*(colors.e[c  ]->t-cstart)/clen; //this color segment's start and end centers
		o2=O1+(O2-O1)*(colors.e[c+1]->t-cstart)/clen;
		r1x=scalex * (R1+(R2-R1)*(colors.e[c  ]->t-cstart)/clen);//segment's start and end radii
		r2x=scalex * (R1+(R2-R1)*(colors.e[c+1]->t-cstart)/clen);
		v=fabs(o2-o1);

		col0.red  =colors.e[ c ]->color.red;
		col0.green=colors.e[ c ]->color.green;
		col0.blue =colors.e[ c ]->color.blue;
		col0.alpha=colors.e[ c ]->color.alpha;
		col1.red  =colors.e[c+1]->color.red;
		col1.green=colors.e[c+1]->color.green;
		col1.blue =colors.e[c+1]->color.blue;
		col1.alpha=colors.e[c+1]->color.alpha;

		//len=(int)((v+fabs(r1x-r2x))*1.4); //the number of circles to draw so as to have no gaps hopefully
		len=(int)((v+fabs(r1x-r2x))*2); //the number of circles to draw so as to have no gaps hopefully
		for (c2=0; c2<len; c2++) {
			cp=o1+v*((float)c2/len); //center of current circle
			r=r1x+(float)c2/len*(r2x-r1x); //radius of current circle
			ry=r*bufh/bufw/(maxy-miny)*(maxx-minx);
			coloravg(&col,&col0,&col1,(float)c2/len);

			ell=(int)(2*M_PI*r*2);
			for (c3=0; c3<ell; c3++) { 
				px=(int)(cp     +  r*cos((float)c3/(ell-1)*2*M_PI) + .5);
				py=(int)(bufh/2 + ry*sin((float)c3/(ell-1)*2*M_PI) + .5);

				//DBG cerr <<"render radial: p:("<<px<<","<<py<<") r="<<r<<endl;
				//DBG if (px<0 || px>=bufw || py<0 || py>=bufh) cerr <<" ********* Warning! gradient render out of bounds!!"<<endl;

				if (px<0) px=0; else if (px>=bufw) px=bufw-1;
				if (py<0) py=0; else if (py>=bufh) py=bufh-1;

				i=py*bufstride + px*bufchannels*bufdepth;

				 //put in buffer
				if (bufdepth==1) { //8bit per channel
					buffer[i++]=(unsigned char)((col.blue &0xff00)>>8);
					buffer[i++]=(unsigned char)((col.green&0xff00)>>8);
					buffer[i++]=(unsigned char)((col.red  &0xff00)>>8);
					buffer[i++]=(unsigned char)((col.alpha&0xff00)>>8);
				} else { //16bit per channel
					buffer[i++]=(unsigned char)((col.alpha&0xff00)>>8);
					buffer[i++]=(unsigned char) (col.alpha&0xff);
					buffer[i++]=(unsigned char)((col.red  &0xff00)>>8);
					buffer[i++]=(unsigned char) (col.red  &0xff);
					buffer[i++]=(unsigned char)((col.green&0xff00)>>8);
					buffer[i++]=(unsigned char) (col.green&0xff);
					buffer[i++]=(unsigned char)((col.blue &0xff00)>>8);
					buffer[i++]=(unsigned char) (col.blue &0xff);
				}
			}
		}
	}

	return 1;
}

} //namespace Laxkit

