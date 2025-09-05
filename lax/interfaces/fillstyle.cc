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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/interfaces/fillstyle.h>
#include <lax/laxutils.h>


using namespace Laxkit;


namespace LaxInterfaces {

/*! \class FillStyle
 * \ingroup interfaces
 * \brief Store various characteristics of a fill.
 *
 * This is the color used during a solid fill
 *
 * fillrule: see LaxFillRule\n
 * fillstyle: a LaxFillStyle, or LAXFILL_None\n
 * 
 * function is a LaxCompositeOp.
 */

FillStyle::FillStyle()
{
	function = Laxkit::LAXOP_Over;

	color.red = color.green = 0;
	color.blue = color.alpha = 0xffff;
	color2 = nullptr;

	fillrule  = LAXFILL_EvenOdd;
	fillstyle = LAXFILL_Solid;

	mask = 0;
}

FillStyle::FillStyle(int r,int g,int b, int a,int fr,int fs,int f)
{
	color.red   = r;
	color.green = g;
	color.blue  = b;
	color.alpha = a;
	color2      = nullptr;
	fillrule    = fr;
	fillstyle   = fs;
	function    = f;
	mask        = 0;
}

FillStyle::FillStyle(const FillStyle &f)
 : FillStyle()
{
	color     = f.color;
	fillrule  = f.fillrule;
	fillstyle = f.fillstyle;
	function  = f.function;
	mask      = f.mask;
	if (f.color2) {
		color2 = f.color2->duplicate();
	}
}

FillStyle &FillStyle::operator=(FillStyle &f)
{
	color     = f.color;
	fillrule  = f.fillrule;
	fillstyle = f.fillstyle;
	function  = f.function;
	mask      = f.mask;
	if (f.color2) {
		color2 = f.color2->duplicate();
	}
	return f;
}

FillStyle::~FillStyle()
{
	if (color2) color2->dec_count();
}

anObject *FillStyle::duplicate(anObject *ref)
{
	FillStyle *style = new FillStyle(*this);
	return style;
}

//! Set the color. Components are 0..0xffff.
void FillStyle::Color(int r,int g,int b,int a)
{
	color.red   = r;
	color.green = g;
	color.blue  = b;
	color.alpha = a;
}

//! Set the color. Components are 0..1.0.
void FillStyle::Colorf(double r,double g,double b,double a)
{
	color.red   = r * 65535;
	color.green = g * 65535;
	color.blue  = b * 65535;
	color.alpha = a * 65535;
}

void FillStyle::Colorf(const Laxkit::ScreenColor &col)
{
	Colorf(col.Red(), col.Green(), col.Blue(), col.Alpha());
}

//! Reverse of dump_out.
void FillStyle::dump_in_atts(Attribute *att,int flag,Laxkit::DumpContext *context) 
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;

		if (!strcmp(name,"color")) {
			SimpleColorAttribute(value, NULL, &color, NULL);

		} else if (!strcmp(name,"fillrule")) {
			if (!strcmp(value,"even"))         fillrule=LAXFILL_EvenOdd;
			else if (!strcmp(value,"odd"))     fillrule=LAXFILL_EvenOdd;
			else if (!strcmp(value,"nonzero")) fillrule=LAXFILL_Nonzero;
			else fillrule=WindingRule;

		} else if (!strcmp(name,"fillstyle")) {
			if (!strcmp(value,"none")) fillstyle = LAXFILL_None;
			//else if (!strcmp(value,"object")) fillstyle=FillObject; //for patterned fills
			else fillstyle = LAXFILL_Solid;

		} else if (!strcmp(name,"function")) {
			function = StringToLaxop(value);

		} else if (!strcmp(name,"mask")) {
			ULongAttribute(value,&mask);
		}
	}
}

Laxkit::Attribute *FillStyle::dump_out_atts(Laxkit::Attribute *att,int what, Laxkit::DumpContext *context)
{
	if (!att) att=new Attribute;

	if (what==-1) {
		att->push("color","rgbaf(1,1,1,1)", "color of the fill");
		att->push("fillrule","nonzero", "or odd, or even");
		att->push("fillstyle","solid", "or none");
		att->push("function", "Over", "Blend mode. Common is None or Over");
		return att;
	}

	char scratch[200];
	sprintf(scratch, "rgbaf(%.10g, %.10g, %.10g, %.10g)", color.Red(),color.Green(),color.Blue(),color.Alpha());
	att->push("color", scratch);
	att->push("fillrule", fillrule == LAXFILL_EvenOdd ? "even" : (fillrule == LAXFILL_Nonzero?"nonzero":"odd"));
	att->push("fillstyle",fillstyle== LAXFILL_Solid ? "solid" : "none"); //or "object"

	if (LaxopToString(function, scratch, 200, NULL)==NULL) {
		sprintf(scratch, "%d", function);
	}
	att->push("function", scratch);

	return att;
}

void FillStyle::dump_out(FILE *f,int indent,int what,Laxkit::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%scolor rgbaf(1,1,1,1) #color of the fill\n",spc);
		fprintf(f,"%sfillrule nonzero     #or odd, or even\n", spc);
		fprintf(f,"%sfillstyle solid      #or none\n",spc);
		fprintf(f,"%sfunction Over        #Blend mode. Common is None or Over\n", spc);
		return;
	}

	fprintf(f,"%scolor rgbf(%.10g, %.10g, %.10g, %.10g)\n",spc, color.Red(),color.Green(),color.Blue(),color.Alpha());
	fprintf(f,"%sfillrule %s\n", spc, fillrule == LAXFILL_EvenOdd?"even":(fillrule==LAXFILL_Nonzero?"nonzero":"odd"));
	fprintf(f,"%sfillstyle %s\n",spc, fillstyle == LAXFILL_Solid ? "solid" : "none"); //or "object"

	char op[50];
	if (LaxopToString(function, op, 50, NULL) == NULL) {
		sprintf(op, "%d", function);
	}
	fprintf(f,"%sfunction %s\n", spc,op);
}


//! Return whether the style will cause any fill or not.
int FillStyle::hasFill()
{
	return !(function == LAXOP_Dest || function == LAXOP_None || fillstyle == LAXFILL_None);
}

/*! Returns old fill rule.
 */
int FillStyle::FillRule(int newrule)
{
	int old = fillrule;
	fillrule = newrule;
	return old;
}


} // namespace LaxInterfaces

