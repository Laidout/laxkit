//
//	
//    The Laxkit, a windowing toolkit
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 3 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>.
//
//    Copyright (C) 2016 by Tom Lechner
//

#include <lax/gradientstrip.h>
#include <lax/displayer.h>
#include <lax/attributes.h>
#include <lax/utf8string.h>
#include <lax/language.h>


#include <iostream>
#define DBG
using namespace std;


namespace Laxkit {


//------------------------------ GradientStrip::GradientSpot ----------------------------------

/*! \class GradientStrip::GradientSpot
 * \ingroup interfaces
 * \brief GradientStrip keeps a stack of these.
 *
 * Keeps position t, and color. The color components are in range [0,0xffff].
 */


/*! If dup_color, then dup the color in spot. Else just link.
 */
GradientStrip::GradientSpot::GradientSpot(GradientStrip::GradientSpot *spot, bool dup_color)
{
	flags = 0;
	name = NULL;

	 //for compatibility with gimp gradients:
	midposition   =.5; //0..1, is along segment of this point to next
	interpolation = 0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition    = 0; //how to vary the color, a line in rgb or in hsv

	if (spot) {
		t  = spot->t;
		nt = spot->nt;
		s  = spot->s;
		ns = spot->ns;
	} else {
		t  = 0;
		nt = 0;
		s  = 0;
		ns = 0;
	}

	if (spot) {
		if (dup_color && spot->color) color = spot->color->duplicate();
		else {
			color = spot->color;
			if (color) color->inc_count();
		}

		 //for compatibility with gimp gradients:
		midposition  =spot->midposition ;
		interpolation=spot->interpolation;
		transition   =spot->transition   ;

	} else {
		ScreenColor scolor(0,0,0,1);
		color = ColorManager::newColor(LAX_COLOR_RGB, &scolor);
	}

}

/*! If dup, then use a duplicate of col, else inc its count.
 */
GradientStrip::GradientSpot::GradientSpot(double tt,double ss,Color *col, bool dup)
{
	flags = 0;
	name = NULL;

	 //for compatibility with gimp gradients:
	midposition   =.5; //0..1, is along segment of this point to next
	interpolation = 0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition    = 0; //how to vary the color, a line in rgb or in hsv

	t  = tt;
	nt = 0;
	s  = ss;
	ns = 0;

	if (col) {
		if (dup) color = col->duplicate();
		else {
			color = col;
			color->inc_count();
		}
	}
	else {
		ScreenColor scolor(0,0,0,1);
		color = ColorManager::newColor(LAX_COLOR_RGB, &scolor);
	}
}

GradientStrip::GradientSpot::GradientSpot(double tt,double ss,ScreenColor *col)
{
	flags = 0;
	name = NULL;

	t  = tt;
	nt = 0;
	s  = ss;
	ns = 0;

	color = ColorManager::newColor(LAX_COLOR_RGB, col);

	 //for compatibility with gimp gradients:
	midposition   = .5; //0..1, is along segment of this point to next
	interpolation = 0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition    = 0; //how to vary the color, a line in rgb or in hsv
}

/*! rr,gg,bb,aa in range [0..1]
 */
GradientStrip::GradientSpot::GradientSpot(double tt,double ss, double rr,double gg,double bb,double aa)
{
	flags = 0;
	name = NULL;

	t  = tt;
	nt = 0;
	s  = ss;
	ns = 0;

	ScreenColor scolor(rr,gg,bb,aa);
	color = ColorManager::newColor(LAX_COLOR_RGB, &scolor);

	 //for compatibility with gimp gradients:
	midposition   = .5; //0..1, is along segment of this point to next
	interpolation = 0; //like gimp? 0=linear, 1=curved, 2=sinusoidal, 3=sphere inc, 4=sphere dec
	transition    = 0; //how to vary the color, a line in rgb or in hsv
}
	
GradientStrip::GradientSpot::~GradientSpot()
{
	delete[] name;
	if (color) color->dec_count();
}

//! Dump in an attribute, then call dump_in_atts(thatatt,0,context).
/*! If Att!=NULL, then return the attribute used to read in the stuff.
 * This allows
 * holding classes to have extra attributes within the spot field to
 * exist and not be discarded.
 */
void GradientStrip::GradientSpot::dump_in(FILE *f,int indent,DumpContext *context, Attribute **Att)
{
	Attribute *att=new Attribute;
	att->dump_in(f,indent);
	dump_in_atts(att,0,context);
	if (Att) *Att=att;
	else delete att;
}

//! Fill the t, red, green, blue, alpha, based on the corresponding attributes.
void GradientStrip::GradientSpot::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	char *value,*name;

	for (int c=0; c<att->attributes.n; c++) { 
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"name")) {
			makestr(this->name, value);

		} else if (!strcmp(name,"t")) {
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
			transition = BooleanAttribute(value);

		} else if (!strcmp(name,"color")) {
			Color *ncolor = ColorManager::newColor(att->attributes.e[c]);
			if (ncolor) {
				if (color) color->dec_count();
				color = ncolor;
			} 

		} else if (!strcmp(name,"rgba")) { //note: this is only for Laidout .097 and below
			double vv[4];
			int nc = DoubleListAttribute(value, vv, 4);
			if (nc >= 3) {
				if (nc == 3) vv[3] = 65535;
				for (int c2=0; c2<4; c2++) vv[c2] /= 65535.0;
				
				Color *ncolor = ColorManager::newColor(LAX_COLOR_RGB, 4, vv[0], vv[1], vv[2], vv[3]);
				if (ncolor) {
					if (color) color->dec_count();
					color = ncolor;
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
void GradientStrip::GradientSpot::dump_out(FILE *f,int indent,int what,DumpContext *context)
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

	if (!isblank(name)) fprintf(f, "%sname %s\n",spc, name);

	fprintf(f,"%st  %.10g\n",spc,t);
	fprintf(f,"%snt %.10g\n",spc,nt);
	fprintf(f,"%ss  %.10g\n",spc,s);
	fprintf(f,"%sns %.10g\n",spc,ns);

	if (flags & Gimp_Spots) {
		fprintf(f,"%smidpoint %.10g\n",spc,midposition);

		if (interpolation==0) fprintf(f,"%sinterpolation linear\n",spc);
		else if (interpolation==1) fprintf(f,"%sinterpolation curved\n",spc);
		else if (interpolation==2) fprintf(f,"%sinterpolation sinusoidal\n",spc);
		else if (interpolation==3) fprintf(f,"%sinterpolation sphere_inc\n",spc);
		else if (interpolation==4) fprintf(f,"%sinterpolation sphere_dec\n",spc);

		fprintf(f,"%stransition %s", spc, transition ? "rgb" : "hsv");
	}

	int n=0;
	n = color->dump_out_simple_string(nullptr, n);
	char *str = new char[n];
	color->dump_out_simple_string(str, n);

	fprintf(f,"%scolor %s\n",spc, str);
	delete[] str;

	//color->dump_out(f,indent+2,what,context);
}

/*! If what==-1, then dump out a psuedocode mockup of what gets dumped.
 * Otherwise dumps out in indented data format described above.
 */
Attribute *GradientStrip::GradientSpot::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("name", "Human Readable", "optional name for this spot");
		att->push("t", "0", "the spot on the x axis to place the color, customarily the spots will "
					        "cover the whole range [0..1] but that range is not mandatory");
		att->push("nt","0", "like t, but normalized to the range [0..1]. Recomputed on reading in GradientStrip");
		att->push("s", "1", "the spot on the y axis to place the color, customarily the spots will "
		                    "cover the whole range [0..1] but that range is not mandatory");
		att->push("ns","1", "like s, but normalized to the range [0..1]. Recomputed on reading in GradientStrip");
		att->push("color", "rgba(1,1,1,1)", "for instance grayf(.5)");
		return att;
	}

	if (!isblank(name)) att->push("name", name);

	att->push("t" ,t);
	att->push("nt",nt);
	att->push("s" ,s);
	att->push("ns",ns);

	if (flags & Gimp_Spots) {
		att->push("midpoint",midposition);

		if      (interpolation==0) att->push("interpolation", "linear");
		else if (interpolation==1) att->push("interpolation", "curved");
		else if (interpolation==2) att->push("interpolation", "sinusoidal");
		else if (interpolation==3) att->push("interpolation", "sphere_inc");
		else if (interpolation==4) att->push("interpolation", "sphere_dec");

		att->push("transition", transition ? "rgb" : "hsv");
	}

	int n=0;
	n = color->dump_out_simple_string(nullptr, n);
	char *str = new char[n];
	color->dump_out_simple_string(str, n);

	att->push("color", str);
	delete[] str;

	//Attribute *att2 = att->pushSubAtt("color");
	//color->dump_out_atts(att2,what,context);

	return att;
}



//--------------------------------- GradientStrip ----------------------------

/*! \class GradientStrip
 *
 * Class to hold a sequential arrangement of colors.
 * GradientStrip doesn't have to specify whether linear or radial, and can be used for Palettes also.
 */


/*! Create new blank gradient. If init, install colors white to black, 0..1. Else create with no colors.
 */
GradientStrip::GradientStrip(int init)
{
	name=file=NULL;

	gradient_flags=0;
	num_columns_hint=0;
	r1=0;
	r2=1;
	p2.x=1;

	maxx=100;
	maxy=25;

	tmin=smin=0;
	tmax=smax=1;

	if (init) FlushColors(true); //sets white to black
}


/*! Create new basic gradient t1 (the start) to t2 (the end). Sets col1 to start, and col2 to the end.
 * t1 defaults to 0, t2 defaults to 1. If otherwise, these become the new min and max values.
 *
 *  This actually sets via Set().
 */
GradientStrip::GradientStrip(ScreenColor *col1, ScreenColor *col2)
{
	name=file=NULL;

	gradient_flags=0;
	num_columns_hint=0;
	r1=0;
	r2=1;
	p2.x=1;

	smin=0;
	smax=1;
	tmin=0;
	tmax=1; 

	Set(col1,col2, false);
}

/*! Create new basic gradient t1 (the start) to t2 (the end). Sets col1 to start, and col2 to the end.
 * t1 defaults to 0, t2 defaults to 1. If otherwise, these become the new min and max values.
 *
 *  This actually sets via Set(). If dup, then dup the Color objects. Else link.
 */
GradientStrip::GradientStrip(Color *col1, int dup1, Color *col2, int dup2)
{
	name=file=NULL;

	gradient_flags=0;
	num_columns_hint=0;
	r1=0;
	r2=1;
	p2.x=1;

	smin=0;
	smax=1;
	tmin=0;
	tmax=1;

	Set(col1,dup1, col2,dup2, false);
}

GradientStrip::GradientStrip(flatpoint from, flatpoint to, double rr1, double rr2, Color *col1, int dup1, Color *col2, int dup2)
{
	name = file = NULL;
	gradient_flags = 0;
	num_columns_hint = 0;

	p1 = from;
	p2 = to;
	r1 = rr1;
	r2 = rr2;

	if (col1 || col2) Set(col1,dup1, col2,dup2, false);
}

GradientStrip::GradientStrip(flatpoint from, flatpoint to, double rr1, double rr2, Laxkit::ScreenColor *col1, Laxkit::ScreenColor *col2)
{
	name = file = NULL;
	gradient_flags = 0;
	num_columns_hint = 0;

	p1 = from;
	p2 = to;
	r1 = rr1;
	r2 = rr2;

	if (col1 || col2) Set(col1, col2, false);
}

GradientStrip::~GradientStrip()
{
	delete[] file;
	delete[] name;
}

void GradientStrip::touchContents()
{
	Previewable::touchContents();
}

void GradientStrip::SetFlags(unsigned int flags, bool on)
{
	if (flags & Gimp_Spots) {
		if (on) gradient_flags |= Gimp_Spots;
		else gradient_flags &= ~Gimp_Spots;
		for (int c=0; c<colors.n; c++) {
			if (on) colors.e[c]->flags |= Gimp_Spots;
			else    colors.e[c]->flags &= ~Gimp_Spots;
		}
	}

	if (flags & StripOnly) {
		if (on) gradient_flags = (gradient_flags & ~(Linear|Radial)) | StripOnly;
		//turning off has no effect, it is already at full fallback
	}

	if (flags & AsPalette) {
		 //use num_columns_hint, ignore radial/linear stuff
		if (on) gradient_flags |= AsPalette;
		else gradient_flags &= ~AsPalette;
	}

	if (flags & Linear) {
		if (on) gradient_flags = (gradient_flags & ~(Linear|Radial|StripOnly)) | Linear;
		else    gradient_flags &= ~Linear; 
	}

	if (flags & Radial) {
		if (on) gradient_flags = (gradient_flags & ~(Linear|Radial|StripOnly)) | Radial;
		else    gradient_flags &= ~Radial; 
	}

	if (flags & Built_in) {
		if (on) gradient_flags |= Built_in;
		else    gradient_flags &= ~Built_in; 
	}

	if (flags & Read_only) {
		if (on) gradient_flags |= Read_only;
		else    gradient_flags &= ~Read_only; 
	}
}

bool GradientStrip::IsRadial()  { return gradient_flags & Radial; }
bool GradientStrip::IsLinear()  { return gradient_flags & Linear; }
bool GradientStrip::IsPalette() { return gradient_flags & AsPalette; }

GradientStrip *GradientStrip::newGradientStrip()
{
	//dup=dynamic_cast<SomeData*>(somedatafactory()->NewObject(LAX_GRADIENTDATA));
	return new GradientStrip;
}

int GradientStrip::maxPreviewSize()
{
	return 200;
}

/*! Warning: name and file are not copied.
 */
anObject *GradientStrip::duplicate(anObject *dup)
{
	GradientStrip *g = dynamic_cast<GradientStrip*>(dup);
	if (dup && !g) return nullptr; //supplied reference was not GradientStrip!

	if (!dup) {
		dup = newGradientStrip();
		if (dup) {
			g = dynamic_cast<GradientStrip*>(dup);
			g->setbounds(minx,maxx,miny,maxy);
		}
	} 

	if (!g) {
		g = new GradientStrip();
		dup = g;
	}

	g->gradient_flags = gradient_flags;
	g->gradient_flags &= ~Built_in;

	g->tmin = tmin;
	g->tmax = tmax;
	g->smin = smin;
	g->smax = smax;

	g->p1 = p1;
	g->p2 = p2;
	g->r1 = r1;
	g->r2 = r2;

	makestr(g->name, name);

	for (int c=0; c<colors.n; c++) {
		g->colors.push(new GradientStrip::GradientSpot(colors.e[c]));
	}

	return dup;
}

/*! This sets the gradient as a radial gradient.
 * Use Set() to set the color range. 
 */
void GradientStrip::SetRadial(flatpoint pp1, flatpoint pp2, double rr1, double rr2)
{
	SetFlags(Radial, true);

	p1 = pp1;
	p2 = pp2;
	r1 = rr1;
	r2 = rr2;

	touchContents();
}

/*! This sets the gradient as a radial gradient.
 * Use Set() to set the color range. 
 *
 * The rr1 and rr2 for linear gradients are for upper and lower bounds away from the main strip.
 */
void GradientStrip::SetLinear(flatpoint pp1,flatpoint pp2,double rr1,double rr2)
{
	SetFlags(Linear, true);

	p1 = pp1;
	p2 = pp2;
	r1 = rr1;
	r2 = rr2;

	touchContents();
}

int GradientStrip::SetStock(GradStockType which)
{
	colors.flush();

	if (which == WhiteToBlack) {
		AddColor(0, 1,1,1,1);
		AddColor(1, 0,0,0,1);

	} else if (which == BlackToWhite) {
		AddColor(0, 0,0,0,1);
		AddColor(1, 1,1,1,1);

	} else if (which == TranspToWhite) {
		AddColor(0, 1,1,1,0);
		AddColor(1, 1,1,1,1);

	} else if (which == WhiteToTransp) {
		AddColor(0, 1,1,1,1);
		AddColor(1, 1,1,1,0);

	} else return 1;

	return 0;
}

/*! Replace any current colors so gradient is col1 -> col2.
 *
 * If reset_bounds, set p1 to the origin, p1 to (1,0), (tmin,tmax)=(0,1).
 * If radial, set r1=0, r2=1. If linear, set r1=r2=-1;
 */
void GradientStrip::Set(Color *col1, bool dup1, Color *col2, bool dup2, bool reset_bounds)
{
	FlushColors(false);

	if (reset_bounds) {
		p1.set(0,0);
		p2.set(1,0);
		if (IsLinear()) { r1=-1;   r2=1; }
		else { r1=0; r2=1; } 
		tmin=0; tmax=1;
		smin=0; smax=1;
	}

	AddColor(new GradientStrip::GradientSpot(0,0, col1, dup1));	
	AddColor(new GradientStrip::GradientSpot(1,0, col2, dup1)); 
}

/*! Replace any current colors so gradient is col1 -> col2.
 */
void GradientStrip::Set(ScreenColor *col1, ScreenColor *col2, bool reset_bounds)
{
	Color *c1 = ColorManager::newColor(LAX_COLOR_RGB, col1);
	Color *c2 = ColorManager::newColor(LAX_COLOR_RGB, col2);
	Set(c1,false, c2,false, reset_bounds);
	c1->dec_count();
	c2->dec_count();
}

/*! Insert new color at position index.
 *  The color components are in range [0,1].
 */
int GradientStrip::SetColor(int index, double red,double green,double blue,double alpha)
{
	if (index<0 || index>=colors.n) return 1;
	Color *color = ColorManager::newColor(LAX_COLOR_RGB, 4, red,green,blue,alpha);

	if (colors.e[index]->color) colors.e[index]->color->dec_count();
	colors.e[index]->color = color;

	return 0;
}

/*! Place new color in right spot in list.
 *
 *  If index is already present, then overwrite.
 */
int GradientStrip::SetColor(int index, ScreenColor *scolor)
{
	if (index<0 || index>=colors.n) return 1;
	Color *color = ColorManager::newColor(LAX_COLOR_RGB, scolor);

	if (colors.e[index]->color) colors.e[index]->color->dec_count();
	colors.e[index]->color = color;

	return 0;
}

/*! Place new color in right spot in list.
 *
 *  If index is already present, then overwrite.
 *  If dup, then duplicate color, else link and inc count.
 */
int GradientStrip::SetColor(int index, Color *color, bool dup)
{
	if (index<0 || index>=colors.n) return 1;
	Color *ncolor = (dup ? color->duplicate() : color);
	if (!dup) ncolor->inc_count();

	if (colors.e[index]->color) colors.e[index]->color->dec_count();
	colors.e[index]->color = ncolor;

	return 0;
}

void GradientStrip::dump_in (FILE *f,int indent,int what,DumpContext *context,Attribute **att)
{
	if (what==0) {
		DumpUtility::dump_in(f,indent,what,context,att);

	} else if (what == GimpGPL) {
		IOBuffer io;
		io.UseThis(f);
		ImportGimpGPL(io);
		io.UseThis(nullptr);

	} else if (what == GimpGGR) {
		IOBuffer io;
		io.UseThis(f);
		ImportGimpGGR(io);
		io.UseThis(nullptr);
	}
}


/*! This will import a gimp_palette.gpl as a Palette.
 * Return whether import was successful.
 */
bool GradientStrip::ImportGimpGPL(IOBuffer &f)
{
	if (f.IsEOF()) return false;
		
	char *line = nullptr;
	int c;
	size_t n = 0;
	bool err = false;

	c = f.GetLine(&line,&n);
	if (c>0 && strncmp(line,"GIMP Palette",12)) { err = true; c = 0; }
	
	if (c>0) c = f.GetLine(&line, &n);
	if (c>0 && !strncmp(line,"Name: ",6)) {
		makestr(name,line+6);
		name[strlen(name)-1]='\0';
	} else c = 0;

	if (c>0) c = f.GetLine(&line, &n);
	if (c>0 && !strncmp(line,"Columns: ",9)) IntAttribute(line+9, &num_columns_hint);

	int rgb[3], n2;
	char *e;
	int ci = 0;
	while (c>0 && !f.IsEOF()) {
		c = f.GetLine(&line, &n);
		if (c <= 0) break;
		e = nullptr;
		n2 = IntListAttribute(line, rgb, 3, &e);
		if (n2 != 3) continue;
		while (e && isspace(*e)) e++;
		if (e[strlen(e)-1] == '\n') e[strlen(e)-1] = '\0';

		AddColor(ci, rgb[0]/255., rgb[1]/255., rgb[2]/255., 1.0, e);
		ci++;
	} 
	if (line) free(line);

	if (!err) {
		gradient_flags |= AsPalette;
	}
	return !err;
}

bool GradientStrip::ExportGimpGPL(IOBuffer &f)
{
	if (f.IsEOF()) return false;

	f.Printf("GIMP Palette\n");
	f.Printf("Name: %s\n",(name?name:"Untitled"));
	if (num_columns_hint > 0) f.Printf("Columns: %d\n", num_columns_hint);
	f.Printf("#\n");

	int c,c2;
	for (c=0; c < colors.n; c++) {
		for (c2=0; c2<3 /*colors.e[c]->color->nvalues*/; c2++) {
			f.Printf("%3d ", int(.5+255*colors.e[c]->color->values[c2]));
		}
		if (colors.e[c]->name) f.Printf ("%s\n", colors.e[c]->name);
		else f.Printf ("%02x%02x%02x\n",
			(int)(colors.e[c]->color->values[0]*255),
			(int)(colors.e[c]->color->values[1]*255),
			(int)(colors.e[c]->color->values[2]*255));
	}

	return true;
}


/*! This will import a gimp_palette.ggr as a GradientStrip.
 * Return whether import was successful.
 */
bool GradientStrip::ImportGimpGGR(IOBuffer &f)
{
	if (f.IsEOF()) return false;
		
	char *line = nullptr;
	int c;
	size_t n = 0;
	bool err = false;

	c = f.GetLine(&line,&n);
	if (c>0 && strncmp(line,"GIMP Gradient",13)) { err = true; c = 0; }
	
	if (c>0) c = f.GetLine(&line, &n);
	if (c>0 && !strncmp(line,"Name: ",6)) {
		makestr(name,line+6);
		name[strlen(name)-1]='\0';
	} else c = 0;

	if (c>0) c = f.GetLine(&line, &n);
	int num = -1;
	if (c>0) IntAttribute(line, &num);

	int n2;
	char *e;
	int ci = 0;
	double vals[15];
	while (c>0 && !f.IsEOF()) {
		c = f.GetLine(&line, &n);
		if (c <= 0) break;
		e = nullptr;
		n2 = DoubleListAttribute(line, vals, 3, &e);
		if (n2 < 13) continue; //todo: really this is probably an error

		GradientSpot *spot = new GradientSpot();
		spot->t = spot->nt = vals[0];
		double seglen = vals[2] - vals[1];
		if (fabs(seglen) > 1e-6) {
			spot->midposition = (vals[1] - vals[0]) / seglen;
		}
		spot->color = ColorManager::newColor(LAX_COLOR_RGB, 4, vals[3],vals[4],vals[5],vals[6]);
		// *** TODO: gimp grads allow different left and right colors, but we are using only the left here
		AddColor(spot);
		ci++;
	} 
	if (line) free(line);

	if (!err) {
		gradient_flags |= AsPalette;
	}
	return !err;
}


bool GradientStrip::ExportGimpGGR(IOBuffer &f)
{
	if (f.IsEOF()) return false;

	f.Printf("GIMP Gradient\n");
	f.Printf("Name: %s\n",(name?name:"Untitled"));
	f.Printf("%d\n", colors.n-1);

	 //  0          Left endpoint coordinate
	 //  1          Midpoint coordinate
	 //  2          Right endpoint coordinate
	 //  3          Left endpoint R
	 //  4          Left endpoint G
	 //  5          Left endpoint B
	 //  6          Left endpoint A
	 //  7          Right endpoint R
	 //  8          Right endpoint G
	 //  9          Right endpoint B
	 // 10          Right endpoint A
	 // 11          Blending function type
	 // 			0 = "linear"
     // 			1 = "curved"
     // 			2 = "sinusoidal"
     // 			3 = "spherical (increasing)"
     // 			4 = "spherical (decreasing)"
     // 			5 = "step")
	 // 12          Coloring type
	 // 			0 = "RGB"
	 // 			1 = "HSV CCW"
	 // 			2 = "HSV CW"
	 // 13          Left endpoint color type
	 // 			0 = "fixed"
	 // 			1 = "foreground",
	 // 			2 = "foreground transparent"
	 // 			3 = "background",
	 // 			4 = "background transparent"
	 // 14          Right endpoint color type

	int c;
	for (c=0; c < colors.n-1; c++) {
		// todo: need to handle non-rgba
		f.Printf("%.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f %d %d %d %d\n",
				colors.e[c]->nt, // left
				colors.e[c]->nt + colors.e[c]->midposition * (colors.e[c+1]->nt - colors.e[c]->nt), // mid point
				colors.e[c+1]->nt, // right
				colors.e[c]->color->values[0], // left r
				colors.e[c]->color->values[1], // left g
				colors.e[c]->color->values[2], // left b
				colors.e[c]->color->values[3], // left a
				colors.e[c+1]->color->values[0], // right r
				colors.e[c+1]->color->values[1], // right g
				colors.e[c+1]->color->values[2], // right b
				colors.e[c+1]->color->values[3], // right a
				0, // blending function
				0, // Coloring type
				0, // left color type
				0 // right color type
			);
	}

	return true;
}


bool GradientStrip::ImportScribusXML(const char *filename)
{
	Attribute att;
	XMLFileToAttribute(&att, filename, nullptr);
	
	if (att.attributes.n == 0) return false;
	Attribute *a = att.find("SCRIBUSCOLORS");
	if (!a) return false;

	const char *nm = a->findValue("Name");

	a = a->find("content:");
	if (!a) return false;

	if (nm) makestr(name, nm);
	double rgbv[3];
	ScreenColor col;

	for (int c=0; c<a->attributes.n; c++) {
		const char *nme = a->attributes.e[c]->name;
		
		if (strcmp(nme, "COLOR")) continue;

		const char *color_name = a->attributes.e[c]->findValue("NAME");
		const char *rgb = a->attributes.e[c]->findValue("RGB");
		const char *cmyk = a->attributes.e[c]->findValue("CMYK");

		if (!rgb && !cmyk) continue;

		if (rgb) {
			int success = HexColorAttributeRGB(rgb, &col, nullptr);
			if (!success) continue; //TODO: this is actually an error which should propagate upward
			rgbv[0] = col.red   / 65535.;
			rgbv[1] = col.green / 65535.;
			rgbv[2] = col.blue  / 65535.;

		} else if (cmyk) {
			// 4 values, in gets mapped: cmyk -> argb
			int success = HexColorAttributeRGB(cmyk, &col, nullptr);
			if (!success) continue;
			double cmyk[4];
			cmyk[0] = col.alpha / 65535.;
			cmyk[1] = col.red   / 65535.;
			cmyk[2] = col.green / 65535.;
			cmyk[3] = col.blue  / 65535.;
			simple_cmyk_to_rgb(cmyk, rgbv);
		}
		//SPOT == "0" or "1"
		//REGISTER == "0" or "1"
		
		AddColor(0, rgbv[0], rgbv[1], rgbv[2], 1.0, !isblank(color_name) ? color_name : nullptr);
	}

	gradient_flags |= AsPalette;
	return true;
}

bool GradientStrip::ExportScribusXML(IOBuffer &f)
{
	if (f.IsEOF()) return false;

	f.Printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	f.Printf("<SCRIBUSCOLORS Name=\"%s\">\n", (name?name:"Untitled"));
	
	for (int c=0; c < colors.n; c++) {
		if (isblank(colors.e[c]->name)) {
			//TODO: need to account for non-rgb
			f.Printf("  <COLOR RGB=\"#%02X%02X%02X\" NAME=\"#%02X%02X%02X\" />\n",
				(int)(colors.e[c]->color->values[0]*255+.5),
				(int)(colors.e[c]->color->values[1]*255+.5),
				(int)(colors.e[c]->color->values[2]*255+.5),
				(int)(colors.e[c]->color->values[0]*255+.5),
				(int)(colors.e[c]->color->values[1]*255+.5),
				(int)(colors.e[c]->color->values[2]*255+.5)
				);

		} else {
			f.Printf("  <COLOR RGB=\"#%02X%02X%02X\" NAME=\"%s\" />\n",
				(int)(colors.e[c]->color->values[0]*255+.5),
				(int)(colors.e[c]->color->values[1]*255+.5),
				(int)(colors.e[c]->color->values[2]*255+.5),
				colors.e[c]->name);
		}
	}

	f.Printf("</SCRIBUSCOLORS>");

	return true;
}


/*! Export two CSS classes per color, one for "background-color" and one for "color".
 */
bool GradientStrip::ExportCSSPalette(IOBuffer &f)
{
	if (f.IsEOF()) return false;

	f.Printf("/* Name: %s */\n",(name?name:"Untitled"));
	
	Utf8String color_name;
	Utf8String color;

	for (int c=0; c < colors.n; c++) {

		if (isblank(colors.e[c]->name)) {
			color_name.Sprintf("hex%02x%02x%02x%02x",
				(int)(colors.e[c]->color->values[0]*255),
				(int)(colors.e[c]->color->values[1]*255),
				(int)(colors.e[c]->color->values[2]*255),
				(int)(colors.e[c]->color->values[3]*255));
		} else color_name = colors.e[c]->name;
		color_name.Replace(" ", "", true);

		color.Sprintf ("rgba(%d, %d, %d, %f)",
			(int)(colors.e[c]->color->values[0]*255),
			(int)(colors.e[c]->color->values[1]*255),
			(int)(colors.e[c]->color->values[2]*255),
			colors.e[c]->color->values[3]);

		f.Printf(".%s-bg { background-color: %s; }\n", color_name.c_str(), color.c_str());
		f.Printf(".%s { color: %s; }\n", color_name.c_str(), color.c_str());
	}

	return true;
}


/*! Export a CSS gradient with fallback to first color, so something like:
 * <code>
 *    background: rgb(100,255,100);
 *    background: linear-gradient(45deg, rgba(100,255,100,1) 0%, rgba(255,100,50,1) 100%);
 * </code>
 */
bool GradientStrip::ExportCSSGradient(IOBuffer &f)
{
	if (f.IsEOF()) return false;

	f.Printf("/* Name: %s */\n",(name?name:"Untitled"));
	
	Utf8String color;

	color.Sprintf ("rgba(%d, %d, %d, %f)",
			(int)(colors.e[0]->color->values[0]*255),
			(int)(colors.e[0]->color->values[1]*255),
			(int)(colors.e[0]->color->values[2]*255),
			colors.e[0]->color->values[3]);
	f.Printf("background: %s;\n", color.c_str());

	if (IsLinear()) {
		flatpoint v = p1 - p2;
		double angle = 180.0 / M_PI * atan2(v.y, v.x);
		f.Printf("background: linear-gradient(%.2fdeg, ", angle);
	} else {
		f.Printf("background: radial-gradient(circle, ");
	}

	for (int c=0; c < colors.n; c++) {
		color.Sprintf ("rgba(%d, %d, %d, %f)",
			(int)(colors.e[c]->color->values[0]*255),
			(int)(colors.e[c]->color->values[1]*255),
			(int)(colors.e[c]->color->values[2]*255),
			colors.e[c]->color->values[3]);

		f.Printf("%s %.2f%% ", color.c_str(), 100*colors.e[c]->nt);
		if (c < colors.n-1) f.Printf(", ");
	}
	f.Printf(");\n");

	return true;
}


/*! Export an svg grid filled with the palette.
 */
bool GradientStrip::ExportSVGGrid(IOBuffer &f)
{
	if (f.IsEOF()) return false;

	f.Printf("<svg width=\"100\" height=\"100\">\n");
	if (!isblank(name)) f.Printf("<!-- %s -->\n");
	
	Utf8String color;
	int xn, yn;
	if (num_columns_hint > 0) {
		xn = num_columns_hint;
	} else {
		double aspect = 1.0;
		xn = int(ceil(sqrt(colors.n / aspect)));
		if (xn == 0) xn = 1;
	}
	yn = (colors.n-1) / xn + 1;
	if (yn <= 0) yn = 1;
	
	double x=0, y=0, w=100.0/xn, h=100.0/yn;

	// draw grid of colors
	for (int c=0; c < colors.n; c++) {

		color.Sprintf ("rgb(%d, %d, %d)",
			(int)(colors.e[c]->color->values[0]*255+.5),
			(int)(colors.e[c]->color->values[1]*255+.5),
			(int)(colors.e[c]->color->values[2]*255+.5));

		f.Printf("  <rect x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\" style=\"fill:%s; fill-opacity:%f; stroke:none;\" />\n",
			x,y,w,h, color.c_str(), colors.e[c]->color->values[3]);

		x += w;
		if ((c+1)%xn == 0) {
			y += h;
			x = 0;
		}
	}

	f.Printf("</svg>");

	return true;
}


/*! Reads in from something like this. p1 and p2 can also be flatvectors:
 * <pre>
 *  matrix 1 0 0 1 0 0
 *  p1 0
 *  p2 1
 *  r1 0
 *  r2 10
 *  spot
 *    t 0
 *    color rgbf(1.0, .5, .25, 1.0)
 *  spot
 *    t 1
 *    color rgbf(0, 0, 0, 1.0)
 *  radial
 * </pre>
 */
void GradientStrip::dump_in_atts(Attribute *att,int flag,DumpContext *context)
{
	if (!att) return;
	char *name,*value,*e;

	int type=0;
	bool firstcolor=true;

	for (int c=0; c<att->attributes.n; c++) {
		name=att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"p1")) {
			if (!FlatvectorAttribute(value,&p1,&e)) {
				double d = 0;
				if (DoubleAttribute(value, &d)) {
					p1.x = d;
					p1.y = 0;
				}
			}

		} else if (!strcmp(name,"p2")) {
			if (!FlatvectorAttribute(value,&p2,&e)) {
				double d = 0;
				if (DoubleAttribute(value, &d)) {
					p2.x = d;
					p2.y = 0;
				}
			}

		} else if (!strcmp(name,"r1")) {
			DoubleAttribute(value,&r1,&e);

		} else if (!strcmp(name,"r2")) {
			DoubleAttribute(value,&r2,&e);

		} else if (!strcmp(name,"num_columns")) {
			IntAttribute(value, &num_columns_hint);

		} else if (!strcmp(name,"spot")) {
			if (firstcolor) {
				firstcolor=false; //only flush previous points if we find any new colors
				FlushColors(false);
			}
			GradientStrip::GradientSpot *spot=new GradientStrip::GradientSpot();
			spot->dump_in_atts(att->attributes.e[c],flag,context);
			AddColor(spot);

		} else if (!strcmp(name,"linear")) {
			if (BooleanAttribute(value)) type=1;

		} else if (!strcmp(name,"radial")) {
			if (BooleanAttribute(value)) type=-1;
		}
	}

	if      (type==-1) SetFlags(Radial, true);
	else if (type== 1) SetFlags(Linear, true);
	else SetFlags(StripOnly, true);

	touchContents();
}

/*! \ingroup interfaces
 * Dump out a GradientStrip. Prints matrix, p, v, and the spots.
 *
 * If what==-1, then dump out a psuedocode mockup of what gets dumped. This makes it very easy
 * for programs to keep track of their file formats, that is, when the programmers remember to
 * update this code as change happens.
 * Otherwise dumps out in indented data format as described in dump_in_atts().
 */
void GradientStrip::dump_out(FILE *f,int indent,int what,DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%sp1 (0,0) #the starting coordinate (not for palettes)\n",spc);
		fprintf(f,"%sp2 (1,0) #the ending coordinate (not for palettes)\n",spc);
		fprintf(f,"%sr1 0     #the starting radius (radial) or the +y extent (linear) (not for palettes)\n",spc);
		fprintf(f,"%sr2 0     #the ending radius (radial) or the -y extent (linear) (not for palettes)\n",spc);
		fprintf(f,"%snum_columns #hint for number of columns in palette view\n",spc);
		fprintf(f,"%sradial   #Specifies a radial gradient\n",spc);
		fprintf(f,"%slinear   #Specifies a linear gradient\n",spc);
		fprintf(f,"%spalette  #Specifies a palette. p1, p2, r1, r2 ignored in this case\n",spc);
		fprintf(f,"%sspot     #There will be at least two gradient data spots, such as this:\n",spc);
		if (colors.n) colors.e[0]->dump_out(f,indent+2,-1,NULL);
		else {
			GradientStrip::GradientSpot g(0,0, .5,1.,1.,1.);
			g.dump_out(f,indent+2,-1,NULL);
		}
		//colors.e[colors.n-1]->dump_out(f,indent+2,-1);//*** should probably check that there are always 2 and not ever 0!
		return;
	}

	if (what == GimpGPL) {
		IOBuffer io;
		io.UseThis(f);
		ExportGimpGPL(io);
		io.UseThis(nullptr);
		return;
	}

	if (what == GimpGGR) {
		IOBuffer io;
		io.UseThis(f);
		ExportGimpGGR(io);
		io.UseThis(nullptr);
		return;
	}

	if (what == ScribusXML) {
		IOBuffer io;
		io.UseThis(f);
		ExportScribusXML(io);
		io.UseThis(nullptr);
		return;
	}

	if (what == CSSColors) {
		IOBuffer io;
		io.UseThis(f);
		ExportCSSPalette(io);
		io.UseThis(nullptr);
		return;
	}

	if (what == SVGGrid) {
		IOBuffer io;
		io.UseThis(f);
		ExportSVGGrid(io);
		io.UseThis(nullptr);
		return;
	}

	// else default:

	if (!IsPalette()) {
		fprintf(f,"%sp1 (%.10g, %.10g)\n",spc, p1.x,p1.y);
		fprintf(f,"%sp2 (%.10g, %.10g)\n",spc, p2.x,p2.y);
		fprintf(f,"%sr1 %.10g\n",spc,r1);
		fprintf(f,"%sr2 %.10g\n",spc,r2);
	}

	if (IsRadial()) fprintf(f,"%sradial\n",spc);
	if (IsLinear()) fprintf(f,"%slinear\n",spc);

	fprintf(f,"%snum_columns %d\n",spc, num_columns_hint);

	for (int c=0; c<colors.n; c++) {
		fprintf(f,"%sspot #%d\n",spc,c);
		colors.e[c]->dump_out(f,indent+2,0,context);
	}
}

Attribute *GradientStrip::dump_out_atts(Attribute *att,int what,DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("p1", "(0,0)", "the starting coordinate");
		att->push("p2", "(1,0)", "the ending coordinate");
		att->push("r1", "0",     "the starting radius (radial) or the +y extent (linear)");
		att->push("r2", "0",     "the ending radius (radial) or the -y extent (linear)");
		att->push("num_columns", "hint for number of columns in palette view");
		att->push("radial",nullptr, "Specifies a radial gradient");
		att->push("linear",nullptr, "Specifies a linear gradient");
		Attribute *att2=att->pushSubAtt("spot",nullptr,"There will be at least two gradient data spots, such as this:");

		if (colors.n) colors.e[0]->dump_out_atts(att2,what,context);
		else {
			GradientStrip::GradientSpot g(0,0, .5,1.,1.,1.);
			g.dump_out_atts(att2,what,context);
		}

		return att;
	}

	char scratch[100];
	sprintf(scratch, "(%.10g, %.10g)", p1.x,p1.y);
	att->push("p1", scratch);
	sprintf(scratch, "(%.10g, %.10g)", p2.x,p2.y);
	att->push("p2", scratch);
	att->push("r1", r1);
	att->push("r2", r2);

	if (IsRadial()) att->push("radial");
	if (IsLinear()) att->push("linear");

	att->push("num_columns", num_columns_hint);

	for (int c=0; c<colors.n; c++) {
		Attribute *att2=att->pushSubAtt("spot");
		colors.e[c]->dump_out_atts(att2,what,context);
	}

	return att;
}

/*! Using the t values, update the GradientStrip::GradientSpot::nt, which need to be in range [0..1].
 *
 * This should be called whenever a color is shifted around, added or removed.
 *
 * If set==0, then set the t to be the same as the normalized nt.
 */
void GradientStrip::UpdateNormalized(bool set)
{
	if (!colors.n) return;

	tmin = colors.e[0]->t;
	tmax = colors.e[colors.n-1]->t;

	for (int c=0; c<colors.n; c++) {
		if (colors.e[c]->t < tmin) tmin=colors.e[c]->t;
		if (colors.e[c]->t > tmax) tmax=colors.e[c]->t;
	}

	double len=tmax-tmin;
	if (len==0) {
		for (int c=0; c<colors.n; c++) colors.e[c]->nt = 0;

	} else {
		for (int c=0; c<colors.n; c++) {
			colors.e[c]->nt = (colors.e[c]->t - tmin)/len;
		}
	}

	if (set) for (int c=0; c<colors.n; c++) colors.e[c]->t = colors.e[c]->nt;
}

/*! Return the t of colors.e[i] mapped to the range [0..1] where
 * 0 is colors.e[0] and 1 is colors.e[colors.n-1].
 *
 * i out of range returns -1.
 */
double GradientStrip::GetNormalizedT(int index)
{
	if (index<0 || index>=colors.n) return -1;
	return colors.e[index]->nt;
	//return (colors.e[i]->t - colors.e[0]->t)/(colors.e[colors.n-1]->t - colors.e[0]->t);
}

int GradientStrip::NumColors()
{
	return colors.n;
}

/*! Range of t values of the stops. If no stops, then 0.
 */
double GradientStrip::TRange()
{
	if (!colors.n) return 0;
	return colors.e[colors.n-1]->t - colors.e[0]->t;
}

double GradientStrip::MinT()
{
	if (!colors.n) return 0;
	return colors.e[0]->t;
}

double GradientStrip::MaxT()
{
	if (!colors.n) return 0;
	return colors.e[colors.n-1]->t;
}

/*! If reset, then replace all with a gradient from 0..1 with white to black.
 * If !reset, then flush all colors, and leave undefined.
 */
int GradientStrip::FlushColors(bool reset)
{
	colors.flush();
	if (!reset) return -1;

	ScreenColor col1(65535, 65535, 65535, 65535);
	ScreenColor col2(    0,     0,     0, 65535);

	tmin=0;
	tmax=1;
	Set(&col1, &col2, false);

	return 0;
}

Color *GradientStrip::GetColor(int index)
{
	if (index<0 || index>=colors.n) return NULL;
	return colors.e[index]->color;
}

GradientStrip::GradientSpot *GradientStrip::GetColorSpot(int index)
{
	if (index<0 || index>=colors.n) return NULL;
	return colors.e[index];
}

/*! Move the color index which to new_t = old_t + dt, rearranging color's stack position if necessary.*
 *  Note this is the GradientStrip t value, not normalized nt.
 *  If clamp, then clamp to min,max for the strip.
 *
 *  Returns the index of which color after shifting.
 */
int GradientStrip::ShiftPoint(int which, double dt, bool clamp)
{
	if (which<0 || which>=colors.n) return which;

	double min = colors.e[0]->t;
	double max = colors.e[colors.n-1]->t;
	colors.e[which]->t += dt;
	if (clamp) {
		if (colors.e[which]->t > max) colors.e[which]->t = max;
		else if (colors.e[which]->t < min) colors.e[which]->t = min;
	}
	
	GradientStrip::GradientSpot *tmp=colors.e[which];

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

	UpdateNormalized(false);
	touchContents();
	return which;
}

//! Flip the order of the colors.
void GradientStrip::FlipColors()
{
	GradientStrip::GradientSpot *tt;

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

int GradientStrip::RemoveColor(int index)
{
	if (index<0 || index>=colors.n) return 1;
	return colors.remove(index);
}

/*! Takes pointer, does not make duplicate.
 *  Returns the index in the strip after adding, or -1 for not added, such as if spot==nullptr.
 */
int GradientStrip::AddColor(GradientStrip::GradientSpot *spot)
{
	if (!spot) return -1;

	int c = 0;
	while (c < colors.n && spot->t > colors.e[c]->t) c++;
	colors.push(spot,1,c);

	UpdateNormalized(false);
	touchContents();
	return c;
}

/*! Add a spot with the given color, or interpolated, if col==NULL.
 * If t is already an existing point, then replace that color. In this case, 
 * nothing is done if col==NULL.
 * Returns the index in the strip after adding, or -1 for not added.
 */
int GradientStrip::AddColor(double t,ScreenColor *col, const char *nname)
{
	if (!col) return AddColor(t, NULL, false, nname);
	
	return AddColor(t, col->red/65535.,col->green/65535.,col->blue/65535.,col->alpha/65535., nname);
}
	
/*! red, green, blue, alpha are assumed to be in range [0..1]
 * Returns the index in the strip after adding, or -1 for not added.
 */
int GradientStrip::AddColor(double t, double red,double green,double blue,double alpha, const char *nname)
{
	Color *color = ColorManager::newColor(LAX_COLOR_RGB, 4, red,green,blue,alpha);
	color->screen.rgbf(red,green,blue,alpha);
	int status = AddColor(t, color, false, nname);
	color->dec_count();
	return status;
}

/*! Add a color without concern for t values for sorting, which are relevant only for gradients.
 */
int GradientStrip::AddPaletteColor(double red,double green,double blue,double alpha, const char *nname, int where)
{
	Color *color = ColorManager::newColor(LAX_COLOR_RGB, 4, red,green,blue,alpha);
	color->screen.rgbf(red,green,blue,alpha);
	GradientStrip::GradientSpot *gds = new GradientStrip::GradientSpot(where,0, color,false);
	color->dec_count();
	if (nname) makestr(gds->name, nname);

	int status = colors.push(gds,1,where);	
	return status;
}

/*! Place new color in right spot in list.
 *
 *  If t is already present, then overwrite.
 *  If dup, then duplicate color, else link and inc count.
 *  If color==NULL, then interpolate at t.
 *
 * Returns the index in the strip after adding, or -1 for not added.
 */
int GradientStrip::AddColor(double t, Color *color, bool dup, const char *nname)
{
	int c=0; 
	while (c<colors.n && t>colors.e[c]->t) c++;

	if (c < colors.n && t == colors.e[c]->t) {
		if (color) {
			 //replace color
			colors.e[c]->color->dec_count();
			colors.e[c]->color = (dup ? color->duplicate() : color);
			if (!dup) color->inc_count();
		}

	} else {
		Color *usecolor = color;
		if (!usecolor) {
			 //interpolate color
			usecolor = WhatColor(t, false);
			dup = false;
		}

		GradientStrip::GradientSpot *gds = new GradientStrip::GradientSpot(t,0, usecolor,dup);
		if (nname) makestr(gds->name, nname);
		colors.push(gds,1,c);
		if (usecolor != color) color->dec_count();
	} 

	UpdateNormalized();
	touchContents();
	return c;
}


/*! Put the color into col if col!=NULL. Else return a new Color.
 *
 * col, if not NULL, MUST be a plain Color, not a ColorRef.
 */
Color *GradientStrip::WhatColor(double t, Color *col, bool is_normalized)
{
	double nt = (is_normalized ? t : (tmax>tmin ? (t-tmin)/(tmax-tmin) : 0));

	if (nt<0 || nt>1) {
		 //if nt out of bounds, check to see how to compute the value.
		 //Set nt to be the proper corresponding point within [0..1]
		if (gradient_flags & (Repeat|FlipRepeat)) {
			int block = int(nt);
			nt -= block; //remainder
			if (gradient_flags & FlipRepeat) {
				if (block%2==1) nt=1-nt;
			}
		}

		 //account for any rounding errors, plus handles Continue case
		if (nt<0) nt=0;
		else if (nt>1) nt=1;
	}

	Color *color = col;
	if (!color) color = new Color;

	if (nt <= colors.e[0]->nt) {
		*color = *colors.e[0]->color;

	} else if (nt >= colors.e[colors.n-1]->nt) {
		*color = *colors.e[colors.n-1]->color;

	} else {
		int c=0;
		while (c<colors.n && nt>colors.e[c]->nt) c++;

		if (c==0) { *color = *colors.e[0]->color; return color; }
		if (c==colors.n)  { *color = *colors.e[colors.n-1]->color; return color; }

		Color *c1 = colors.e[c-1]->color;
		Color *c2 = colors.e[c  ]->color;

		if (color->colorsystemid != c1->ColorSystemId()) {
			color->UpdateToSystem(c1);
		}

		nt = (nt-colors.e[c-1]->nt)/(colors.e[c]->nt-colors.e[c-1]->nt);

		for (int c=0; c<c1->NumChannels(); c++) {
			color->ChannelValue(c, c2->ChannelValue(c)*t + c1->ChannelValue(c)*(1-t)); 
		}
	}

	return color;
}

/*! Figure out what color lays at coordinate t.
 * Out of bounds are computed according to whether gradient_flags has
 * Repeat, FlipRepeat, or Continue set (none of those defaults to Continue).
 *
 * \todo need to implement the smoother Gimp_Spots flags
 */
int GradientStrip::WhatColor(double t, ScreenColor *col, bool is_normalized)
{
	double nt = (is_normalized ? t : (tmax>tmin ? (t-tmin)/(tmax-tmin) : 0));

	if (nt<0 || nt>1) {
		 //if nt out of bounds, check to see how to compute the value.
		 //Set nt to be the proper corresponding point within [0..1]
		if (gradient_flags & (Repeat|FlipRepeat)) {
			int block = int(nt);
			nt -= block; //remainder
			if (gradient_flags & FlipRepeat) {
				if (block%2==1) nt=1-nt;
			}
		}

		 //account for any rounding errors, plus handles Continue case
		if (nt<0) nt=0;
		else if (nt>1) nt=1;
	}

	if (nt <= colors.e[0]->nt) {
		*col = colors.e[0]->color->screen;

	} else if (nt >= colors.e[colors.n-1]->nt) {
		*col = colors.e[colors.n-1]->color->screen;

	} else {
		int c=0;
		while (c<colors.n && nt>colors.e[c]->nt) c++;
		if (c==0) { *col=colors.e[0]->color->screen; return -1; }
		if (c==colors.n)  { *col=colors.e[colors.n-1]->color->screen; return 1; }

		ScreenColor c1 = colors.e[c-1]->color->screen,
					c2 = colors.e[c  ]->color->screen;

		t=(t-colors.e[c-1]->t)/(colors.e[c]->t-colors.e[c-1]->t);
		col->red  = (unsigned short) (t*c2.red   + (1-t)*c1.red);
		col->green= (unsigned short) (t*c2.green + (1-t)*c1.green);
		col->blue = (unsigned short) (t*c2.blue  + (1-t)*c1.blue);
		col->alpha= (unsigned short) (t*c2.alpha + (1-t)*c1.alpha);
	}

	return 0;
}

/*! Fill image with a representation of the gradient/palette.
 * Please note this is mainly for generating screen preview images.
 *
 * Return 0 for success, or nonzero for error at some point.
 */
int GradientStrip::renderToBufferImage(LaxImage *image)
{
	if (IsPalette()) {
		return RenderPalette(image);

	} else if (IsRadial()) {
		return RenderRadial(image);

	} //else default to linear

	return RenderLinear(image);
}

int GradientStrip::RenderPalette(LaxImage *image)
{
    double aspect=double(image->h())/image->w();

	int xn,yn;
	double dx,dy;

    if (num_columns_hint>0) {
        xn=num_columns_hint;
        yn=colors.n/xn;
        if (colors.n%xn!=0) yn++;
    } else {
        xn=int(ceil(sqrt(colors.n/aspect)));
        yn=int(xn*aspect);
        while (xn*yn<colors.n) yn++;
    }

    dx=image->w()/xn;
    dy=image->h()/yn;

    if (dx<=0) dx=1;
    if (dy<=0) dy=1;

	Displayer *dp = GetDefaultDisplayer();
	dp->MakeCurrent(image);

	 //render transparency first
	dp->NewFG(0., 0., 0., 0.);
	dp->BlendMode(LAXOP_Source);
	dp->drawrectangle(0,0, image->w(), image->h(), 1);
	dp->BlendMode(LAXOP_Over);

    int i;
    double x=0,y;

    y=image->h()-dy;
    for (i=0; i<colors.n; i++) {
        if (i%xn==0) {
            x=0;
            y+=dy;
        }

		dp->NewFG(&colors.e[i]->color->screen);
        dp->drawrectangle(x,y,dx+1,dy+1, 1);

        x+=dx;
    }

	return 0;
}

int GradientStrip::RenderLinear(LaxImage *image)
{
	Displayer *dp = GetDefaultDisplayer();
	dp->MakeCurrent(image);

    double offsets[colors.n];
    ScreenColor scolors[colors.n];

    for (int c=0; c<colors.n; c++) {
        offsets[c]=colors.e[c]->nt;
        scolors[c]=colors.e[c]->color->screen;
    }

    dp->setLinearGradient(3, p1.x,p1.y, p2.x,p2.y, offsets, scolors, colors.n);

    //flatpoint v1=flatpoint(0,data->r1) - flatpoint(0,0);
    //flatpoint v2=flatpoint(0,data->r2) - flatpoint(0,0);

    dp->moveto(0,0);
    dp->lineto(image->w(),0);
    dp->lineto(image->w(),image->h());
    dp->lineto(0,image->h());
    dp->closed();
    dp->fill(0);

	return 0;
}

int GradientStrip::RenderRadial(LaxImage *image)
{ //***
	return RenderLinear(image); //TEMP!!
}


//-------------------------------- Utility functions -------------------------------

/*! Convenience function to set up a new GradientStrip as a Palette. */
GradientStrip *GradientStrip::newPalette()
{
	GradientStrip *palette = new GradientStrip;
	palette->SetFlags(AsPalette, true);
	return palette;
}



/*! \ingroup misc
 *
 * Make a palette with RYGCBM horizontally and white to black vertically.
 *
 * If include_gray_strip, then the final row is a transition from full white to full black.
 */
GradientStrip *GradientStrip::rainbowPalette(int w, int h, bool include_gray_strip)
{
	Palette *p = newPalette();
	makestr(p->name, _("Rainbow"));
	p->num_columns_hint = w;

	int x, y;
	double rgb[3];
	double max = 1.0;
	
	if (include_gray_strip) h--;
	
	for (y=0; y<h; y++) {
		for (x=0; x<w; x++) {
			if (x<w/6) {
				rgb[0] = max;
				rgb[1] = (max*x*6/w);
				rgb[2] = 0;
			} else if (x<w*2/6) {
				rgb[0] = max-(max*(6*x-w)/w);
				rgb[1] = max;
				rgb[2] = 0;
			} else if (x<w*3/6) {
				rgb[0] = 0;
				rgb[1] = max;
				rgb[2] = (max*(6*x-2*w)/w);
			} else if (x<w*4/6) {
				rgb[0] = 0;
				rgb[1] = max-(max*(6*x-3*w)/w);
				rgb[2] = max;
			} else if (x<w*5/6) {
				rgb[0] = (max*(6*x-4*w)/w);
				rgb[1] = 0;
				rgb[2] = max;
			} else {
				rgb[0] = max;
				rgb[1] = 0;
				rgb[2] = max-(max*(6*x-5*w)/w);
			}
			
			if (y<h/2) {
				rgb[0] = rgb[0]*y/(h/2);
				rgb[1] = rgb[1]*y/(h/2);
				rgb[2] = rgb[2]*y/(h/2);
			} else {
				rgb[0] = max*(y-(h-1)/2)/(h/2)+(rgb[0])*(h-1-y)/(h/2);
				rgb[1] = max*(y-(h-1)/2)/(h/2)+(rgb[1])*(h-1-y)/(h/2);
				rgb[2] = max*(y-(h-1)/2)/(h/2)+(rgb[2])*(h-1-y)/(h/2);
			}
			if (rgb[0]<0) rgb[0]=0;
			else if (rgb[0]>max) rgb[0]=max;
			if (rgb[1]<0) rgb[1]=0;
			else if (rgb[1]>max) rgb[1]=max;
			if (rgb[2]<0) rgb[2]=0;
			else if (rgb[2]>max) rgb[2]=max;
			p->AddColor(y*w+x, rgb[0], rgb[1], rgb[2], 1.0, nullptr);
		}
	}
	if (include_gray_strip) {
		for (x=0; x<w; x++) {
			rgb[0] = rgb[1] = rgb[2] = (max*x/(w-1));
			p->AddColor(h*w+x, rgb[0], rgb[1], rgb[2], 1.0, nullptr);
		}

	}
	return p;
}


} //namespace Laxkit

