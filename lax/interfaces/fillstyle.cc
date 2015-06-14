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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/interfaces/fillstyle.h>

using namespace LaxFiles;
using namespace Laxkit;


namespace LaxInterfaces {

/*! \class FillStyle
 * \ingroup interfaces
 * \brief Store various characteristics of a fill.
 *
 * This is the color used during a solid fill
 *
 * fillrule: see LaxFillRule\n
 * fillstyle (from Xlib): FillSolid, FillTiled, FillStippled, or FillOpaqueStippled, plus
 * FillNone (\#define as 100)\n
 * 
 * function is a LaxCompositeOp.
 */

FillStyle::FillStyle()
{
	function=Laxkit::LAXOP_Source;
	color.red=color.green=0;
	color.blue=color.alpha=0xffff;
	fillrule=LAXFILL_EvenOdd;
	fillstyle=FillSolid;
}

FillStyle::FillStyle(int r,int g,int b, int a,int fr,int fs,int f)
{
	color.red=r;
	color.green=g;
	color.blue=b;
	color.alpha=a;
	fillrule=fr;
	fillstyle=fs;
	function=f;
}

//! Set the color. Components are 0..0xffff.
void FillStyle::Color(int r,int g,int b,int a)
{
	color.red  =r;
	color.green=g;
	color.blue =b;
	color.alpha=a;
}

//! Reverse of dump_out.
void FillStyle::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context) 
{
	if (!att) return;
	char *name,*value;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"color")) {
			int i[4];
			if (IntListAttribute(value,i,4)==4) {
				color.red=i[0];
				color.green=i[1];
				color.blue=i[2];
				color.alpha=i[3];
			}
		} else if (!strcmp(name,"fillrule")) {
			if (!strcmp(value,"even"))         fillrule=LAXFILL_EvenOdd;
			else if (!strcmp(value,"odd"))     fillrule=LAXFILL_EvenOdd;
			else if (!strcmp(value,"nonzero")) fillrule=LAXFILL_Nonzero;
			else fillrule=WindingRule;
		} else if (!strcmp(name,"fillstyle")) {
			if (!strcmp(value,"none")) fillstyle=FillNone;
			//else if (!strcmp(value,"object")) fillstyle=FillObject; //for patterned fills
			else fillstyle=FillSolid;
		} else if (!strcmp(name,"function")) {
			if (!strcmp(value,"copy")) function=LAXOP_Source;
			else IntAttribute(value,&function);
		} else if (!strcmp(name,"mask")) {
			ULongAttribute(value,&mask);
		}
	}
}


/*! Puts something like:
 * <pre>
 *  color 65535 0 65535 65535
 *  fillrule odd
 *  fillstyle solid 
 *  function copy
 * </pre>
 *
 * Ignores what. Uses 0 for it (unless -1).
 */
void FillStyle::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (what==-1) {
		fprintf(f,"%scolor 0 0 65535 65535 #color of the fill\n",spc);
		fprintf(f,"%sfillrule nonzero  #or odd, or even\n", spc);
		fprintf(f,"%sfillstyle solid   #or none\n",spc);
		fprintf(f,"%sfunction copy     # (only copy is implemented)\n", spc);
		return;
	}
	fprintf(f,"%scolor %d %d %d %d\n",spc,color.red,color.green,color.blue,color.alpha);
	fprintf(f,"%sfillrule %s\n", spc,fillrule==LAXFILL_EvenOdd?"even":(fillrule==LAXFILL_Nonzero?"nonzero":"odd"));
	fprintf(f,"%sfillstyle %s\n",spc,fillstyle==FillSolid?"solid":"none"); //or "object"
	if (function==LAXOP_Source) fprintf(f,"%sfunction copy\n", spc);
	else fprintf(f,"%sfunction %d\n", spc,function);
}

//! Return whether the style will cause any fill or not.
int FillStyle::hasFill()
{
	return fillstyle==0 || function==LAXOP_Dest || fillstyle==FillNone;
}

} // namespace LaxInterfaces

