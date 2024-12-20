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
//    Copyright (C) 2004-2012 by Tom Lechner
//

#include <lax/interfaces/linestyle.h>
#include <lax/drawingdefs.h>
#include <lax/laxutils.h>


using namespace Laxkit;


namespace LaxInterfaces {

/*! \class LineStyle
 * \ingroup interfaces
 * \brief Store various characteristics of a line.
 *
 * width==0 is taken to be hairline. width<0 is taken to mean invisible.
 * Care should be taken to make sure that (red,green,blue) corresponds to color.
 *
 * See LaxCapStyle and LaxJoinStyle.
 * 
 * \todo int width; *** including invisible vs. hairline (==0)
 */


LineStyle::LineStyle()
{
	mask = ~0;

	width      = 0;
	widthtype  = 1; //0 for screen width, 1 for real width
	color.red  = color.green = 0;
	color.blue = color.alpha = 0xffff;
	color2     = NULL;

	capstyle    = LAXCAP_Butt;
	endcapstyle = 0; //0 means use capstyle? .. need better cap support, use engraver line quality??
	//dash_start_caps=LAXCAP_Butt;
	//dash_end_caps  =LAXCAP_Butt;

	joinstyle  = LAXJOIN_Miter;
	miterlimit = 100; //so this means 100*(line thickness)
	function   = LAXOP_Over;

	stroke_fill = nullptr;

	 //dashes todo:
	 // on/off, dashes is list of lengths proportional to width of on and off
	 // off/on, dashes is list of lengths proportional to width of on and off
	 // broken, dashes holds settings for zero_threshhold, broken_threshhold, and other stuff..
	dotdash     = 0;
	dash_offset = 0;
	dashes      = NULL;
	numdashes   = 0;
}

LineStyle::LineStyle(const LineStyle &l) 
{
	width      = l.width;
	widthtype  = l.widthtype;
	color      = l.color;
	color2     = l.color2 ? l.color2->duplicate() : nullptr;
	capstyle   = l.capstyle;
	joinstyle  = l.joinstyle;
	miterlimit = l.miterlimit;
	function   = l.function;
	stroke_fill = l.stroke_fill;
	if (stroke_fill) stroke_fill->inc_count();

	dotdash     = l.dotdash;
	numdashes   = l.numdashes;
	dash_offset = l.dash_offset;
	if (numdashes) {
		dashes = new double[numdashes];
		memcpy(dashes,l.dashes, numdashes*sizeof(double));
	} else dashes = nullptr;
}

LineStyle &LineStyle::operator=(LineStyle &l) 
{
	width      = l.width;
	widthtype  = l.widthtype;
	color      = l.color;
	if (color2) color2->dec_count();
	color2     = l.color2 ? l.color2->duplicate() : nullptr;
	capstyle   = l.capstyle;
	joinstyle  = l.joinstyle;
	miterlimit = l.miterlimit;
	dotdash    = l.dotdash;
	function   = l.function;
	stroke_fill = l.stroke_fill;
	if (stroke_fill) stroke_fill->inc_count();

	delete[] dashes;
	numdashes   = l.numdashes;
	dash_offset = l.dash_offset;
	if (numdashes) {
		dashes = new double[numdashes];
		memcpy(dashes,l.dashes, numdashes*sizeof(double));
	} else dashes = nullptr;

	return l;
}

LineStyle::~LineStyle()
{
	if (stroke_fill) stroke_fill->dec_count();
	if (color2) color2->dec_count();
	delete[] dashes;
}

anObject *LineStyle::duplicate(anObject *ref)
{
	LineStyle *style = new LineStyle(*this);
	return style;
}

void LineStyle::Color(unsigned long col)
{
	ScreenColor cc(col);
	Colorf(cc);
}

//! Set the color. Components are 0..0xffff.
void LineStyle::Color(int r,int g,int b,int a)
{
	color.red  =r;
	color.green=g;
	color.blue =b;
	color.alpha=a;
}

//! Set the color. Components are 0..1.0.
void LineStyle::Colorf(double r,double g,double b,double a)
{
	color.red  =r*65535;
	color.green=g*65535;
	color.blue =b*65535;
	color.alpha=a*65535;
}

void LineStyle::Colorf(const Laxkit::ScreenColor &col)
{
	Colorf(col.Red(), col.Green(), col.Blue(), col.Alpha());
}

//! Dump in.
void LineStyle::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context)
{
	if (!att) return;
	char *name,*value;

	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"color")) {
			SimpleColorAttribute(value, NULL, &color, NULL);

		} else if (!strcmp(name,"width")) {
			DoubleAttribute(value,&width);

		} else if (!strcmp(name,"miterlimit")) {
			DoubleAttribute(value,&width);

		} else if (!strcmp(name,"capstyle")) {
			if      (!strcmp(value,"round"))      capstyle = LAXCAP_Round;
 			else if (!strcmp(value,"projecting")) capstyle = LAXCAP_Projecting;
 			else if (!strcmp(value,"zero"))       capstyle = LAXCAP_Zero_Width;
			else capstyle = LAXCAP_Butt;

		} else if (!strcmp(name,"endcapstyle")) {
			if      (!strcmp(value,"round"))      endcapstyle = LAXCAP_Round;
 			else if (!strcmp(value,"butt"))       endcapstyle = LAXCAP_Butt;
 			else if (!strcmp(value,"projecting")) endcapstyle = LAXCAP_Projecting;
 			else if (!strcmp(value,"zero"))       endcapstyle = LAXCAP_Zero_Width;
			else endcapstyle = 0;

		} else if (!strcmp(name,"joinstyle")) {
			if (!strcmp(value,"round")) joinstyle=LAXJOIN_Round;
 			else if (!strcmp(value,"bevel")) joinstyle=LAXJOIN_Bevel;
 			else if (!strcmp(value,"miter")) joinstyle=LAXJOIN_Miter;
			else joinstyle=LAXJOIN_Extrapolate;

		} else if (!strcmp(name,"dotdash")) {
			IntAttribute(value,&dotdash);

		} else if (!strcmp(name,"function")) {
			function = StringToLaxop(value);

		//} else if (!strcmp(name,"mask")) {
		//	ULongAttribute(value,&mask);
		}
	}
}

Laxkit::Attribute *LineStyle::dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("color","rgbaf(1,1,1,1)", "rgba in range [0..1]");
		att->push("capstyle","round",       "or miter, butt, projecting, zero");
		att->push("endcapstyle","round",    "or miter, butt, projecting, zero, same");
		att->push("joinstyle","round",      "or miter, bevel, extrapolate");
		att->push("miterlimit","100",       "means limit is 100*width");
		att->push("dotdash","5",            "an integer whose bits define an on-off pattern");
		att->push("width","1",              "width of the line");
		att->push("function","Over",        "Blend mode. Common is None or Over");

		return att;
	}
			
	const char *str;

	//att->push("mask %lu",mask);
	char scratch[200];
	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", color.Red(),color.Green(),color.Blue(),color.Alpha());
	att->push("color", scratch);

	if (capstyle==LAXCAP_Butt) str="butt";
	else if (capstyle==LAXCAP_Round) str="round";
 	else if (capstyle==LAXCAP_Projecting) str="projecting";
 	else if (capstyle==LAXCAP_Zero_Width) str="zero";
    else str="?";
	att->push("capstyle", str);

	if (endcapstyle !=0) {
		if      (endcapstyle == LAXCAP_Butt) str="butt";
		else if (endcapstyle == LAXCAP_Round) str="round";
		else if (endcapstyle == LAXCAP_Projecting) str="projecting";
		else if (endcapstyle == LAXCAP_Zero_Width) str="zero";
		else str="?";
		att->push("endcapstyle", str);
	}

	if (joinstyle==LAXJOIN_Miter) str="miter";
	else if (joinstyle==LAXJOIN_Round) str="round";
 	else if (joinstyle==LAXJOIN_Bevel) str="bevel";
 	else if (joinstyle==LAXJOIN_Extrapolate) str="extrapolate";
    else str="?";
	att->push("joinstyle",str);
	att->push("miterlimit",miterlimit);

	att->push("dotdash", dotdash);
	att->push("width", width);

	if (LaxopToString(function, scratch, 200, NULL)==NULL) {
		sprintf(scratch, "%d", function);
	}
	att->push("function", scratch);

	return att;
}

//! ***implement mask!! should only output the actually defined values?
void LineStyle::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	Attribute att;
	dump_out_atts(&att, what, context);
	att.dump_out(f,indent);

//	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
//	if (what==-1) {
//		//fprintf(f,"%smask                   #what is active in this linestyle\n", spc);
//		fprintf(f,"%scolor rgbaf(1,1,1,1)   #rgba in range [0..1]\n",spc);
//		fprintf(f,"%scapstyle round         #or miter, projecting, zero\n", spc);
//		fprintf(f,"%sjoinstyle round        #or miter, bevel, extrapolate\n",spc);
//		fprintf(f,"%smiterlimit 100         #means limit is 100*width\n",spc);
//		fprintf(f,"%sdotdash 5              #an integer whose bits define an on-off pattern\n",  spc);
//		fprintf(f,"%sfunction Over          #Blend mode. Common is None or Over\n", spc);
//		fprintf(f,"%swidth %.10g\n", spc,width);
//		return;
//	}
//
//	const char *str;
//
//	//fprintf(f,"%smask %lu\n", spc,mask);
//	fprintf(f,"%scolor rgbf(%.10g, %.10g, %.10g, %.10g)\n",spc, color.Red(),color.Green(),color.Blue(),color.Alpha());
//
//	if (capstyle==LAXCAP_Butt) str="butt";
//	else if (capstyle==LAXCAP_Round) str="round";
// 	else if (capstyle==LAXCAP_Projecting) str="projecting";
// 	else if (capstyle==LAXCAP_Zero_Width) str="zero";
//    else str="?";
//	fprintf(f,"%scapstyle %s\n", spc,str);
//
//	if (joinstyle==LAXJOIN_Miter) str="miter";
//	else if (joinstyle==LAXJOIN_Round) str="round";
// 	else if (joinstyle==LAXJOIN_Bevel) str="bevel";
// 	else if (joinstyle==LAXJOIN_Extrapolate) str="extrapolate";
//    else str="?";
//	fprintf(f,"%sjoinstyle %s\n",spc,str);
//	fprintf(f,"%smiterlimit %.10g\n",spc,miterlimit);
//
//	fprintf(f,"%sdotdash %d\n",  spc,dotdash);
//	fprintf(f,"%swidth %.10g\n", spc,width);
//
//	char op[50];
//	if (LaxopToString(function, op, 50, NULL) == NULL) {
//		sprintf(op, "%d", function);
//	}
//	fprintf(f,"%sfunction %s\n", spc,op);
}


//! Return whether we will actually be drawing a stroke.
/*! Currently, this returns 1 for when function!=LAXOP_Dest and width>0.
 */
int LineStyle::hasStroke()
{ return function!=LAXOP_Dest && function!=LAXOP_None && width>0; }



} // namespace LaxInterfaces

