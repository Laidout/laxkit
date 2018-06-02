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
//    Copyright (C) 2004-2013 by Tom Lechner
//

#include <cstdlib>
#include <cctype>
#include <lax/attributes.h>
#include <lax/fileutils.h>
#include <lax/strmanip.h>
#include <lax/misc.h>
#include <lax/transformmath.h>
#include <lax/colors.h>

#include <unistd.h>
#include <sys/file.h>
#include <limits.h>
#include <iostream>


//template implementation:
#include <lax/lists.cc>


using namespace std;
#define DBG 


using namespace Laxkit;

namespace LaxFiles {

/*! \defgroup attributefileformat Indented Data File Format
 * 
 * The file format used by the LaxFiles::Attribute class is based on indentation delimiting
 * different fields. You can think of it as a simplified xml, where there is no
 * difference between elements and attributes. The 
 * <a href="laxrc.html">laxrc</a> file uses this structure, as do all the
 * <a href="group__interfaces.html">interface</a> data
 * classes. Please see the interface classes for examples of how Attributes are
 * used in practice.
 *
 * \todo ****** the IntAttribute types of functions use functions like strtol, which will
 *   stop parsing at first character that is invalid for the conversion, but for a value
 *   to be truly converted, "34" is ok but "34g" is not a pure integer, for instance. there
 *   is no default check for things like that. It should be an option.
 * 
 * \todo update this!!!
 *
 * ~~~~~~~~~~ The following text should be considered mere brainstorming:  ~~~~~~~~~~
 * 
 * *** please note that not all this is currently implemented... at the moment is only the very basic
 * name=value, subatts is in place, not the definition parsing and applying..
 * 
 * Attributes are basically a name,
 * with a value, and any number of sub-entries. The sub entries can either exist only in the entry,
 * or they can be references to other entries (todo!). Like python, related information is indicated by
 * a certain level of indentation, unlike XML, for instance, that uses 
 * &quot;&lt;tags&gt;&quot; that are often difficult to read through quickly.
 * 
 * News flash: Just found out about Yaml.org, which is a much more developed 
 * indentation based data file format, must investigate that further!
 *
 * \section types Types and Definitions
 * 
 * ***this is not current:
 * 
 * Attributes can be read in assuming the basic "name value" or they can be
 * specific types.
 * Base level entries are begun on a single line. This line has an optional type 
 * specifier first (no indenting), followed by an entry name, followed by an 
 * optional date specification, followed by an optional value field.
 *
 * If type specifier is not present, then the entry is assumed to be just a generic Attribute.
 * Otherwise, first the entry is loaded with any default values for that type, and then any values
 * or subattributes listed overwrite those default values.
 * 
 * It is assumed that the date has the format 
 * <tt>yyyy-mm-dd-hh:mm:ss.sssss</tt>. If it finds something that looks like "yyyy-mm-dd", then
 * it assumes this is the date specifier. Otherwise that bit is read in as
 * the start of the value. The parser will read in all of the date that is there.
 * If you have a value that starts off like that, and you do not include a date specification,
 * then you must put quotes around it.
 *
 * \todo ***need some way to automatically set how many spaces a tab is equal to.
 *   it is currently 1, which is irritating when your editor automatically puts
 *   in a tab rather than spaces..
 * \todo dump_in_from_buffer(const char *buf) and dump_out_to_buffer(char *buf) ?
 * 
\verbatim
# comment
name value
   subattname value
   
[entrytype] entryname [value]
   attribute1 value
   attribute2 value
   attribute2.5 "1" "2" \"value\""  #<-- multiple quote blocks, value= '"all in" "quotes is the \"value\""'
   attribute3 "all in quotes is the \"value\"" #<-- \" escapes the quote, value='all in quotes is the \"value\"'
     subattribute1 value
     subattribute2 value
       subsubatt1 \ #<-- means the value is contained in the following lines
          That initial '\' next to the name must be followed by either a newline or a space. 
          Otherwise you might simply escape a '\#' character..
          Value for subsubatt1 continues
          as long there are lines with the same indentation \#this is included
          as the first line after the name. #this comment is not included
          On any of these lines, trailing whitespace and comments are stripped.
            Initial whitespace is included.
          .
          Blank lines are written with the correct indentation and a single '.'.
          If you need a line with only a '.', then you can write '\.'. If you need a line
          with '\.', then write '\\.'. A line with '\\.' is written as "\\\."...

          subsubsubatt1 "The previous line broke reading that value, so this is read as a sub att.."
       subsubatt2 <<< filename #<-- the value is the contents of file filename
       subsubatt3 << BLAH  #<-(comment not included in tag) 
All the characters following the newline of the previous line until that BLAH on its own line
is encountered become the value of subsubatt2, not including any newline just before the blah.

Blank lines like the previous line are included.
The end marker doesn't have to be 'BLAH'. It can be anything without white space. #this is included.
BLAH
       subsubatt4 < BLAH
          Like the raw value in the previous attribute, this goes until BLAH on its own line
          is encountered, but each line (non-empty) must have the standard indentation.
          
          Blank lines need only be blank lines, proper indentation is not necessary. 
          Comments and trailing whitespace are not stripped, so #this is included
          Really except for the indent, this is identical to the raw 
          value above, but it makes a file looks nicer.
          BLAH
          subsubsubatt1 "subatt of subsubatt4"
   attribute4 value
   anotherentry  #<-- this places a reference to the entry with name anotherentry within this entry
                 #    if it exists above, otherwise it is a field with null value.
\endverbatim
 *
 * How to define default types within a file:
 * 
 * int, uint (non-negative int), pint (positive integer),\n
 * string, date(FMTSTRING???) \n
 * real[minval,maxval]\n
 *
\verbatim

define image
  filename: string untitled
  width: uint 0            #<--- type plus default value
  height 0                 #<--- (no colon) string type, default="0"
  caption "No caption"     #<--- (no colon) string type, default="No caption"
  referencecount: uint
  referencecount= uint #?? = instead of :?
  real1to10: real:[1,10] #<-- allow range 1 to 10, no default value, must be defined manually
  anotherreal1to10: real(5):[1,10] #<-- allow range 1 to 10, default value of 5, must be defined manually
  areal: real1to10(8)

default image
  filename "blah.jpg"
  caption "Default image"

image animage
  filename "yadda.jpg" 
   # animage then would dump out like this:
   # animage
   #   filename "yadda.jpg"
   #   caption  "Default image"
   #   width    0
   #   height   "0"
   #   areal    8
   #
\endverbatim
 *   
 */

//---------------------------------- Value Conversion Routines -----------------------------------	

//! Convert strings like "24kb" and "3M" to a number.
/*! \ingroup attributes
 *
 * "never" gets translated to INT_MAX. "34" becomes 34 kilobytes
 *
 * Return 0 for success or nonzero for error in parsing. If there is 
 * an error, ll gets 0.
 *
 * Really this is pretty simple check. Looks for "number order" where
 * order is some text that begins with 'm' for megabytes, 'g' for gigabytes
 * or 'k' for kilobytes
 *
 * If towhat=='k' or 'K' returns
 * in kilobytes, 'm' or 'M' megabytes, and 'g' or 'G' in gigabytes.
 */
int ByteSizeAttribute(const char *s, long *ll, char towhat)
{
	char *str=newstr(s);
	stripws(str);
	if (!strcasecmp(str,"never")) {
		delete[] str;
		if (ll) *ll=INT_MAX;
		return 0;
	}
	char *e;
	long l=strtol(str,&e,10);//supposedly, e will never be NULL
	if (e==str) {
		delete[] str;
		if (ll) *ll=0;
		return 1;
	}
	while (isspace(*e)) e++;
	if (*e) {
		if (*e=='m' || *e=='M') l*=1024;
		else if (*e=='g' || *e=='G') l*=1024*1024;
		else if (*e=='k' || *e=='K') ; //kb is default
		else {
			 //unknown units
			delete[] str;
			if (ll) *ll=0;
			return 2;
		}
	}
	if (towhat=='m' || towhat=='M') l/=1024;
	else if (towhat=='g' || towhat=='G') l/=1024*1024;

	delete[] str;
	if (ll) *ll=l;
	return 0;
}

//! Turn v into a double, put in d if successful, return 1. Else don't change d and return 0.
/*! \ingroup attributes
 */
int DoubleAttribute(const char *v,double *d,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	double dd=strtod(v,&e);
	if (endptr) *endptr=e;
	if (e==v) return 0;
	*d=dd;
	return 1;
}

//! Turn v into a double[] of maximum size n. d must already exist. Returns number of items parsed.
/*!
 * \ingroup attributes
 * Put in d[] if successful, return number of items parsed. Else don't change d's elements and return 0.
 *
 * The numbers must be separated by whitespace or a comma.
 */
int DoubleListAttribute(const char *v,double *d,int maxn,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	int n=0;
	double dd;
	while (n<maxn && v && *v) {
		dd=strtod(v,&e);
		if (e==v) break;;
		d[n++]=dd;
		v=e;
		while (isspace(*v) || *v==',') v++;
	}
	if (endptr) *endptr=e;
	return n;
}

//! Turn v into a new double[], and put it in d. Put the number of values in n_ret.
/*! \ingroup attributes
 * Put in d if successful, return number of items parsed. Else don't change d and return 0.
 * The numbers must be separated by whitespace.
 *
 * The first part unparsable into an int breaks the reading. The rest of the string is ignored.
 *
 * Returns the number of doubles successfully parsed. Please note that if there is an error,
 * 0 is returned, but n_ret is not changed.
 */
int DoubleListAttribute(const char *v,double **d,int *n_ret)
{
	if (!v) return 0;
	char *str=newstr(v);
	int n=0;
	char **fields=splitonspace(str,&n);
	if (!n) { if (n_ret) *n_ret=0; *d=NULL; return 0; }
	char *e;
	int nn=0;
	double dd,*ddd=new double[n];
	for (int c=0; c<n; c++) {
		v=fields[c];
		dd=(double)strtod(v,&e);
		if (e==v) break;
		ddd[nn++]=dd;
	}
	if (nn) {
		*d=ddd;
		if (n_ret) *n_ret=nn;
	} else {
		delete[] ddd;
		*d=NULL;
		if (n_ret) *n_ret=0;
	}
	delete[] fields;
	return nn;
}

//! Turn v into a float, put in f if successful, return 1. Else don't change f and return 0.
/*! \ingroup attributes
 */
int FloatAttribute(const char *v,float *f,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	float ff=strtod(v,&e);
	if (endptr) *endptr=e;
	if (e==v) return 0;
	*f=ff;
	return 1;
}

//! Turn v into an unsigned int, put in i if successful, return 1. Else don't change i and return 0.
/*! \ingroup attributes
 */
int UIntAttribute(const char *v,unsigned int *i,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	int ii=(int)strtol(v,&e,10);
	if (endptr) *endptr=e;
	if (e==v) return 0;
	*i=ii;
	return 1;
}

//! Turn v into an int, put in i if successful, return 1. Else don't change i and return 0.
/*! \ingroup attributes
 *
 * \todo warning, this will parse "200jjj" as 200, and return 1.
 */
int IntAttribute(const char *v,int *i,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	int ii=(int)strtol(v,&e,10);
	if (endptr) *endptr=e;
	if (e==v) return 0;
	*i=ii;
	return 1;
}

//! Figure out if value is yes==true==1 or no==false==0. Return the boolean value.
/*! If value==NULL or value=="" then assume true. This is usually what is
 * wanted since it would be something like name="someflag" and no value means
 * that the flag simply exists.
 *
 * If v is "true" or "yes" (regarless of case), return 1. If "false" or "no", returns 0.
 * Otherwise, If atol(v) returns nonzero, return 1, else 0.
 */
int BooleanAttribute(const char *v)
{
	if (!v || !strcmp(v,"")) return 1;
	if (!strcasecmp(v,"true")  || !strcasecmp(v,"yes") ) return 1;
	if (!strcasecmp(v,"false") || !strcasecmp(v,"no") ) return 0;
	if (atol(v)) return 1;
	return 0;
}

//! Find an ARGB color in v, and return in either an unsigned long and/or a ScreenColor.
/*! A, R, G, B are found from color_ret by masking with respectively
 * 0xff000000, 0xff0000, 0xff00, and 0xff.
 *
 * See SimpleColorAttribute(const char *, double *, const char **)
 * for more detail on how v should be structured.
 * This function just converts output from that other one.
 */
int SimpleColorAttribute(const char *v,unsigned long *color_ret, Laxkit::ScreenColor *scolor_ret, const char **end_ptr)
{
	double colors[5];
	const char *endpp=NULL;
	int status = SimpleColorAttribute(v, colors, &endpp);
	if (status != 0) return status;
	if (end_ptr) *end_ptr=endpp;

	if (color_ret) {
		*color_ret = (int(colors[0]*255+.5)<<16)
				   | (int(colors[1]*255+.5)<<8)
				   | (int(colors[2]*255+.5))
				   | (int(colors[3]*255+.5)<<24);
	}

	if (scolor_ret) {
		scolor_ret->rgbf(colors[0], colors[1], colors[2], colors[3]);
	}

	return 0;
}

/*! Colors should be enough to potentially hold 5 doubles.
 * Returned value is rgba clamped to [0..1].
 *
 * The value can be something like "black", "white", "red", "green", "blue", "yellow", "orange",
 * "purple". "transparent" maps to transparent black.
 *
 * Also, it can be something like any of the following:
 * <pre>
 *  #112233ff           <-- uses HexColorAttributeRGB
 *  rgba16(65535,65535,65535,65535)
 *  rgb(255,255,255)
 *  rgba(255,255,255,1.0) ***
 *  rgba(50%,255,255,1.0) ***
 *  rgbf( 1.0, .5, .5)
 *  rgbaf(1.0, .5, .5, 1.0)
 *  gray(255)
 *  grayf(.5)
 *  cmykf(.8, .9, .1, .2)
 *  hsl(360,20%,10%)
 *  65535,65535,65535,65535     #<-- simple list like this is assumed to be 16 bit rgba.
 * </pre>
 *  
 * These forms follow a pattern. They have to start with rgb, rgba, gray, graya, cmyk, cmyka, hsl, hsla,
 * ignoring case. The gray ones will return an rgb with each color field equal to the gray value.
 * The cmyk and hsl will be converted to rgb with a simple reciprocal function (not ICC based).
 *
 * Following the letters should be 'f' for floats in range [0..1],
 * '8' for 8 bit [0..255], "16" for 16 bit numbers [0..65535].
 * If f, 8, or 16 are missing, assume v mostly follows CSS3 color format for that label. That means alpha values
 * are always in range [0..1], the first number of hsl is [0..360], and otherwise assume
 * fields in range [0..255] or [0%..100%].
 *
 * Note first value of hsl is ALWAYS [0..360].
 *
 * A bare list of numbers is assumed to be a list of 4 16bit rgba numbers.
 * 
 * Returns 0 for successful parsing, and color is returned.
 * Returns 1 for unsuccessful for failure, and colors not changed.
 *
 * \todo make like css, and allow like rgba(50%, 20%, 10%, .1),
 *       css opacity is always [0..1].
 *       also hsl(hue,saturation,lightness) or hsla(), hue is [0..360), with r,g,b at 0,120,240.
 *       The others are [0..255] or percents.
 */
int SimpleColorAttribute(const char *v, double *colors, const char **end_ptr)
{
	while (isspace(*v)) v++;


	int type=0;
	int numcc=0; //3=rgb or hsl, 1=gray, 4=cmyk

	if (*v=='#') {
		ScreenColor color;
		if (HexColorAttributeRGB(v, &color, end_ptr)==0) return 1;
		colors[0] = color.Red();
		colors[1] = color.Green();
		colors[2] = color.Blue();
		colors[3] = color.Alpha();
		return 0;
	}

	if (strcasestr(v,"rgb")==v) {
		v+=3;
		numcc=3;
		type=LAX_COLOR_RGB;

	} else if (strcasestr(v,"gray")==v) {
		v+=4;
		numcc=1;
		type=LAX_COLOR_GRAY;

	} else if (strcasestr(v,"cmyk")==v) {
		v+=4;
		numcc=4;
		type=LAX_COLOR_CMYK;

	} else if (strcasestr(v,"hsl")==v) {
		v+=3;
		numcc=3;
		type=LAX_COLOR_HSL;

	} else if (!isdigit(*v)) {
		 //is same name, so check css like named colors, assume fully opaque
		int r=-1,g,b;
		int alpha=0xff;
		if      (!strncasecmp(v,"transparent",11))  { r=0x00; g=0x00; b=0x00; alpha=0x00; v+=11; }
		else if (!strncasecmp(v,"maroon",6))  { r=0x80; g=0x00; b=0x00; v+=6; }
		else if (!strncasecmp(v,"red",3))     { r=0xff; g=0x00; b=0x00; v+=3; }
		else if (!strncasecmp(v,"orange",6))  { r=0xff; g=0xA5; b=0x00; v+=6; }
		else if (!strncasecmp(v,"yellow",6))  { r=0xff; g=0xff; b=0x00; v+=6; }
		else if (!strncasecmp(v,"olive",5))   { r=0x80; g=0x80; b=0x00; v+=5; }
		else if (!strncasecmp(v,"purple",6))  { r=0x80; g=0x00; b=0x80; v+=6; }
		else if (!strncasecmp(v,"fuchsia",7)) { r=0xff; g=0x00; b=0xff; v+=7; }
		else if (!strncasecmp(v,"white",5))   { r=0xff; g=0xff; b=0xff; v+=5; }
		else if (!strncasecmp(v,"black",5))   { r=0x00; g=0x00; b=0x00; v+=5; }
		else if (!strncasecmp(v,"lime",4))    { r=0x00; g=0xff; b=0x00; v+=4; }
		else if (!strncasecmp(v,"green",5))   { r=0x00; g=0x80; b=0x00; v+=5; }
		else if (!strncasecmp(v,"navy",4))    { r=0x00; g=0x00; b=0x80; v+=4; }
		else if (!strncasecmp(v,"blue",4))    { r=0x00; g=0x00; b=0xff; v+=4; }
		else if (!strncasecmp(v,"aqua",4))    { r=0x00; g=0xff; b=0xff; v+=4; }
		else if (!strncasecmp(v,"teal",4))    { r=0x00; g=0x80; b=0x80; v+=4; }
		else if (!strncasecmp(v,"cyan",4))    { r=0x00; g=0xff; b=0xff; v+=4; } //not css, but is x11 color, widely accepted

		if (r>=0) {
			colors[0]=r/255.;
			colors[1]=g/255.;
			colors[2]=b/255.;
			colors[3]=alpha/255.;
			if (end_ptr) *end_ptr=v;
			return 0;
		}

		cerr << " *** could not parse svg color: "<<v<<endl;
		return 1;
	}


	int a=0;  //has alpha
	int f=-1; //is float==0, is css==1, or 8, or 16bit

	if (strchr(v,'.')!=NULL) f=0; //assume floats if has decimal points

	if (type!=0 && (*v=='a' || *v=='A')) { a=1; v++; } // has alpha
	if (type!=0 && (*v=='f' || *v=='F')) { f=0; v++; } // read in floats
	else if (type!=0 && *v=='8') { f=8; v++; }
	else if (type!=0 && v[0]=='1' && v[1]=='6') { f=16; v+=2; }
	if (f==-1) {
		 //no specific bit format found, try to guess..
		if (type!=0) f=1; //for label found, assume numbers inside follow css3, which is mostly 8 bit
		else f=16; //for no label found, is a bare list of numbers, assume 16 bit
	}

	bool hasparen= (*v=='(');
	if (hasparen) v++;

	int numf=0;
	char *e;
	double dd;
	double d[5];
	while (numf<5 && v && *v) {
		dd=strtod(v,&e);
		if (e==v) break;
		d[numf]=dd;
		v=e;
		while (isspace(*v)) v++;

		 //convert all fields to 0..1
		if (*v=='%') {  d[numf]/=100; v++; }
		else if (numf==0 && type==LAX_COLOR_HSL) d[numf]/=360; //first of hsl always 0..360
		else if (f==1) {
			if (numcc>0 && numcc==numf) ; //assume css opacity already in 0..1
			else d[numf]/=255; //otherwise assume was in range 0..255. Percents handled above
		} else if (f==8)  d[numf]/=255;
		else   if (f==16) d[numf]/=65535;
		numf++;

		while (isspace(*v) || *v==',') v++;
	}

	if (type==0) { //adapt to 3 or 4 fields when only a number list was supplied, assume rgb or rgba
		type=LAX_COLOR_RGB;
		numcc=3;
		if (numf==4) a=1;
	}
	if (a==0 && numf==numcc+1) a=1; //assume having alpha is implied, so rgbf and rgbaf both can have alpha
	if (numf!=numcc+a) {
		return 1;
	}

	 //clamp values to [0..1]. hsl has special conversion to divide by 360.
	for (int cc=0; cc<numf; cc++) {
		colors[cc]=d[cc];
		if (colors[cc]<0) colors[cc]=0;
		else if (colors[cc]>1.0) colors[cc]=1.0;
	}

	 //now convert to rgb
	if (type==LAX_COLOR_RGB) {
		 //rgb
		if (!a) colors[3]=1.0; //make fully opaque if alpha field not provided

	} else if (type==LAX_COLOR_GRAY) {
		 //gray
		if (!a) colors[3]=1.0; //make fully opaque if alpha field not provided
		else colors[3]=colors[1];
		colors[1]=colors[2]=colors[0];

	} else if (type==LAX_COLOR_HSL) {
		 //hsl
		simple_hsl_to_rgb(colors[0], colors[1], colors[2], &colors[0], &colors[1], &colors[2]);
		if (!a) colors[3]=1.0; //make fully opaque if alpha field not provided

	} else {
		 //cmyk
		double rgb[4];
		Laxkit::simple_cmyk_to_rgb(colors, rgb);
		if (!a) colors[3]=1.0; //make fully opaque if alpha field not provided
		else colors[3]=colors[4];
		colors[0]=rgb[0];
		colors[1]=rgb[1];
		colors[2]=rgb[2];
	}

	if (hasparen) {
		while (isspace(*v)) v++;
		if (*v==')') v++;
	}
	if (end_ptr) *end_ptr=v;
	return 0;
}

/*! Read in an rgb value such as "af3" or "ff00ff".
 * The hex format can have an optional initial '#'.
 * Beyond that, the acceptable formats are:
 *   rgb, argb, rrggbb, aarrggbb, rrrrggggbbbb, aaaarrrrggggbbbb.
 *
 * Return 1 if successful, or 0 if unable to parse.
 */
int HexColorAttributeRGB(const char *value,Laxkit::ScreenColor *color,const char **endptr)
{
	if (!color) return 0;

	const char *v=value;
	if (*v=='#') v++;
	int n=0;
	int r,g,b,a;
	while (isxdigit(v[n])) n++;
	char *end=NULL;
	unsigned long num=strtol(v,&end,16);
	if (end==v) {
		 //wasn't a number there to parse!
		if (endptr) *endptr=value;
		return 0;
	}

	if (endptr) *endptr=value+n;

	if (n==3 || n==4) { //rgb or argb
		b= ((num&0xf)  <<4)|(num&0xf);
		g=(((num&0xf0) <<4)|(num&0xf0))>>4;
		r=(((num&0xf00)<<4)|(num&0xf00))>>8;
		if (n==4) a=(((num&0xf000)<<4)|(num&0xf000))>>12;
		else a=255;

		color->red  =(r<<8)|r; //scaling up to 16 bit
		color->green=(g<<8)|g;
		color->blue =(b<<8)|b;
		color->alpha=(a<<8)|a;
		color->pixel=color->Pixel();
		return 1;

	} else if (n==6 || n==8) { //rrggbb or aarrggbb
		b=(num&0xff);
		g=(num&0xff00)>>8;
		r=(num&0xff0000)>>16;
		if (n==8) a=(num&0xff000000)>>24; else a=255;

		color->red  =(r<<8)|r; //scaling up to 16 bit
		color->green=(g<<8)|g;
		color->blue =(b<<8)|b;
		color->alpha=(a<<8)|a;
		color->pixel=color->Pixel();
		return 1;

	} else if (n==12 || n==16) { //rrrrggggbbbb or aaaarrrrggggbbbb
		char s[5];
		s[0]=v[0]; s[1]=v[1]; s[2]=v[2]; s[3]=v[3]; s[4]='\0';
		if (n==16) {
			color->alpha=strtol(s,&end,16);
			v+=4;
		} else color->red=strtol(s,&end,16);

		s[0]=v[0]; s[1]=v[1]; s[2]=v[2]; s[3]=v[3]; s[4]='\0';
		color->red=strtol(s,&end,16);

		s[0]=v[4]; s[1]=v[5]; s[2]=v[6]; s[3]=v[7]; s[4]='\0';
		color->green=strtol(s,&end,16);

		s[0]=v[8]; s[1]=v[9]; s[2]=v[10]; s[3]=v[11]; s[4]='\0';
		color->blue=strtol(s,&end,16);

		color->pixel=color->Pixel();
		return 1;
	}

	if (endptr) *endptr=value;
	return 0;
}

//! Read in an rgb value such as "af3" or "ff00ff".
/*! Return 1 if successful, or 0 if unable to parse.
 */
int HexColorAttributeRGB(const char *v,unsigned long *l,const char **endptr)
{
	ScreenColor color;
	if (HexColorAttributeRGB(v,&color,endptr)==0) return 0;
	*l=color.Pixel();
	return 1;
}

//! Return from something like "1 2", "1,2", "(1,2)", or "(1 2)".
/*! Return 1 if successful, else 0.
 */
int FlatvectorAttribute(const char *v,flatvector *vec,char **endptr)
{
	while (isspace(*v)) v++;
	int paren=(*v=='(');
	if (paren) v++;
	char *end;
	double fv[2];
	int n=DoubleListAttribute(v,fv,2,&end);
	if (n!=2) return 0;
	v=end;
	while (isspace(*v)) v++;
	if (paren && *v!=')') return 0; //need closing parenthesis!
	else if (paren) v++;
	if (endptr) *endptr=const_cast<char *>(v);
	(*vec).x=fv[0]; (*vec).y=fv[1];
	return 1;
}

//! Return from something like "1 2 3", "1,2,3", "(1,2,3)", or "(1 2 3)".
/*! Return 1 if successful, else 0.
 */
int SpacevectorAttribute(const char *v,spacevector *vec,char **endptr)
{
	while (isspace(*v)) v++;
	int paren=(*v=='(');
	if (paren) v++;
	char *end;
	double fv[3];
	int n=DoubleListAttribute(v,fv,3,&end);
	if (n!=3) return 0;
	v=end;
	while (isspace(*v)) v++;
	if (paren && *v!=')') return 0; //need closing parenthesis!
	else if (paren) v++;
	if (endptr) *endptr=const_cast<char *>(v);
	(*vec).x=fv[0]; (*vec).y=fv[1]; (*vec).z=fv[2];
	return 1;
}

//! Return from something like "1 2 3 4", "1,2,3,4", "(1,2,3,4)", or "(1 2 3 4)".
/*! Return 1 if successful, else 0.
 */
int QuaternionAttribute(const char *v,Quaternion *vec,char **endptr)
{
	while (isspace(*v)) v++;
	int paren=(*v=='(');
	if (paren) v++;
	char *end;
	double fv[4];
	int n=DoubleListAttribute(v,fv,4,&end);
	if (n!=4) return 0;
	v=end;
	while (isspace(*v)) v++;
	if (paren && *v!=')') return 0; //need closing parenthesis!
	else if (paren) v++;
	if (endptr) *endptr=const_cast<char *>(v);
	(*vec).x=fv[0]; (*vec).y=fv[1]; (*vec).z=fv[2]; (*vec).z=fv[3];
	return 1;
}

//! Turn v into an unsigned long, put in l if successful, return 1. Else don't change l and return 0.
/*! \ingroup attributes
 */
int ULongAttribute(const char *v,unsigned long *l,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	unsigned long ll=(unsigned long)strtol(v,&e,10);
	if (endptr) *endptr=e;
	if (e==v) return 0;
	*l=ll;
	return 1;
}

//! Turn v into a long, put in l if successful.
/*! \ingroup attributes
 * Return 1 if successful. Else don't change l and return 0.
 * endptr will point to either the first invalid char, or to v.
 */
int LongAttribute(const char *v,long *l,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	long ll=strtol(v,&e,10);
	if (endptr) *endptr=e;
	if (e==v) return 0;
	*l=ll;
	return 1;
}

//! Turn v into an int[] of maximum size n. Return the number of integers parsed.
/*! \ingroup attributes
 * Put in d[] if successful, return number of items parsed. Else don't change d's elements and return 0.
 * The numbers must be separated by whitespace.
 */
int IntListAttribute(const char *v,int *i,int maxn,char **endptr)//endptr=NULL
{
	if (!v) return 0;
	char *e;
	int n=0;
	int ii;
	while (n<maxn && v && *v) {
		ii=(int)strtol(v,&e,10);
		if (e==v) break;
		i[n++]=ii;
		v=e;
		while(isspace(*v)) v++;
		if (*v==',') v++;
	}
	if (endptr) *endptr=e;
	return n;
}

//! Turn v into a new int[], and put it in i. Put the number of values in n_ret.
/*! \ingroup attributes
 * Put in i if successful, return number of items parsed. Else point i to NULL and return 0.
 * The numbers must be separated by whitespace.
 *
 * The first part unparsable into an int breaks the reading. The rest of the string is ignored.
 *
 * Returns the number of ints successfully parsed. This is the same number put in n_ret, if
 * n_ret!=NULL.
 *
 * \todo *** need to do some testing on this
 */
int IntListAttribute(const char *v,int **i,int *n_ret,char **endptr)
{
	if (!v) return 0;
	
	int *ii=new int[5],
		max=5,
		c,
		n=0;
	const char *vv=v;
	char *e=NULL;
	
	while (1) {
		c=IntListAttribute(vv,ii+max-5,5,&e);
		if (e==vv) break; //no values read
		n+=c;
		if (c<5) break; // definitely reached end of parseable ints
		int *iii=new int[max+5];
		memcpy(iii,ii,max*sizeof(int));
		delete[] ii;
		ii=iii;
		max+=5;
		vv=e;
	}
	*i=ii;
	if (endptr) *endptr=e;
	if (n_ret) *n_ret=n;
	return n;
}

//! Return a double[6] transform derived from v.
/*! If m!=NULL, then put the result there. Otherwise, return a new double[6].
 *
 * The transform can be a simple list of 6 real numbers or an SVG style transform.
 * In the first case, less than 6 numbers will return NULL. Parsing stops after those
 * 6 numbers.
 * 
 * The SVG style transform is composed of a list of chunks of:
 *  "translate(3 5)",
 *  "scale(3,2)",
 *  "rotate(3.1415, 5,6)",
 *  "matrix(1 0 0 1 0 0)",
 *  "skewX(4)",
 *  "skewy(4)".
 *  The numbers inside the parentheses can be separated by whitespace or by a comma, and for
 *  translate, rotate, and scale, all the numbers after the 1st are optional.    
 *  Parsing will stop at the first chunk not recognized as a further transform element.
 *
 * If there is an error, NULL is returned, else m is returned. If m is NULL, then a new
 * double[6] is returned.
 *
 *  \todo *** implement me!
 */
double *TransformAttribute(const char *v,double *m,char **endptr)
{
	double matrix[6];
	if (!m) m=matrix;

	while (isspace(*v)) v++;
	if (isdigit(*v) || *v=='-' || *v=='.') {
		 //found a number, so assume a list of 6 numbers
		int n=DoubleListAttribute(v,matrix,6,endptr);
		if (n!=6) return NULL;
		if (m==matrix) {
			m=new double[6];
			transform_copy(m,matrix);
		}
		return m;
	}

	 //else we assume svg transform attribute
	 //Please note this is not very strict parsing. corrupt input may slip through
	transform_identity(matrix);
	double d[6], tt[6];
	const char *vv;
	char *ee=NULL;
	int n;
	while (*v) {
		while (isspace(*v) || *v==',') v++;

		vv=v;
		if (!isalpha(*v)) break; //end of list!

		while (isalpha(*v)) v++;
		if (*v=='(') v++;
		n=DoubleListAttribute(v,tt,6,&ee);
		if (n==0 || ee==v) break; //sudden ending after a word!! this is an error!
		v=ee;
		while (isspace(*v)) v++;
		if (*v==')') v++;

		transform_identity(d);

		if (!strncmp(vv,"matrix",6)) {
			if (n!=6) break;
			transform_copy(d,tt);

		} else if (!strncmp(vv,"translate",9)) {
			if (n!=2) tt[1]=0;
			transform_set(d, 1,0,0,1, tt[0],tt[1]);

		} else if (!strncmp(vv,"scale",5)) {
			double sx=d[0], sy=d[1];
			if (n!=2) sy=sx;
			transform_set(d, sx,0,0,sy,0,0);

		} else if (!strncmp(vv,"rotate",6)) {
			double a=tt[0]/180.*M_PI;
			if (n==3) {
				 //rotate around point
				double cx=tt[1], cy=tt[2];
				transform_set(d, 1,0,0,1, cx,cy);
				transform_mult(tt, d,matrix);
				transform_copy(matrix,tt);
				transform_set(d, cos(a),sin(a),-sin(a),cos(a), 0,0);
				transform_mult(tt, d,matrix);
				transform_copy(matrix,tt);
				transform_set(d, 1,0,0,1, -cx,-cy);
			} else {
				transform_set(d, cos(a),sin(a),-sin(a),cos(a), 0,0);
			}

		} else if (!strncmp(vv,"skewX",5)) {
			transform_set(d, 1,0,tan(tt[0]/180.*M_PI),1, 0,0);

		} else if (!strncmp(vv,"skewY",5)) {
			transform_set(d, 1,tan(tt[0]/180.*M_PI),0,1, 0,0);

		} else {
			 //found bad name!!
			break;
		}

		// d * matrix -> matrix
		transform_mult(tt, d,matrix);
		transform_copy(matrix,tt);
	}

	if (m==matrix) m=new double[6];
	transform_copy(m,matrix);

	if (endptr) *endptr=const_cast<char *>(v);
	return m;
}

//! Grab the first chunk of str, return as new char[].
/*! For instance, a string: ' "1\"2\"3" "4" 5 ' would return s='1"2"3', and endptr would point to the
 * quote character before the 4. If there are no initial quotes, then the first
 * whitespace delimited chunk is returned.
 * 
 * If there are quotes, then '\\n' becomes a newline and '\\t' becomes a tab, '\\' becomes a backslash,
 * and '\"' becomes one doublequote. In fact any other character following the backslash just inserts
 * that character.
 *
 * Quotes can be '"' or '\''.
 *
 * On error or no first chunk, NULL is returned.
 *
 * Please note that it is assumed that there is no trailing comment in v. That is, there
 * is no final chunk of text beginning with a '#' character.
 *
 * \todo ****** on error, what happens to endptr?
 * \todo **** all of v is copied right at the beginning, wasting much memory, should just crawl
 *   along v, copying chars to a smaller buf, reallocating that if necessary?
 */
char *QuotedAttribute(const char *v,char **endptr)
{
	if (!v) return NULL;
	char *s=new char[strlen(v)+1],*sp=s;
	const char *p=v;
	char quote=0;

	while (isspace(*p)) p++;

	if (*p!='"' && *p!='\'') {
		 // no quotes, just grab the first ws delimited chunk
		while (*p && !isspace(*p)) {
			*sp=*p;
			sp++;
			p++;
		}

	} else {
		 // there were initial quotes..
		quote=*p;
		p++; //skip initial quote
		while (1) {
			while (*p && *p!='\\' && *p!=quote) { *sp=*p; sp++; p++; }
			if (*p==quote) { //endquote
				p++;
				while (isspace(*p)) p++;
				break;
			}
			if (!*p) break;
			if (!*(p+1)) { *sp++=*p++; break; }
			p++;
			if (*p=='n') *sp++='\n';
			else if (*p=='t') *sp++='\t';
			else *sp++=*p;
			p++;
		}
	}
	
	*sp='\0';
	if (endptr) *endptr=const_cast<char *>(p);
	return s;
}

//! Like QuotedAttribute(), but assumes that the whole thing is meant as only one chunk.
/*! Leading and trailing whitespace is stripped.
 *
 * QuotedAttribute() will decompose something like '"1" "2"' into '1' and the rest
 * is '"2"', but this will return the whole '"1" "2"', without stripping the quotes.
 * But, if you only have '"1 2"', then the quotes are stripped, and '1 2' is returned.
 * 
 * Please note that it is assumed that there is no trailing comment in v. That is, there
 * is no final chunk of text beginning with a '#' character.
 */
char *WholeQuotedAttribute(const char *v)
{
	if (!v) return NULL;
	while (isspace(*v)) v++;
	if (!*v) return NULL;
	
	if (*v!='"') {
		 //the simple case just return whole thing
		const char *end=v+strlen(v)-1;
		while (end!=v && isspace(*end)) end--;
		if (end==v) return NULL;
		return newnstr(v,end-v+1);		
	}
	
	//else starts with quote
	
	 // so this chunk starts with a quote, it could be:
	 // '"2" "3"'  or  '"23\"4"  56' or  '"aohoau aou "'
	const char *p=v+1; //v points to first quote
	const char *end=NULL;
	while (*p) {
		while (*p && *p!='\\' && *p!='"') p++;
		if (*p=='\\') { //potential escaped quote
			p++;
		} else if (*p=='"') { //endquote for the initial quote
			end=p; // make end point to final quote
			p++;
			while (isspace(*p)) p++;
			if (*p!='\0') end=NULL; //do whole thing if another chunk found
			break;
		}
		p++;
	}
	if (end==NULL) { // must use whole thing from v (includes initial quote)
		end=p+strlen(p);
	} else {
		v++;
	}
	if (end==v) return NULL;
	return newnstr(v,end-v);
}

//---------------------------------- Dump helper functions ---------------------------------

//! Used to dump out the value part of an attribute.
/*! This is assumed to already point to the spot in the file right
 * after the name part, assumed to already have been written to file.
 * If the value has no newlines, it is put on the same line, potentially with
 * quotes escaped, and after writing out a space. If there are newlines, " \\\n"
 * is output and value is written on subsequent lines at indent.
 *
 * A newline is always the last thing written to the file. If value==NULL,
 * a single newline is written to f.
 *
 * If the string has quotes in it or a number sign, they will be escaped unless noquotes.
 *
 * \todo note that this will almost always quote value, and it probably shouldn't for simple values
 */
void dump_out_value(FILE *f,int indent,const char *value, int noquotes, const char *comment)
{
	if (value) {
		if (strchr(value,'\n')) {
			 // if value has \n such as "aoeuaoeu \n aouoeu"
			fprintf(f," \\%s%s\n", comment ? " #":"", comment ? comment:"");
			dump_out_indented(f,indent,value);
			fprintf(f,"\n");

		} else if ((strchr(value,'#') || strchr(value,'"')) && !noquotes) {
			 // simply written value, but has quotes, so must escape quotes
			fprintf(f," ");
			dump_out_escaped(f,value,-1);
			fprintf(f,"%s%s\n", comment ? " #":"", comment ? comment:"");

		} else {
			 //force quotes when there's space chars
			if (!strchr(value,' ')) noquotes=1;
			if (noquotes) fprintf(f," %s",value);
			else fprintf(f," \"%s\"",value);
			if (comment) fprintf(f," #%s\n", comment);
			else fprintf(f,"\n");
		}
	} else {
		if (comment) fprintf(f," #%s\n", comment);
		else fprintf(f,"\n");
	}
}

//! Dump out n characters of string, escaping quote characters if necessary.
/*! \ingroup fileutils
 *  If n<0 then dump all characters of str.
 *
 * If str==NULL, then nothing is written out.
 *
 * If str has trailing or leading whitespace or it starts with a quote,
 * or it contains a '#' character, then the whole string gets quoted.
 * Otherwise, it is written as is.
 * 
 * So say str='&nbsp;&nbsp;he said, "blah"', then what gets dumped is:\n
 * "  he said, \"blah\""
 *
 * A final newline after str is never written.
 *
 * \todo *** quote on escaped, quote on any ws, quote on initial or trailing ws, never quote, quote on escaped or ws, always quote
 * \todo *** Currently assumes no newlines... could substitute '\\n' for them...
 */
void dump_out_escaped(FILE *f, const char *str, int n)
{
	if (!str) return;
	if (n<0) n=strlen(str);
	if (n==0) { fprintf(f,"\"\""); return; } // empty but not null string
	const char *s=str;
	const char *ss=strchr(s,'#');
	if (*str!='"' && !(isspace(*str) || isspace(str[n-1])) && !(ss && ss-str<n)) {
		 // no start or ending whitespace, no '#',
		 // and doesn't start with a quote
		fprintf(f,"%s",str); 
		return; 
	}
	 // to be here, external quotes are necessary
	fprintf(f,"\""); 
	do {
		ss=strchr(s,'"');
		if (!ss || ss-str>=n) {
			fprintf(f,"%s",s);
			break;
		}
		fwrite(s,1,ss-s,f);
		fprintf(f,"\\\"");
		s=ss+1;
	} while (s-str<n);
	fprintf(f,"\""); 
}

/*! Return a new char[] escaped for use inside a quoted string.
 * Optionally enclose in actual quotes.
 * Quotes the same as quote, backslashes, newlines, carriage returns, and tabs will be escaped.
 * Also a '#' will trigger quotes, but will not itself be escaped.
 *
 * Generally, quote will be " or '.
 */
char *escape_string(const char *value, char quote, bool include_quotes)
{
	int ne=0;
	const char *v=value;
	while (*v) {
		if (*v==quote || *v=='\\' || *v=='\r' || *v=='\n' || *v=='\t' || *v=='#') ne++;
		v++;
	}

	if (!ne && !include_quotes) return newstr(value);

	char *nv=new char[strlen(value)+ne+3];
	char *v2=nv;
	v=value;

	if (include_quotes) { *v2=quote; v2++; }

	char s=0;
	while (*v) {
		s=0;
		if (*v==quote) s=quote;
		else if (*v=='\n') s='n';
		else if (*v=='\r') s='r';
		else if (*v=='\t') s='t';
		else if (*v=='\\') s='\\';

		if (s) {
			*v2='\\'; v2++;
			*v2=s;
		} else *v2=*v;

		v++;
		v2++;
	}

	if (include_quotes) { *v2=quote; v2++; }
	*v2='\0';

	return nv;
}

/*! Return a new char[] escaped for use inside a quoted string, including the quotes.
 * Escapes the quote character, backslashes, newlines, carriage returns, and tabs.
 *
 * Generally, quote will be " or '.
 */
void dump_out_quoted(FILE *f, const char *value, char quote)
{
	char *str = escape_string(value, quote, true);
	if (!str) return;
	fwrite(str, 1, strlen(str), f);
	delete[] str;
}

//! Just dump out string at constant indent.
/*! If there are newlines it the string, they are inserted in the
 *  stream as newlines. Blank lines are written '.'.
 *  So, "abc\n\ndef\nghi" (with actual newlines) gets written as:\n
 * \verbatim
   abc
   .
   def
   ghi
\endverbatim
 *  It always writes out a final newline.
 *
 *  It is assumed that the file pointer points to the beginning of the 
 *  line to output, so that first line will contain the indent then
 *  the first part of str.
 *
 *  See also Attribute::dump_in_indented().
 */
void dump_out_indented(FILE *f, int indent, const char *str)
{
	if (!str) return;
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	const char *s=str;
	const char *n;
	do {
		n=strchr(s,'\n');
		if (!n) {
			fprintf(f,"%s%s\n",spc,s);
			break;
		}
		if (n==s) fprintf(f,"%s.\n",spc); 
		else if (n==s+1 && *s=='.') fprintf(f,"%s\\.\n",spc);
		else if (n==s+2 && *s=='\\' && s[1]=='.') fprintf(f,"%s\\\\.\n",spc);
		else { fwrite(spc,1,indent,f); fwrite(s,1,n-s+1,f); }
		s=n+1;
	} while (*s);
}

//! Skip until the first nonblank, non-comment line that has indentation less or equal to indent.
/*! If that is the current line, then don't move.
 */
void skip_to_next_attribute(IOBuffer &f,int indent)
{
	long pos;
	char *line=NULL;
	size_t n=0;
	int c,i;

	while (!f.IsEOF()) {
		pos = f.Curpos();
		i = 0;
		c = getline_indent_nonblank(&line,&n,f,indent,"#",'"',0, &i); //0 means dont skip lines
		if (!c) break;
		if (i<=indent) {
			f.SetPos(pos);
			break;
		} 
	}
	if (line) f.FreeGetLinePtr(line);
}


//---------------------------------- Attribute -----------------------------------	
/*! \class Attribute
 * \ingroup attributes
 * \brief Class to facilitate Laxkit's file io of data.
 *
 * Attributes have a unique name, a particular value, and any number of additional subattributes.
 * They can read and write what I'm calling ida files. Ida standing for 
 * <a href="group__attributefileformat.html">'indented data'</a>. 
 * As a result, one is tempted to refer to databases stored in ida files
 * as 'ida know' files.
 * 
 * name is the id tag of the entry, or the attribute name.
 * 
 * atttype is the type of thing that name is. If atttype==NULL, then assume the value is
 * a string.
 *
 *
 *  \todo *** make sure that line is being freed where necessary...
 *  \todo most of the dump_in_* and dump_out_* could be put in standalone functions
 *  	  which would be more useful
 *  
 *  (note that pangomm and atkmm have an Attribute class, though it is in their own
 *  namespace)
 *
 * Attribute::flags is not natively used by Attribute. It exists to aid other
 * classes to keep a simple hint about what data is contained.
 */


Attribute::Attribute(const char *nn, const char *val,const char *nt)
{
	name=value=atttype=NULL;
	makestr(name,nn);
	makestr(value,val);
	makestr(atttype,nt);
	comment = NULL;
	flags=0;
}

Attribute::~Attribute()
{
	delete[] name;
	delete[] value;
	delete[] atttype;
	delete[] comment;
}

//! Set name, value, atttype, comment to NULL and flush attributes.
void Attribute::clear()
{
	delete[] name;    name    = NULL;
	delete[] value;   value   = NULL;
	delete[] atttype; atttype = NULL;
	delete[] comment; comment = NULL;
	attributes.flush();
}

/*! Update this->comment. Important: Currently, should not contain newlines.
 */
void Attribute::Comment(const char *ncomment)
{
	makestr(comment, ncomment);
}

//! Return a new deep copy of *this.
Attribute *Attribute::duplicate()
{
	Attribute *att=new Attribute(name,value,atttype);
	att->flags = flags;
	makestr(att->comment, comment);

	for (int c=0; c<attributes.n; c++) {
		if (!attributes.e[c]) continue; //tweak to ignore NULL attributes
		att->push(attributes.e[c]->duplicate(),-1);
	}
	return att;
}

//! Convenience function to search for a subattribute, and return its value string.
/*! i_ret is the index of the subattribute found, or -1 if not found, or -2 if found,
 * but could not convert to double.
 *
 * If an attribute is not found, NULL is returned.
 * If an attribute with a NULL value is found, then "" is returned.
 */
const char *Attribute::findValue(const char *fromname,int *i_ret)
{
	for (int c=0; c<attributes.n; c++)
		if (attributes.e[c]->name && !strcmp(attributes.e[c]->name,fromname)) {
			if (!isblank(attributes.e[c]->value)) {
				if (i_ret) *i_ret=c;
				if (!attributes.e[c]->value) return "";
				return attributes.e[c]->value;
			}
			break;
		}
	if (i_ret) *i_ret=-1;
	return NULL;
}

//! Convenience function to search for a subattribute, and convert its value to a double.
/*! i_ret is the index of the subattribute found, or -1 if not found, or -2 if found,
 * but could not convert to double.
 *
 * If an attribute is not found, 0 is returned.
 */
double Attribute::findDouble(const char *fromname,int *i_ret)
{
	for (int c=0; c<attributes.n; c++)
		if (attributes.e[c]->name && !strcmp(attributes.e[c]->name,fromname)) {
			if (!isblank(attributes.e[c]->value)) {
				if (i_ret) *i_ret=c;
				return strtod(attributes.e[c]->value,NULL);
			}
			break;
		}
	if (i_ret) *i_ret=-1;
	return 0;
}

//! Convenience function to search for a subattribute, and convert its value to a long.
/*! i_ret is the index of the subattribute found, or -1 if not found, or -2 if found,
 * but could not convert to long.
 *
 * If an attribute is not found, 0 is returned.
 */
long Attribute::findLong(const char *fromname,int *i_ret)
{
	for (int c=0; c<attributes.n; c++)
		if (attributes.e[c]->name && !strcmp(attributes.e[c]->name,fromname)) {
			if (!isblank(attributes.e[c]->value)) {
				if (i_ret) *i_ret=c;
				return strtol(attributes.e[c]->value,NULL,10);
			}
			break;
		}
	if (i_ret) *i_ret=-1;
	return 0;
}

//! Return the first sub-attribute with the name fromname, or NULL if not found.
/*! If i_ret!=NULL, then fill it with the index of the attribute, if found, or -1 if not found.
 */
Attribute *Attribute::find(const char *fromname,int *i_ret)
{
	for (int c=0; c<attributes.n; c++)
		if (attributes.e[c]->name && !strcmp(attributes.e[c]->name,fromname)) {
			if (i_ret) *i_ret=c;
			return attributes.e[c];
		}
	if (i_ret) *i_ret=-1;
	return NULL;
}

/*! Allow for easier pushing of subatts. Push a new one with given name and value,
 * and return pointer to it.
 */
Attribute *Attribute::pushSubAtt(const char *nname, const char *nvalue)
{
	Attribute *att=new Attribute(nname,nvalue);
	push(att,-1);
	return att;
}

//! Push a full blown, already constructed Attribute onto the attribute stack.
/*! If where==-1, then push onto the top of the stack.
 * The calling code must not delete the att. It is now the responsibility of *this.
 * Returns the index of the newly pushed att, or -1 if pushing failed.
 */
int Attribute::push(Attribute *att,int where)
{ 
	if (att) return attributes.push(att,1,where); 
	return -1;
}

//! Shortcut to push with name and a NULL value to top of stack.
int Attribute::push(const char *nname)
{
	Attribute *v=new Attribute(nname,NULL,NULL);
	return push(v,-1);
}

int Attribute::pushStr(const char *nname, int where, const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    int c = vsnprintf(NULL, 0, fmt, arg);
    va_end(arg);

    char *message = new char[c+1];
    va_start(arg, fmt);
    vsnprintf(message, c+1, fmt, arg);
    va_end(arg);

    int status = push(nname, message, where);
    delete[] message;
    return status;

}

int Attribute::push(const char *nname,const char *nval, const char *ncomment, int where)
{
	int i = push(nname, nval, where);
	attributes.e[i]->Comment(ncomment);
	return i;
}

//! Push a simple nname==nval attribute onto this.
/*! Note that this simply creates a new Attribute and calls push(Attribute *,int).
 *  If where==-1, then push onto the top of the stack.
 */
int Attribute::push(const char *nname,const char *nval,int where)
{
	Attribute *v=new Attribute(nname,nval,NULL);
	return push(v,where);
}

//! Push a simple nname==nval attribute onto this.
/*! Note that this simply pushes a new Attribute with name and a string for value 
 * equivalent to nval. It calls push(Attribute *,int). 
 *
 * If where==-1, then push onto the top of the stack.
 */
int Attribute::push(const char *nname,int nval,int where)
{
	char scratch[20];
	sprintf(scratch,"%d",nval);
	Attribute *v=new Attribute(nname,scratch,NULL);
	return push(v,where);
}

//! Push a simple nname==nval attribute onto this.
/*! Note that this simply pushes a new Attribute with name and a string for value 
 * equivalent to nval. It calls push(Attribute *,int). 
 *
 * If where==-1, then push onto the top of the stack.
 */
int Attribute::push(const char *nname,long nval,int where)
{
	char scratch[20];
	sprintf(scratch,"%ld",nval);
	Attribute *v=new Attribute(nname,scratch,NULL);
	return push(v,where);
}

//! Push a simple nname==nval attribute onto this.
/*! Note that this simply pushes a new Attribute with name and a string for value 
 * equivalent to nval. It calls push(Attribute *,int). 
 *
 * If where==-1, then push onto the top of the stack.
 */
int Attribute::push(const char *nname,unsigned long nval,int where)
{
	char scratch[20];
	sprintf(scratch,"%lu",nval);
	Attribute *v=new Attribute(nname,scratch,NULL);
	return push(v,where);
}

//! Push a simple nname==nval attribute onto this.
/*! Note that this simply pushes a new Attribute with name and a string for value 
 * equivalent to nval. It calls push(Attribute *,int). 
 *
 * If where==-1, then push onto the top of the stack.
 */
int Attribute::push(const char *nname,double nval,int where)
{
	char scratch[20];
	sprintf(scratch,"%.10g",nval);
	Attribute *v=new Attribute(nname,scratch,NULL);
	return push(v,where);
}

//! Remove the subattribute with the given index.
/*! Return 0 for succes or 1 for out of range.
 */
int Attribute::remove(int index)
{
	if (index<0 || index>=attributes.n) return 1;
	attributes.remove(index);
	return 0;
}

//! Write out the subattributes to f, not including this->name/value.
/*! Conveniently, you can call \a dump_out(stdout,0) to dump out the Attribute
 * to the screen.
 *
 * Please note this does not output this->name and value. If you need those also,
 * then use dump_out_ful().
 */
void Attribute::dump_out(FILE *f, int indent)
{
	if (!attributes.n) return;
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';

	 //try to print out at least marginally nicely by space padding after name
	int namewidth = 1, w;
	for (int c=0; c<attributes.n; c++) {
		if (!attributes.e[c]->name) continue;
		w = strlen(attributes.e[c]->name);
		if (w > namewidth) namewidth = w;
	}
	//char spc2[namewidth+1]; memset(spc2,' ',namewidth); spc[namewidth]='\0';
	char format[10];
	sprintf(format, "%%-%ds", namewidth);
	
	for (int c=0; c<attributes.n; c++) {
		fprintf(f,"%s",spc);

		 //dump out name
		if (!attributes.e[c]->name) fprintf(f,"\"\"");
		else if (strchr(attributes.e[c]->name,' ') || strchr(attributes.e[c]->name,'\t'))
			dump_out_escaped(f,attributes.e[c]->name,-1);//***does this work as intended??
		else fprintf(f,format,attributes.e[c]->name);
		//else fprintf(f,"%s",attributes.e[c]->name);

		 //dump out value
		if (!attributes.e[c]->value) fprintf(f,"\n");
		else dump_out_value(f, indent+2, attributes.e[c]->value, 0, attributes.e[c]->comment);

		 //dump out subatts
		attributes.e[c]->dump_out(f,indent+2);
	}
}

//! Write out to f.
/*! Conveniently, you can call \a dump_out_full(stdout,0) to dump out the Attribute
 * to the screen.
 *
 * If there is no name, value, or subatts then nothing is written. If name==NULL,
 * then nothing is written for name, but value is still written. This can
 * potentially cause problems when name=NULL and value is something like
 * "1234\naeuo". This will cause a blank line with indent followed by two
 * lines with indent+2: "1234" and "aeuo" beneath it.
 *
 * \todo make name==NULL and value=something like "1234\naeuo" behave in
 *   some sort of reasonable way, like have name become "" or ~ or something..
 *   ... null name gets "", but problems still if name has whitespace..
 */
void Attribute::dump_out_full(FILE *f, int indent)
{
	if (!name && !value && !attributes.n) return;
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	
	fprintf(f,"%s",spc);

	if (!name) fprintf(f,"\"\"\n");
	else if (strchr(name,' ') || strchr(name,'\t')) dump_out_escaped(f,name,-1);//***does this work as intended??
	else fprintf(f,"%s",name);

	if (!value) fprintf(f,"\n");
	else dump_out_value(f, indent+2, value);
	
	for (int c=0; c<attributes.n; c++) {
		attributes.e[c]->dump_out(f,indent+2);
	}
}

//! The opposite of dump_out_indented().
/*! Lines must be indented >= indent. Blank lines have ".". The first
 * true blank line, or the first lesser indented line stops the reading.
 * Comments are stripped from each line. A comment is anything
 * from an unescaped, unquoted '#' character to the end of the line.
 */
char *Attribute::dump_in_indented(IOBuffer &f, int Indent)
{
	char *line=NULL,*str=NULL,*tline;
	size_t n;
	int c,indent;

	long pos;
	int firstindent=-1;
	int linenumber=0;

	while (!f.IsEOF()) {
		pos = f.Curpos();
		linenumber++;

		//DBG cerr <<"dump in, line number: "<<linenumber<<endl;

		c = getline_indent_nonblank(&line,&n,f,Indent,"#",'"',0); //0 means dont skip lines
		if (c<=0) break;

		indent = how_indented(line);
		if (firstindent<0) firstindent = indent;

		if (indent < Indent) {
			f.SetPos(pos);
			if (f.IsEOF()) f.Clearerr();
			if (line) f.FreeGetLinePtr(line);
			line=NULL;
			n=0;
			return str;
		}
		 //preserve spaces beyond Indent, but pull back if less than first 
		 //indent, but more than Indent
		if (indent<firstindent) firstindent = indent;
		else indent = firstindent;

		tline = line+indent;
		c -= indent;
		if (c <= 0) break;
		if (linenumber>1) appendstr(str,"\n");

		if (!strcmp(tline,".")) {
			//do nothing...  line has a single '.' at the beginning, prepend "\n" on next line
		} else if (!strcmp(tline,"\\.")) {
			appendstr(str,"."); //line has only "\.", add ".\n"
		} else {
			if (c>1 && tline[0]=='\\' && tline[1]=='\\') line++;
			appendstr(str,tline);
		}
	}
	
	if (line) f.FreeGetLinePtr(line);
	return str;
}

//! Read in until tag is encountered on its own line.
/*! Blank lines need only be blank, they do not have to have the expected
 * indentation. Reading is terminated only by finding tag or eof.
 * Comments are not stripped. The final newline before the tag is not
 * included.
 */
char *Attribute::dump_in_until(IOBuffer &f, const char *tag, int Indent)//indent=0
{
	char *str  = NULL;
	char *line = NULL, *tline;
	size_t n;
	int c,indent=-1,i;
	long pos;

	while (!f.IsEOF()) {
		pos = f.Curpos();
		c = f.GetLine(&line,&n);
		if (c<=0) break;

		if (indent==-1) indent = i = how_indented(line);
		else i = how_indented(line);

		if (i < Indent) { // not indented enough, so return.
			f.SetPos(pos);
			if (f.IsEOF()) f.Clearerr();
			if (line) f.FreeGetLinePtr(line);
			return 0;

		} else if (i < indent) indent = i;
		
		tline = line+indent;
		c -= indent;
		//if (c<=0) break; ***use blank lines....
		if (!strncmp(tline,tag,strlen(tag)) && (!tline[strlen(tag)] || tline[strlen(tag)]=='\n')) {
			 // tag reached, so break;
			break;
		}
		appendstr(str,tline);
	}
	if (line) f.FreeGetLinePtr(line);
	if (str[strlen(str)-1]=='\n') str[strlen(str)-1]='\0';
	return str;
}

//! Remove backslashes. Double backslash becomes single backslash.
/*! Note this does not substite characters. Thus "\\t" converts to 't'.
 */
char *removeescapes(char *&str)
{
	int len=strlen(str);
	for (int c=0; c<len; c++) {
		if (str[c]=='\\') {
			memmove(str+c,str+c+1, len-c);
			len--;
		}
	}
	return str;
}

/*! This function just wraps f in an IOBuffer, and calls dump_in(IOBuffer &f, int Indent,Attribute **stopatsub=NULL)
 */
int Attribute::dump_in(FILE *f, int Indent,Attribute **stopatsub)//stopatsub=NULL
{
	IOBuffer ff;
	ff.UseThis(f);

	dump_in(ff,Indent,stopatsub);

	ff.UseThis(NULL);
	return 0;
}

//! Read in sub-attribute data, starting after initial line...
/*! This function is meant to be called after the line with this->name and value has
 * already been parsed. This function just parses in all the subattributes,
 * and recursively their subattributes. Usually it will be called with Indent equal
 * to 1 more than what this->name/value was indented at. The actual indentation of
 * the subattribute must only be greater than or equal to Indent.
 *
 * Each subelement parsed in is pushed onto the top of the attributes stack.
 *
 * If stopatsub!=NULL, then parsing halts after 1 subattribute is encountered,
 * *stopatsub is made to point to the relevant subattribute (which is somewhere on
 * the attributes stack), and the actual indentation of the subattribute line is returned.
 * This allows programs to parse just so far, then they can pass parsing control
 * to their own subelement that corresponds to the current subattribute (the one on the
 * top of the attributes stack) if they so wish. The FILE pointer will point to the
 * beginning of the line following the subattribute name and value.
 *
 * After this other parsing routine
 * returns, the program can call this function again, and it will continue parsing as
 * if it had just read those subsubattributes. Just be sure that your custom parsing
 * routine advances past all of what may be considered subsubattributes, or else this
 * function will likely think they are furthur subattributes.
 * 
 * In any case, Parsing stops upon the first non-blank, non-comment line that has something
 * at an indentation less than indent.
 *
 * No substitutions are done. That is, say defaultpage is defined
 * previously in a containing scope, and a subattribute with name
 * defaultpage is found here, it is parsed in as name="defaultpage",
 * value=NULL. Substituting must be done elsewhere.
 * 
 * If stopatsub!=NULL, return -1 for nothing found or EOF, or if a subattribute was just
 * parsed, return the actual indent of that subattribute.
 *
 * If stopatsub==NULL, return the number of subattributes read, or a negative number for error.
 * The calling code must check for EOF itself. Compounding elements with the +name mechanism
 * counts as 1 subattribute read per +name line.
 * 
 * \todo make name==NULL (in file as "") and value=something like "1234\naeuo" behave in
 *   some sort of reasonable way, like have name become "" or ~ or something..
 */
int Attribute::dump_in(IOBuffer &f, int Indent,Attribute **stopatsub)
{
	if (f.IsEOF()) return 0;

	char *line=NULL;
	int c,indent;
	size_t n=0;
	char *val,*fld,*temp=NULL;
	int nf,numattsread=0;
	Attribute *att=NULL;
	if (stopatsub) *stopatsub=NULL;

	while (!f.IsEOF()) {
		c = getline_indent_nonblank(&line,&n,f,Indent,"#",'"',1);
		if (c <= 0) break; // eof or bad line
		
		 // now line is on a properly indented line that has no trailing whitespace
		fld = line;
		while (isspace(*fld)) fld++;
		indent = fld-line; //*** must check for improperly indented file!!
				// the line should be at least Indent as per the above getline
				// wrong:
				//   blah
				//       subblah
				//     subblah
		val = fld;
		while (*val && !isspace(*val)) val++;
		nf = val-fld;
		while (isspace(*val)) val++;
		fld[nf]='\0';
		if (val-fld==nf) val=NULL;

		 // now fld points to the potential field name, which goes for nf chars,
		 // and val points to the start of the value part.
		 // fld can be:
		 //  blah     <-- append to subatts...
		 //  +blah    <-- combine with subatt with same name as blah
		 //  .blah    <-- scope out, search for blah above...??
		 // val can be: 
		 //   \       <-- expect simple value indented starting next line..
		 //   <<<filename <-- signal that format is ATT_VALUE_FILE, contents in file filename 
		 //   << BLAH <-- raw read in until BLAH is encountered again. 
		 //   < BLAH <-- indented raw read in until BLAH is encountered again. 
		 //   (NULL)
		 //   somesimplevalue

		if (!att) {
			att=new Attribute;
			makestr(att->name,fld);
			removeescapes(att->name);
			attributes.push(att);
		}

		numattsread++;
		if (!val) {} // do nothing for null value
		else if (!strcmp(val,"\\")) {
			//**** there's confusion when line ends in "\   "..
			 // name \ #comment should have been stripped
			 //   stuff
			val = temp = dump_in_indented(f,indent+1);

		} else if (!strncmp(val,"<<<",3)) {
			 // <<< filename
			val += 3;
			while (isspace(*val)) val++;
			if (!*val) { 
				val = NULL; 
				DBG cerr <<" <<< broken filename!!"<<endl; 
			} else {
				// *** supposed to read in file and put it in attribute
			}

		} else if (!strncmp(val,"<<",2)) {
			 // << INDENTEDTAG
			val += 2;
			while (isspace(*val)) val++;
			if (!*val) { 
				val = NULL;
				DBG cerr <<" <<< broken indented tag!! "<<endl;
			} else {
				val = temp = dump_in_until(f,val,indent+1);
			}

		} else if (*val=='<') {
			 // < RAWTAG
			val++;
			while (isspace(*val)) val++;
			if (!*val) {
				val = NULL;
				DBG cerr <<" <<< broken rawtag!! "<<endl;
			} else {
				val = temp = dump_in_until(f,val,0);
			}

		} else { // is a simple value on name line, check for quotes.. 
			if (*val=='"') { 
				 // if end of val is a matched quote, remove quotes..
				int e = 0, //e==1 if a backslash is encountered, and must parse next char
					m = 0, //the number of recognizable chunks, must be 1 at end to remove quotes
					q = 0; //the position of the last unescaped quote
				for (c=1; val[c]!='\0'; c++) {
					if (e) { e = 0; continue; }
					if (q>0 && !isspace(val[c])) { m = 2; break; }
					if (val[c]=='\\') e = !e;
					else if (val[c]=='"') { q = c; m++; }
				}
				// if loop was broken 1 before end, then remove quotes
				if (m==1) { // was matched quote, need to unescape quotes now
					val[q]='\0';
					val++;
					for (c=0; val[c]!='\0'; c++) {
						if (val[c]=='\\') {
							if (val[c+1]=='"') memmove(val+c,val+c+1,strlen(val+c+1)+1);
						}
					}
				}
			}
		}
		if (val) makestr(att->value,val);
		if (temp) { delete[] temp; temp=NULL; }

		//DBG char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
		//DBG cerr<<spc<<"Found att("<<attributes.n<<"):\""<<(att->name?att->name:"(no name)")<<"\" = \""
		//DBG 	<< (att->value?att->value:"(no name)")<<"\""<<((att->format==ATT_VALUE_FILE)?"(is file)":"")<<endl;

		if (stopatsub) {
			*stopatsub=att;
			if (temp) delete[] temp;
			if (line) f.FreeGetLinePtr(line);
			return indent;
		} else att->dump_in(f,indent+1);
		att=NULL;
	}

	if (temp) delete[] temp;
	if (line) f.FreeGetLinePtr(line);
	return numattsread;
}


//! Read in a whole database.
/*! The base is name=file, value=filename.
 * For what, see AttributeTypes. Default is ATT_Att.
 * Also checks for ATT_Json and ATT_Xml.
 * Warning that xml may fail for html that does not have closing indications on any tags.
 *
 * \todo *** when dumping in, it is sometimes useful to preserve what the position in the file
 *   at which the attribute is written. this would require an extra long variable in Attribute..
 *   For instance for decent error checking, one wants to know where the error occured, but one
 *   might still want the ease of dumping in the whole file to an Attribute before parsing values..
 * 
 * Returns 0 for success, otherwise nonzero error.
 */
int Attribute::dump_in(const char *filename, int what)
{
	if (what == ATT_Json) {
		if (JsonFileToAttribute(filename, this) == this) return 0;
		return 1;
	}
	if (what == ATT_Xml) {
		if (XMLFileToAttribute(this, filename, NULL) == this) return 0;
		return 1;
	}

//	-------------
	IOBuffer f;
	f.OpenFile(filename, "r");

	if (!f.IsOpen()) {
		DBG cerr <<"Open "<<filename<<" failed."<<endl;
		return 1;
	}

	makestr(name,"file");
	makestr(value,filename);
	makestr(atttype,NULL);

	DBG cerr <<"Reading "<<filename<<"...."<<endl;

	dump_in(f,0);

	f.Close();
//	-------------
//	FILE *f=fopen(filename,"rb");
//	if (!f) {
//		DBG cerr <<"Open "<<filename<<" failed."<<endl;
//		return 1;
//	}
//
//	makestr(name,"file");
//	makestr(value,filename);
//	makestr(atttype,NULL);
//
//	DBG cerr <<"Reading "<<filename<<"...."<<endl;
//
//	dump_in(f,0);
//
//	fclose(f);
//	-------------

	return 0;
}

/*! Like dump_in(const char*, Attribute*), but use a string instead of reading
 * in file contents.
 */
int Attribute::dump_in_str(const char *str)
{
	IOBuffer f;
	f.OpenCString(str);

	makestr(name,"string");
	makestr(value,NULL);
	makestr(atttype,NULL);

	//dump_in(f,0);
	cerr << " *** need to implement  Attribute::dump_in_str()!"<<endl;

	f.Close();
	return 0;
}

/*! Return 0 success, or nonzero error.
 */
int Attribute::dump_in_json(const char *str)
{
	if (JsonStringToAttribute (str, this, NULL) == this) return 0;
	return 1;
}

/*! Return 0 success, or nonzero error.
 */
int Attribute::dump_in_xml (const char *str)
{
	long pos = 0;
	const char **stand_alone_tag_list = NULL;
	if (XMLChunkToAttribute(this, str,strlen(str), &pos, NULL, stand_alone_tag_list) == this) return 0;
	return 1;
}

//! Extract a single "name=value" string. The value can be quoted.
/*! On error or no name value found, end_ptr gets set to str, and nonzero is returned. name
 * and value are not modified in this case. Otherwise, 0 is returned and name and value
 * are set.
 *
 * So if str=="a=5 b=10 c=2", then returned values are name="5", value="10",
 * and end_ptr is "b=10 c=2" (without the whitespace before b). The value here is
 * parsed from the '=' until whitespace.
 *
 * If you have something like a="1 2 3" with the quotes in str itself, then
 * name="a" and value is "1 2 3" without the quotes.
 *
 * assign is the single character delimiter, default is '='. In CSS for instance, it might be ':'.
 *
 * stopat is a '\0' terminated
 * list of characters, any of which will terminate the parsing if unquoted. So, you 
 * can properly parse an xml tag ".... color=red/>" or "... color=red>".
 *
 * delim is an optional end of line marker, such as ';', which is ignored.
 * 
 * end_ptr will be set to the character just after what's been parsed. If delim
 * is encountered, end_ptr will point to the following character.
 */
int NameValueAttribute(const char *str, char **name, char **value, char **end_ptr, 
					   char assign,char delim,const char *stopat)
{
	while (isspace(*str)) str++;
	const char *s=str;
	char *e=NULL;

	 //find a string of non-whitespace characters
	while (*s && !isspace(*s) && *s!=delim && *s!=assign && !(stopat && strchr(stopat,*s))) s++;
	if (s==str) {
		if (delim && *str==delim) str++;
		 // there was nothing there but whitespace
		if (end_ptr) *end_ptr=const_cast<char *>(str);
		return 1;
	}
	 
	 //so found a name
	*name=newnstr(str,s-str);
	str=s;
	while (isspace(*str)) str++;
	if (*str!=assign) {
		 //name but no value
		if (delim && *str==delim) str++;
		*value=NULL;
		if (end_ptr) *end_ptr=const_cast<char *>(str);
		return 0; //*** might want to return 1?
	}
	 
	 // so theres an '=', but not sure if there's a value yet
	str++; //skip over the assign char
	while (isspace(*str)) str++; //str now points to the start of value
	if (*str=='"' || *str=='\'') *value=QuotedAttribute(str,&e);
	else if (*str) {
		e=const_cast<char *>(str);
		while (*e && !isspace(*e) && *e!=delim && !(stopat && strchr(stopat,*e))) e++;
		*value=newnstr(str,e-str);
	}
	if (!*value) {
		 // no value found but there was an '='
		if (end_ptr) *end_ptr=const_cast<char *>(str); //points to where value would start
		return 2;
	}

	 // name and value found
	if (delim && *e==delim) e++;
	if (end_ptr) *end_ptr=e;
	return 0;
}

//! Transform something like "name=value name2=value2" into an Attribute.
/*! If att!=NULL, then append subattributes to it and also return att.
 * Otherwise, return a new Attribute.
 */
Attribute *NameValueToAttribute(Attribute *att,const char *str, char assign, char delim)
{
	if (!att) att=new Attribute;

	char *name=NULL,*value=NULL;
	char *end;

	end=const_cast<char *>(str);
	do {
		str=end;
		if (NameValueAttribute(str,&name,&value,&end,assign,delim,NULL)!=0) break;
		att->push(name,value);
		delete[] name;
		delete[] value;
	} while (end!=str);
	return att;
}



//---------------------------------- AttributeObject -----------------------------------	
/*! \class AttributeObject
 *
 * Just like Attribute, but subclassing anObject so as to be reference countable.
 */


AttributeObject::AttributeObject()
  : Attribute()
{
	data = NULL;
}

AttributeObject::AttributeObject(const char *nn, const char *nval,const char *nt)
  : Attribute(nn,nval,nt)
{
	data = NULL;
}

AttributeObject::~AttributeObject()
{
	if (data) data->dec_count();
}

Attribute *AttributeObject::duplicate()
{
	AttributeObject *att=new AttributeObject(name,value,atttype);
	att->flags=flags;
	for (int c=0; c<attributes.n; c++) {
		if (!attributes.e[c]) continue; //tweak to ignore NULL attributes
		att->push(attributes.e[c]->duplicate(),-1);
	}

	if (data) att->SetData(data->duplicate(NULL), 1);
	return att;
}

void AttributeObject::SetData(anObject *ndata, int absorb)
{
	if (!ndata && !data) return;
	if (ndata && ndata == data) {
		if (absorb) ndata->dec_count();
	} else {
		if (data) data->dec_count();
		data = ndata;
		if (data && !absorb) data->inc_count();
	}
}

//---------------------------------- XML Conversion Helpers -----------------------------------	

/*! Warning: completely overwrites the file.
 *
 * Returns 0 for success. Non-zero for error.
 *
 * The attribute passed in must be particularly formatted. For each main tag, any subtag
 * that is not named "content:" is written as an XML attribute of that tag, not as an element. Those
 * must have only a name and value. Any sub-attributes they may have are ignored.
 * There should be only one "content:" sub-attribute for a tag. This will contain all the elements
 * in the tag. That "content:" sub-attribute might contain several "cdata:" elements, which
 * are character data in between other tagged elements. "cdata:" attributes must have a value, but not
 * have any subattributes.
 *
 * \todo perhaps have a niceness parameter, so to lay out XML attributes inside a
 *   tag on separate lines:
 *   <blah first="1"
 *         second="2">
 * \todo *** this needs a lot of work for escaped quoted stuff....
 */
int AttributeToXMLFile(FILE *f, Attribute *att, int indent)
{
	if (!f || !att) return 1;

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	int c,c2,c3;
	for (c=0; c<att->attributes.n; c++) {
		 //check for cdata, output as is if so
		if (!strcmp(att->attributes.e[c]->name,"cdata:")) {
			fprintf(f,"%s",att->attributes.e[c]->value);
			continue;
		}

		 //opening tag
		fprintf(f,"%s<%s",spc,att->attributes.e[c]->name);
		if (!strcmp("!--",att->attributes.e[c]->name)) {
			 //output a comment...
			fprintf(f," %s -->\n",att->attributes.e[c]->value?att->attributes.e[c]->value:"");
			continue;
		}
		c3=-1;
		for (c2=0; c2<att->attributes.e[c]->attributes.n; c2++) {
			 //anything other than content: write as an attribute
			if (strcmp(att->attributes.e[c]->attributes.e[c2]->name,"content:"))
				fprintf(f," %s=\"%s\"",att->attributes.e[c]->attributes.e[c2]->name,
									   att->attributes.e[c]->attributes.e[c2]->value);
			else c3=c2;
		}
		 //close the tag
		if (att->attributes.e[c]->name[0]=='?') fprintf(f,"?>\n"); //assume no subelements for <?...?> tags
		else if (c3==-1 && !att->attributes.e[c]->value) fprintf(f,"/>\n"); //no value or elements
		else if (c3==-1 && att->attributes.e[c]->value) { //no elements, but has value, write as cdata
			fprintf(f,">%s</%s>\n",att->attributes.e[c]->value,att->attributes.e[c]->name);
		} else fprintf(f,">\n"); //has elements

		 //if has elements (a content: block found), then output, then close the tag
		if (c3>=0) {
			AttributeToXMLFile(f,att->attributes.e[c]->attributes.e[c3],indent+2);
			fprintf(f,"%s</%s>\n",spc,att->attributes.e[c]->name);
		} 

	}
	return 1;
}

//! Output any Attribute to some generic XML.
/*! Normal attributes make no distinction between elements and attributes.
 * They are all subattributes. This function does nothing special for "!--" or
 * "?thing" attributes. 
 *
 * The xml output will be a set of nested elements. 
 * If there are no subattributes and no value, then the output is "<tag/>".
 * If there are no subattributes and a non-null value, then it'll be "<tag>value</tag>".
 * If there are subattributes and a non-null value, then the tag will be:
 * "<tag value=\""whatever the value was\"> ...(subattributes)...</tag>".
 * If thera are subattributes, but no value, it will be the same, only 
 * without the value attribute of tag.
 *
 * To output data inputted from XMLFileToAttribute(), you should use AttributeToXMLFile().
 */
int SubAttributesToXMLFile(FILE *f, Attribute *att, int indent)
{
	if (!f || !att) return 1;

	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	int c;
	for (c=0; c<att->attributes.n; c++) {

		if (att->attributes.e[c]->attributes.n==0) {
			if (!att->attributes.e[c]->value) {
				 //no value and no subatts
				fprintf(f,"%s<%s/>\n",spc,att->attributes.e[c]->name);
			} else {
				 //yes value and no subatts
				fprintf(f,"%s<%s>%s</%s>\n",spc,
										att->attributes.e[c]->name,
										att->attributes.e[c]->value,
										att->attributes.e[c]->name);
			}
		} else {
			 //yes subattributes

			 //print opening tag
			if (att->attributes.e[c]->value) {
				 //has value
				fprintf(f,"%s<%s value=\"",spc,att->attributes.e[c]->name);
				dump_out_escaped(f,att->attributes.e[c]->value,strlen(att->attributes.e[c]->value));
				fprintf(f,"\">\n");
			} else {
				 //has no value
				fprintf(f,"%s<%s>\n",spc,att->attributes.e[c]->name);
			}

			 //print content
			SubAttributesToXMLFile(f,att->attributes.e[c],indent+2);

			 //print closing tag
			fprintf(f,"%s</%s>\n",spc,att->attributes.e[c]->name);
		}
	}
	return 1;
}

//! Convert the attribute to a character array.
/*! If appendtothis!=NULL, then append to it. Otherwise return a new char[].
 *
 * Comments are derived from attributes with name "!--". A <?xml ... ?> tag is 
 * derived from an attribute with name ?xml. Any value is mapped to content. Subattributes
 * are mapped to xml attributes of a tag with name as the element name. Further content
 * sections, if any, are mapped from attributes named "content:".
 *
 * \todo implement me!
 */
char *AttributeToXML(Attribute *att,char *&appendtothis, char **error_ret)
{//***
	if (!att) return NULL;
	cout <<"*** implement AttributeToXML!!"<<endl;
	return NULL;
}

//! Read in an XML file, and rather simply make an Attribute out of it.
/*! This merely opens the file and returns XMLChunkToAttribute(NULL,f,stand_alone_tag_list).
 *
 * Puts in att if att!=NULL, otherwist returns a new Attribute.
 * See XMLChunkToAttribute(Attribute*,const char *,long,long*,const char *,const char **)
 * for details about the conversion.
 *
 * \todo WARNING!! something like in html where img, br, hr, etc do not have closing tags
 *   are not dealt with properly... need to implement stand_alone_tag_list.. it would be
 *   really clever to be able to figure out that list from a dtd..
 */
Attribute *XMLFileToAttribute(Attribute *att,const char *file,const char **stand_alone_tag_list)
{
	FILE *f=fopen(file,"r");
	if (!f) return NULL;

	if (att==NULL) att=new Attribute;
	XMLChunkToAttribute(att,f,stand_alone_tag_list);
	
	fclose(f);
	return att;
}

/*! like XMLFileToAttribute(), but open file with flock() active.
 * Also sets locale to "C" while loading.
 */
Attribute *XMLFileToAttributeLocked(Attribute *att,const char *file,const char **stand_alone_tag_list)
{
    setlocale(LC_ALL,"C");
    char *rf=expand_home(file);
    int fd=open(rf,O_RDONLY);
    delete[] rf;
    if (fd<0) { setlocale(LC_ALL,""); return NULL; }
    flock(fd,LOCK_EX);
    FILE *f=fdopen(fd,"r");
    if (!f) { setlocale(LC_ALL,""); close(fd); return NULL; }

	if (att==NULL) att=new Attribute;
    XMLChunkToAttribute(att,f,stand_alone_tag_list);

    flock(fd,LOCK_UN);
    fclose(f);// this closes fd too
    setlocale(LC_ALL,"");

    return att;
}

////size of file chunk to read in at a time
//#define maxbuf 1024

//! Read in a part of an XML stream.
/*! This assumes that the next thing in f is an xml tag beginning with a less than sign.
 * This function parses in everything until the close of the tag. It is a very simplistic
 * conversion. If you need more, you should probably use the actual libxml.
 *
 * If att==NULL, then return a new Attribute. Otherwise, append the new things to att.
 *
 * stand_alone_tag_list is a list of element names that are self contained, that is, they
 * have no closing tag, and do not explicitly end in /&lt;.
 * 
 * See XMLChunkToAttribute(Attribute*,const char *,long,long*,const char *,const char **)
 * for details about the conversion.
 * 
 * \todo this just reads in the whole file to memory then parses it, which might be rather
 *   inefficient, but it is easy to code..
 */
Attribute *XMLChunkToAttribute(Attribute *att,FILE *f,const char **stand_alone_tag_list)
{
	if (!att) att=new Attribute;
	
	// <?xml version="1.0"?>
	// <!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
	
	int n;
	long revertpos=ftell(f); // position to revert to if too much of file read in.
	fseek(f,0,SEEK_END);
	long maxbuf=ftell(f)-revertpos;
	fseek(f,revertpos,SEEK_SET);
	char buf[maxbuf+1];
	
	n=fread(buf,1,maxbuf,f);
	if (n<=0) return NULL; //error
	
	XMLChunkToAttribute(att,buf,maxbuf,NULL,NULL,stand_alone_tag_list);
	
	return att;
}

//! Skip starting at c. Update c to point to the first non whitespace, non comment.
static void skipws(const char *buf,long n,long *c)
{
	while (*c<n && isspace(buf[*c])) (*c)++;
}

//! Return 1 for str in list, or 0. list is NULL terminated list.
/*! \ingroup misc
 *
 * \todo move this to strmanip.h?
 */
int one_of_them(const char *str,const char **list)
{
	if (!list || !str) return 0;
	while (*list) {
		if (!strcmp(*list,str)) return 1;
		list++;
	}
	return 0;
}

//! Read in XML to an Attribute from a memory buffer.
/*!
 * Say you have something like this Passepartout file:
 * \verbatim
 *   <?xml version="1.0"?>
 *   <!-- this is a comment -->
 *   <!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
 *   <document paper_name="Letter" doublesided="true" landscape="false" first_page_num="1">
 *    <title>single thing to value</title>
 *    <page>
 *        <frame name="Raster beetile-501x538.jpg" 
 *               matrix="0.812233 0 0 0.884649 98.6048 243.107" 
 *               lock="false"
 *               flowaround="false"
 *               obstaclemargin="
 *               0" type="raster" file="beetile-501x538.jpg"/>
 *     </page>
 *     <extra thing="2">text<b>bold</b>more text</extra>
 *   </document>
 * \endverbatim
 * 
 * The Attribute that is created will look something like this:
 * 
 * <pre>
 *   ?xml
 *     version 1.0
 *   !-- "this is a comment"
 *   !DOCTYPE
 *     svg
 *     PUBLIC
 *     "-//W3C//DTD SVG 1.1//EN"
 *     "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"
 *   document
 *     paper_name Letter
 *     doublesided true
 *     landscape false
 *     first_page_num 1
 *     content:
 *       title "single thing to value"  <---TODO!!! test!
 *       page
 *         content:
 *           frame 
 *             name Raster beetile-501x538.jpg
 *             matrix 0.812233 0 0 0.884649 98.6048 243.107
 *             lock false
 *             flowaround false
 *             obstaclemargin 0
 *             type raster
 *             file beetile-501x538.jpg 
 *       extra
 *         thing 2
 *         content:
 *           cdata: text
 *           b bold
 *           cdata: more text
 *           
 * </pre>
 *
 * You'll notice that the 'content:' field of the Attribute contains the XML elements 
 * that are not xml attributes in the opening &lt;...&gt; tag. 
 * \todo UNLESS there are only non-tagged characters between
 *   the tags. In that case, the text becomes the value of the given tag.
 *
 * Something like:
 * \verbatim
 *  111 <b>22 2 2</b> 333
 * \endverbatim
 * Will become:
 * <pre>
 *   cdata: 111
 *   b
 *     content:
 *       cdata: 22 2 2  #TODO<--test this
 *   cdata: 333
 * </pre>
 * 
 * \todo should have case sensitive switch
 * \todo WARNING!! something like in html where img, br, hr, etc do not have closing tags
 *   are not dealt with properly... need to flesh out stand_alone_tag_list
 * \todo this should be thoroughly gone over by someone who actually knows something about XML
 *   in current state is rather sloppy, and is just barely enough to be useful in most cases
 *   I've dealt with recently...
 */
Attribute *XMLChunkToAttribute(Attribute *att,
							   const char *buf, //! Buffer with the xml
							   long n,          //! Length of buf
							   long *C,         //! Current position in buf which gets updated
							   const char *until, //*** might not be necessary...
							   const char **stand_alone_tag_list)
{
	if (!att) att=new Attribute;

	char *error=NULL, *value=NULL, *name=NULL;
	char *nm,*vl,*e;
	char final;
	long c,c2;
	int hassubs=1;
	if (C) c=*C; else c=0;
	
	while (c<n && !error) {
		skipws(buf,n,&c);
		if (c<n && buf[c]!='<') { 
			 // add a cdata block, all till '<' or eof
			c2=c;
			while (c<n && buf[c]!='<') c++;
			value=newnstr(buf+c2,c-c2);
			att->push("cdata:",value);
			delete[] value;
		}
		if (c<n && buf[c]=='<' && n>c+1) { 
			hassubs=1;
			
			 // found a tag at last
			c++;
			if (buf[c]=='/') {
				 // this is a closing tag!
				if (C) *C=c-1;
				break;
			}
			skipws(buf,n,&c);
			c2=c;
			 //scan for tag name text as a string of non-whitespace, and not '/' or '>'
			while (c<n && !isspace(buf[c]) && buf[c]!='/' && buf[c]!='>') c++;
			 //now c2 points to beginning of name, c points to char just after name
			if (c==c2) {
				makestr(error,"Empty Tag! Aborting..");
				break;
			} else name=newnstr(buf+c2,c-c2);
			att->push(name);
			
			 //detect <?... ?> and <!...> and <!--...-->
			if (name[0]=='!' && name[1]=='-' && name[2]=='-') {
				 // found a comment!! must parse specially
				 // c points to char just after "!--"
				e=const_cast<char*>(strstr(buf+c,"-->"));
				if (!e) c2=n-c; else c2= (e-(buf+c)); //c2==length of comment not including '-->'
				makenstr(att->attributes.e[att->attributes.n-1]->value,buf+c,c2);
				c=c+c2+3;
				if (c>n) c=n;
				continue;
			} else if (name[0]=='!') {
				final='!';
				hassubs=0;
			} else if (name[0]=='?') {
				final='?';
				hassubs=0;
			} else final='/';

			 // parse in xml attributes name=value name="value"... until > or (final)>
			skipws(buf,n,&c);
			while (c<n && buf[c]!='>' && buf[c]!=final) {
				 // name=value
				 // name="value"
				 // name = "val; val  val \" vala \""
				NameValueAttribute(buf+c, &nm, &vl, &e, '=', 0, "/>");
				if (e!=buf+c) {
					att->attributes.e[att->attributes.n-1]->push(nm,vl);
					c=e-buf;
				} else break;
				skipws(buf,n,&c);
			}
			
			 //advance past final '>'
			c2=c;
			while (c<n && buf[c]!='>' && buf[c]!=final) c++;
			if (c<n) {
				if (buf[c]==final && buf[c+1]=='>') { c+=2; hassubs=0; }
				else if (buf[c]=='>') c++;
			}
			 
			 //add all the xml sub-elements  ==  Attribute sub-attributes)
			 // and parse out the closing tag
			if (hassubs && !one_of_them(name,stand_alone_tag_list)) {
				att->attributes.e[att->attributes.n-1]->push("content:");
				XMLChunkToAttribute(att->attributes.e[att->attributes.n-1]
									  ->attributes.e[att->attributes.e[att->attributes.n-1]->attributes.n-1],
									buf,n, &c, name, stand_alone_tag_list);

				 //should be the closing tag for name now
				skipws(buf,n,&c);
				c2=c;
				if (c<n && buf[c]=='<') {
					c++; //skip the '<'
					skipws(buf,n,&c);
					if (buf[c]=='/') {
						c++;
						skipws(buf,n,&c);
						if (c+(long)strlen(name)<n && !strncmp(name,buf+c,strlen(name))) {
							c+=strlen(name);
							skipws(buf,n,&c);
							c++; //*** assuming final '>'
							Attribute *att2=att->attributes.e[att->attributes.n-1];
							if (att2->attributes.n==1 
								  && !att2->value
								  && !strcmp(att2->attributes.e[0]->name,"content:")
								  && att2->attributes.e[0]->attributes.n==0)	{
								makestr(att2->value,att2->attributes.e[0]->value);
								att2->attributes.flush();
							}
						} else {
							makestr(error,"Missing end tag");
						}
					} else {
						makestr(error,"Missing end tag");
					}
				} else {
					makestr(error,"Missing end tag");
				}
				if (error) c=c2; //reposition to start of mystery tag
				if (C) *C=c;
			}

		} else c++; //was at final char, which was a '<'
	}
	DBG if (error) cerr <<"XML-in error: "<<error<<" near "<<(name?name:"unknown")<<endl;
	 
	 //slightly flatten attribute for simple values, so
	 //<title>blah</title> -->  name=title, value=blah
	if (att->attributes.n==1 
		  && !att->value
		  && !strcmp(att->attributes.e[0]->name,"cdata:")
		  && att->attributes.e[0]->attributes.n==0)	{
		makestr(att->value,att->attributes.e[0]->value);
		att->attributes.flush();
	}
	return att;
}



//---------------------------------- CSS Conversion helpers -------------------------------
//! Write out a specially formatted Attribute to a CSS file.
/*! Return 0 for success, or nonzero error.
 */
int AttributeToCSSFile(FILE *f, Attribute *att, int indent)
{
	if (!f) return 1;
	char *str=AttributeToCSS(att,NULL,NULL);
	if (isblank(str)) {
		if (str) delete[] str;
		return 1;
	}

	fwrite(str,1,strlen(str),f);

	fclose(f);
	return 1;
}

char *AttributeToCSS(Attribute *att,char **appendtothis, char **error_ret)
{
	cerr << " *** must implement AttributeToCSS()!!"<<endl;
	return NULL;
}

//! Create a specially formatted Attribute with data from a CSS file.
/*! Cascading style sheet files have this kind of syntax:
 * \code
 *
 *  selector [, selector2, ...][:pseudo-class] {
 *    property: value;
 *     [property2: value2;
 *       ...]
 *       }
 *  \/\* comment *\/
 *  \endcode
 *
 *  \todo *** implement me!!
 */
Attribute *CSSFileToAttribute (const char *cssfile,Attribute *att)
{
	cerr << " *** must implement CSSFileToAttribute()!!"<<endl;
	return NULL;

//	FILE *f=fopen(file,"r");
//	if (!f) return NULL;
//
//	if (att==NULL) att=new Attribute;
//	
//	fclose(f);
//	return att;
}

Attribute *CSSToAttribute (const char *css,Attribute *att)
{
	cerr << " *** must implement CSSToAttribute()!!"<<endl;
	return NULL;
}

/*! Parse a string like "fill-color:#123456; stroke-opacity: .6" into an Attribute:
 * <pre>
 *   fill-color      #123456
 *   stroke-opacity  .6
 * </pre>
 */
Attribute *InlineCSSToAttribute (const char *thecss, Attribute *att)
{
	if (!att) att=new Attribute;

	//------------------
	//const char *p=css;
	//	
    //
	//do {
	//	while (isspace(*p)) p++;
	//} while (p && *p);
    //
	//------------------

	char *p;
	char *css = newstr(thecss);
	int n=0;
	char **strs = spliton(css, ';', &n);
	for (int c=0; c<n; c++) {
		p=strs[c];
		stripws(p, 3); //removes initial and trailing ws in place

		int n2=0;
		char **namevalue = spliton(p, ':', &n2);
		if (n2==1) {
			stripws(namevalue[0], 3);
			att->push(namevalue[0]);
		} else if (n2==2) {
			stripws(namevalue[0], 3);
			stripws(namevalue[1], 3);
			att->push(namevalue[0], namevalue[1]);
		}
		delete[] namevalue;
	}

	delete[] strs;

	//------------------


	return att;
}


//---------------------------------- JSON Conversion helpers -------------------------------

/*! Negative indent means output with no whitespace.
 */
char *AttributeToJsonString(Attribute *att,char **appendtothis, int indent, char **error_ret)
{
//	char spc[indent<0?0:indent+1]; memset(spc,' ',indent<0?0:indent); spc[indent<0?0:indent]='\0';

////	if (indent<0) appendstr(*appendtothis,"{");
////	else {
////		appendstr(*appendtothis,spc);
////		appendstr(*appendtothis,"{\n");
////	}
//
//	for (int c=0; c<att->attributes.n; c++) {
//		if (indent>0) appendstr(*appendtothis,spc);
//
//		 //name
//		appendstr(*appendtothis,"\"");
//		appendstr(*appendtothis,att->attributes.e[c]->name);
//		appendstr(*appendtothis,"\": ");
//
//		 //value
//		if (***is string) {
//		} else (*** is value) {
//		} else (*** is object) {
//			appendstr(*appendtothis,"{");
//			AttributeToJson(att->attributes.e[c],appendtothis,indent+4,error_ret);
//			if (indent>0) appendstr(*appendtothis,spc);
//			appendstr(*appendtothis,"}");
//
//		} else (*** is array) {
//		}
//
//		if (c<att->attributes.n-1) appendstr(*appendtothis,",");
//	}
//
//
//	appendstr(*appendtothis,"}\n");

	return NULL;
}

/*! Return 0 for success, or nonzero for error.
 * Negative indent means output with no whitespace.
 *
 * See JsonStringToAttribute() for the expected format.
 */
int AttributeToJsonFile(const char *jsonfile, Attribute *att, int indent)
{
	FILE *f = fopen(jsonfile, "w");
	if (!f) {
		// *** error! could not open for writing!
		return 1;
	}

	int status = DumpAttributeToJson(f, att, indent);

	fclose(f);
	return status;
}

int DumpAttributeToJson(FILE *f, Attribute *att, int indent)
{
	char spc[indent<0?0:indent+1]; memset(spc,' ',indent<0?0:indent); spc[indent<0?0:indent]='\0';

	if (att->flags == JSON_Null) {
		fprintf(f, "null");

	} else if (att->flags == JSON_True) {
		fprintf(f, "true");

	} else if (att->flags == JSON_False) {
		fprintf(f, "false");

	} else if (att->flags == JSON_Int || att->flags == JSON_Float) {
		fprintf(f, "%s", att->value);

	} else if (att->flags == JSON_String) {
		dump_out_quoted(f, att->value, '"');

	} else if (att->flags == JSON_Array) {
		fprintf(f, "[ ");
		if (att->attributes.n) fprintf(f, "\n");
		for (int c=0; c<att->attributes.n; c++) {
			fprintf(f, "%s", spc);
			DumpAttributeToJson(f, att->attributes.e[c], indent+2);
			if (c != att->attributes.n-1) fprintf(f, ",\n");
			else fprintf(f, "\n");
		}
		if (att->attributes.n) fprintf(f, "%s", spc);
		fprintf(f, "]");

	} else if (att->flags == JSON_Object) {
		fprintf(f, "{ ");
		if (att->attributes.n) fprintf(f, "\n");
		for (int c=0; c<att->attributes.n; c++) {
			fprintf(f, "%s", spc);
			dump_out_quoted(f, att->attributes.e[c]->name, '"');
			fprintf(f, ": ");
			if (att->attributes.e[c]->attributes.n == 0) {
				 // *** error! Missing the value to a key!
				break;
			}
			DumpAttributeToJson(f, att->attributes.e[c]->attributes.e[0], indent+2);
			if (c != att->attributes.n-1) fprintf(f, ",\n");
			else fprintf(f, "\n");
		}
		if (att->attributes.n) fprintf(f, "%s", spc);
		fprintf(f, "}");
	}


	return 0;
}


Attribute *JsonFileToAttribute (const char *jsonfile, Attribute *att)
{
	char *contents = read_in_whole_file(jsonfile, NULL, 0);
	Attribute *a = JsonStringToAttribute(contents, att, NULL);
	delete[] contents;
	return a;
}

/*! This uses Attribute::flags to hint what type the attribute is. See JsonAttTypes.
 * Also, att->name will hold "null", "true", "false", "int", "float", "array", or "object".
 *
 * Null, true, and false will have null att->value. name will be "true", "false", or "null".
 *
 * Something like:
 *
 * <pre>
 *   { "hashname" : [ "array", "of", 5, "stuff", 10.0 ],
 *     "hashname2" " null
 *   }
 * </pre>
 *
 * will become: 
 * <pre>
 *   object
 *     key hashname
 *       array
 *         string "array"
 *         string "of"
 *         int 5
 *         string "stuff"
 *         float 10.0
 *     key hashname2
 *       null
 * </pre>
 */
Attribute *JsonStringToAttribute (const char *jsonstring, Attribute *att, const char **end_ptr)
{
	if (!att) att = new Attribute();

	const char *str = jsonstring;
	const char *strend;
	char *endptr;

	while (*str) {
		while (*str && isspace(*str)) str++;

		if (*str=='t' && !strncmp(str, "true", 4)) {
			str += 4;
			att->push("true");
			att->Top()->flags = JSON_True;

		} else if (*str=='f' && !strncmp(str, "false", 5)) {
			str += 5;
			att->push("false");
			att->Top()->flags = JSON_False;

		} else if (*str=='n' && !strncmp(str, "null", 4)) {
			str += 4;
			att->push("null");
			att->Top()->flags = JSON_Null;

		} else if (*str=='"') {
			char *s = QuotedAttribute(str, &endptr);
			if (!s || str == endptr) {
				 // *** uh oh! problem in the parsing!
				s++;

			} else {
				str = endptr;
				att->push("string", s);
				att->Top()->flags = JSON_String;
				delete[] s;
			}

		} else if (*str=='[') {
			str++;
			Attribute *att2 = att->pushSubAtt("array");
			att2->flags = JSON_Array;

			while (*str && isspace(*str)) str++;
			while (1) {
				endptr = NULL;
				Attribute *att3 = JsonStringToAttribute(str, NULL, &strend);
				if (att3) {
					att2->push(att3, -1);
					str = strend;
					while (*str && isspace(*str)) str++;

				} else break;
				
				while (*str && isspace(*str)) str++;
				if (*str != ',') break;
				str++;
			}

			while (*str && isspace(*str)) str++;
			if (*str != ']') {
				// *** error!! missing close bracket
			}

		} else if (*str=='{') {
			str++;
			Attribute *att2 = att->pushSubAtt("object");
			att2->flags = JSON_Object;

			while (1) {

				while (*str && isspace(*str)) str++;
				if (*str != '"') {
					if (*str != '}') {
						// *** error! expecting a string!
					}
					break;
				}

				str++;
				strend = str+1;
				while (*strend && *strend != '"') {
					if (*strend == '\\') strend++;
					if (*strend) strend++;
				}

				if (*strend == '\0' || *strend != '"') {
					// *** badly formed string!
					break;
				}

				Attribute *key = new Attribute("key", NULL);
				makenstr(key->value, str+1, strend-str-1);
				att2->push(key, -1);

				endptr = NULL;
				Attribute *value = JsonStringToAttribute(str, NULL, &strend);
				if (value) {
					key->push(value, -1);
					str = strend;

				} else break;
				
				while (*str && isspace(*str)) str++;
				if (*str != ',') break;
				str++;
			}

			while (*str && isspace(*str)) str++;
			if (*str != '}') {
				// *** error!! missing close curly brace
				break;
			}

		} else if (isdigit(*str) || *str == '.') {
			 //is number
			int isint = 1;
			strend = str;

			while (isdigit(*strend)) strend++; //int part
			if (*strend == '.') { //fraction
				isint = 0;
				strend++;
				while (isdigit(*strend)) strend++;
			}
			if (*strend == 'e' || *strend == 'E') {
				strend++;
				if (*strend == '+' || *strend == '-') strend++;
				if (!isdigit(*strend)) {
					// *** malformed number!
					break;
				}
				while (isdigit(*strend)) strend++;
			}
			//if (str == strend) { break; }  <- this shouldn't happen

			Attribute *att2 = new Attribute(isint ? "int" : "float", NULL);
			makenstr(att2->value, str, strend-str);
			str = strend;

			att->push(att2, -1);
		}
	}


	if (end_ptr) *end_ptr = str;

	return att;
}


} //namespace

