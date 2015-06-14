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
//    Copyright (C) 2013 by Tom Lechner
//


#include <lax/interfaces/somedatafactory.h>
#include <lax/interfaces/captioninterface.h>
#include <lax/interfaces/viewportwindow.h>
#include <lax/transformmath.h>
#include <lax/strmanip.h>
#include <lax/utf8utils.h>
#include <lax/language.h>

#include <lax/lists.cc>

using namespace LaxFiles;
using namespace Laxkit;

#include <iostream>
using namespace std;
#define DBG 

#define sgn(a) ((a)<0?-1:((a)>0?1:0))

namespace LaxInterfaces {

//--------------------------------- CaptionData -------------------------------
/*! \class CaptionData
 * \brief Holds a little bit of text.
 * 
 * This is for very simple text labels and such. There can be multiple lines
 * of text with variable centering. One CaptionData unit should be considered one "point"
 * when the object is scaled same as paper.
 */
/*! \var double CaptionData::xcentering
 * \brief 0 is left, 100 is right, 50 is center, and other number is partial centering.
 *
 * All centering is around the origin.
 */
/*! \var double CaptionData::ycentering
 * \brief 0 is top, 100 is bottom, 50 is center, and other number is partial centering.
 *
 * All centering is around the origin.
 */
/*! \var char *CaptionData::font
 * \brief The filename of the font to use.
 */
/*! \var double CaptionData::fontsize
 * \brief The point size of the font.
 *
 * This size is relative to the object's coordinate space which are assumed to be inches.
 * Thus a fontsize of 1pt will be 1/72nd of 1 unit in the object's space.
 */

	
CaptionData::CaptionData(const char *ntext, const char *nfontname, const char *nfontstyle,  int fsize, double xcenter, double ycenter)
  : lines(2)
{
	DBG cerr <<"in CaptionData constructor"<<endl;
	
	fontname=newstr(nfontname);
	fontstyle=newstr(nfontstyle);
	fontfile=NULL;
	//font=NULL;
	fontsize=fsize;
	font=NULL;
	state=0;

	int numlines=0;
	char **text=split(ntext,'\n',&numlines);
	for (int c=0; c<numlines; c++) {
		lines.push(text[c]);
		linelengths.push(0);
	}
	xcentering=xcenter;
	ycentering=ycenter;
	//centerline=NULL;

	red=green=blue=.5;
	alpha=1.;

	DBG if (ntext) cerr <<"CaptionData new text:"<<endl<<ntext<<endl;
	DBG cerr <<"..CaptionData end"<<endl;
}

CaptionData::~CaptionData()
{
	DBG cerr <<"in CaptionData destructor"<<endl;

	if (fontname) delete[] fontname;
	if (fontstyle) delete[] fontstyle;
	if (fontfile) delete[] fontfile;
	if (font) font->dec_count();

	DBG cerr <<"-- CaptionData dest. end"<<endl;
}
	
int CaptionData::CharLen(int line)
{
	if (line<0 || line>=lines.n) return 0;
	return strlen(lines.e[line]);
}

//! Update cached pixel length of line. Also return that value.
/*! If line<0 then compute for all lines, and returns maximum length.
 */
int CaptionData::ComputeLineLen(int line)
{
	if (line<0) {
		int max=0,w;
		for (int c=0; c<lines.n; c++) {
			w=ComputeLineLen(c);
			if (w>max) max=w;
		}
		return w;
	}

	//****
	return 0;
}

/*! 
 * Default dump for a CaptionData. 
 *
 * Dumps:
 * <pre>
 *  font aontuhaot.ttf
 *  fontsize 10
 *  xcentering  50
 *  ycentering  50
 *  text \
 *    Blah Blah.
 *    Blah, "blah blah"
 * </pre>
 * width and height are the integer pixel dimensions of the image.
 *
 * Ignores what. Uses 0 for it.
 */
void CaptionData::dump_out(FILE *f,int indent,int what,LaxFiles::DumpContext *context)
{
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	if (fontname) fprintf(f,"%sfontname \"%s\"\n",spc,fontname);
	if (fontstyle) fprintf(f,"%sfontstyle \"%s\"\n",spc,fontstyle);
	fprintf(f,"%sfontsize %.10g\n",spc,fontsize);
	fprintf(f,"%sxcentering %.10g\n",spc,xcentering);
	fprintf(f,"%sycentering %.10g\n",spc,ycentering);
	fprintf(f,"%smatrix %.10g %.10g %.10g %.10g %.10g %.10g\n",spc,
				m(0),m(1),m(2),m(3),m(4),m(5));
	fprintf(f,"%scolor rgbaf %.10g %.10g %.10g %.10g \n",
				spc, red,green,blue,alpha);
	if (lines.n) {
		fprintf(f,"%stext \\\n",spc);
		for (int c=0; c<lines.n; c++) {
			fprintf(f,"%s  %s\n",spc,lines.e[c]); // *** this destroys spaces!!
		}
	}
}
	
//! See dump_out().
void CaptionData::dump_in_atts(Attribute *att,int flag,LaxFiles::DumpContext *context)
{
	if (!att) return;
	char *name,*value;
	minx=miny=0;
	maxx=maxy=-1;
	for (int c=0; c<att->attributes.n; c++) {
		name= att->attributes.e[c]->name;
		value=att->attributes.e[c]->value;
		if (!strcmp(name,"matrix")) {
			double mm[6];
			DoubleListAttribute(value,mm,6);
			m(mm);
		} else if (!strcmp(name,"xcentering")) {
			DoubleAttribute(value,&xcentering);
		} else if (!strcmp(name,"ycentering")) {
			DoubleAttribute(value,&ycentering);
		} else if (!strcmp(name,"text")) {
			SetText(value);
		} else if (!strcmp(name,"fontname")) {
			fontname=newstr(value);
		} else if (!strcmp(name,"fontstyle")) {
			fontstyle=newstr(value);
		} else if (!strcmp(name,"fontsize")) {
			DoubleAttribute(value,&fontsize);
		} else if (!strcmp(name,"color")) {
			double co[4];
			int n=DoubleListAttribute(value,co,4);
			if (n!=4) continue;
			red  =co[0];
			green=co[1];
			blue =co[2];
			alpha=co[3];
		}
	}
}

//! Set new text.
/*! Accepts multi line text, where lines are delineated with '\n'.
 * 
 * Returns 0 for success, 1 for error.
 */
int CaptionData::SetText(const char *newtext)
{
	lines.flush();
	linelengths.flush();

	int numlines=0;
	char **text=split(newtext,'\n',&numlines);
	for (int c=0; c<numlines; c++) {
		lines.push(text[c]);
		linelengths.push(0);
	}
	state=0;
	FindBBox();
	return 0;
}

int CaptionData::DeleteChar(int line,int pos,int after, int *newline,int *newpos)
{
	if (after) {
		if (pos==(int)strlen(lines.e[line]) && line<lines.n-1) {
			 //combine lines
			appendstr(lines.e[line],lines.e[line+1]);
			lines.remove(line+1);
			linelengths.remove(line+1);
			ComputeLineLen(line);

		} else if (pos<(int)strlen(lines.e[line])) {
			 //delet char within a line
			const char *p=utf8fwd(lines.e[line]+pos+1,lines.e[line],lines.e[line]+strlen(lines.e[line])); 
			int cl=p-(lines.e[line]+pos);
			memmove(lines.e[line]+pos,lines.e[line]+pos+cl,strlen(lines.e[line]+cl-pos));
			ComputeLineLen(line);
		}
	} else {
		if (pos==0 && line>0) {
			 //combine lines
			appendstr(lines.e[line-1],lines.e[line]);
			lines.remove(line);
			linelengths.remove(line);
			line--;
			pos=strlen(lines.e[line]);
			ComputeLineLen(line);
		} else if (pos>0) {
			const char *p=utf8back(lines.e[line]+pos-1,lines.e[line],lines.e[line]+strlen(lines.e[line]));
			int cl=(lines.e[line]+pos)-p;
			memmove(lines.e[line]+pos-cl,lines.e[line]+pos,strlen(lines.e[line]-pos));
			pos-=cl;
			ComputeLineLen(line);
		}
	}

	if (newline) *newline=line;
	if (newpos) *newpos=pos;
	return 0;
}

int CaptionData::InsertChar(unsigned int ch, int line,int pos, int *newline,int *newpos)
{
	if (ch=='\n') {
		 //add new line
		char *str=new char[2]; str[0]=ch; str[1]='\0';

		lines.push(str, 2, line);
		linelengths.push(0, line);

		ComputeLineLen(line);
		line++;
		ComputeLineLen(line);
		pos=0;
		*newpos=pos;
		*newline=line;

	} else {
		char utf8[10];
		int cl=utf8encode(ch,utf8);
		utf8[cl]='\0';

		char *nline=new char[strlen(lines.e[line]+cl+1)];
		*nline='\0';
		if (pos) {
			strncpy(nline,lines.e[line],pos);
			nline[pos]='\0';
		}
		strcat(nline,utf8);
		strcat(nline,lines.e[line]+pos);

		delete[] lines.e[line];
		lines.e[line]=nline;
		ComputeLineLen(line);
		pos+=cl;
		*newpos=pos;
	}

	return 0;
}

/*! Assumes font is accurate????
 *
 * \todo *** must be able to find the actual extent of the text!
 *   right now, the interface must handle bbox finding.
 */
void CaptionData::FindBBox()
{//***
	double height=lines.n*fontsize,
	       width=height;
//	int w=h;
//	int ww,hh; 
//	loadfont();
//	if (font) {
//		for (int c=0; c<lines.n; c++) {
//			imlib_get_text_size(text[c],&ww,&hh);
//			if (ww>w) w=ww;
//		}
//		*** note that ww,hh are pixel values based on font, not scaled screen values
//	}

	if (lines.n) {
		width=0;
		for (int c=0; c<lines.n; c++) {
			if (linelengths.e[c]>width) width=linelengths.e[c];
		}
	}

	minx=-xcentering/100*width;
	maxx=minx+width;
	miny=-ycentering/100*height;
	maxy=miny+height;
}

//! Set horizontal centering, and adjust bbox.
double CaptionData::XCenter(double xcenter)
{
	xcentering=xcenter;
	double w(maxx-minx);
	minx=-xcentering/100*w;
	maxx=minx+w;

	return xcentering;
}

//! Set vertical centering, and adjust bbox.
double CaptionData::YCenter(double ycenter)
{
	ycentering=ycenter;
	double h(maxy-miny);
	miny=-ycentering/100*h;
	maxy=miny+h;

	return ycentering;
}

//--------------------------------- CaptionInterface ---------------------------------
/*! \class CaptionInterface
 * \brief Interface for manipulating CaptionData objects.
 *
 * \todo this could also be used for little sticky notes in the viewport, these would
 *   be text blocks that are not transformed with the viewport's matrix..
 */
/*! \var int CaptionInterface::mode
 * 
 * If mode==0, then edit existing text normally. If mode==1, then we are
 * working on a brand new object. If the interface is called off without actually
 * writing any text, then do not install the object.
 */


CaptionInterface::CaptionInterface(int nid,Displayer *ndp, const char *newtext) : anInterface(nid,ndp)
{
	data=NULL;
	coc=NULL;
	showdecs=3;
	mode=0;

	needtodraw=1;

	if (newtext) {
		if (!data) data=new CaptionData(newtext,
						 "sans", //font name   //"/usr/X11R6/lib/X11/fonts/TTF/temp/hrtimes_.ttf",
						 NULL, //font style
						 36, //font size
						 0,  //xcenter,
						 0); //ycenter
	}
}

CaptionInterface::~CaptionInterface()
{
	DBG cerr <<"----in CaptionInterface destructor"<<endl;
	deletedata();
}

//! Return new CaptionInterface.
/*! If dup!=NULL and it cannot be cast to CaptionInterface, then return NULL.
 */
anInterface *CaptionInterface::duplicate(anInterface *dup)
{
	if (dup==NULL) dup=new CaptionInterface(id,NULL,NULL);
	else if (!dynamic_cast<CaptionInterface *>(dup)) return NULL;
	return anInterface::duplicate(dup);
}

const char *CaptionInterface::Name()
{ return _("Caption Tool"); }

//! Check out nobj and use it as the data if possible.
int CaptionInterface::UseThis(anObject *nobj,unsigned int mask)
{
	return 0;
}

//! Sets showdecs=3, and needtodraw=1.
int CaptionInterface::InterfaceOn()
{
	DBG cerr <<"CaptionInterfaceOn()"<<endl;
	showdecs=3;
	needtodraw=1;
	mode=0;//***


	if (!data) data=new CaptionData("Blah balh aoeunth323\nline2\n  spaced line 3", 
						 "sans", //font name   //"/usr/X11R6/lib/X11/fonts/TTF/temp/hrtimes_.ttf",
						 NULL, //font style
						 36, //font size
						 0,  //xcenter,
						 0); //ycenter
	return 0;
}

//! Calls Clear(), sets showdecs=0, and needtodraw=1.
int CaptionInterface::InterfaceOff()
{
	Clear(NULL);
	showdecs=0;
	needtodraw=1;
	DBG cerr <<"CaptionInterfaceOff()"<<endl;
	return 0;
}

void CaptionInterface::Clear(SomeData *d)
{
	if ((d && d==data) || (!d && data)) {
		data->dec_count(); 
		data=NULL; 
	} 
}

//! Draw ndata, but remember that data should still be the resident data afterward.
int CaptionInterface::DrawData(anObject *ndata,anObject *a1,anObject *a2,int info)
{
	if (!ndata || dynamic_cast<CaptionData *>(ndata)==NULL) return 1;
	CaptionData *d=data;
	data=dynamic_cast<CaptionData *>(ndata);
	int td=showdecs,ntd=needtodraw;
	showdecs=2;//***
	needtodraw=1;
	Refresh();
	needtodraw=ntd;
	showdecs=td;
	data=d;
	return 1;
}

/*! Note that this calls  imlib_context_set_drawable(dp->GetWindow()).
 * Please note that Imlib does not naturally handle skewed fonts, and this function
 * doesn't either.
 *
 * Returns 1 if no data, -1 if thing was offscreen, or 0 if thing drawn.
 *
 * \todo should work this to not need imlib, allowing a redefinable backend to render the text.
 */
int CaptionInterface::Refresh()
{
	if (!dp || !needtodraw) return 0;
	needtodraw=0;
	if (!data) return 1;

	dp->font(data->fontname,data->fontstyle,data->fontsize);//1 for real size, not screen size
	if (!data->state) {
		 //need to find line lengths
		for (int c=0; c<data->lines.n; c++) {
			data->linelengths.e[c]=dp->textextent(NULL, data->lines.e[c],-1, NULL,NULL,NULL,NULL,0);
		}
		data->state=1;
		data->FindBBox();
	}

		
	 //find how large
	flatpoint pb=flatpoint(0,0),
			  pt=flatpoint(0,data->fontsize),
			  ptt=flatpoint(0,data->fontsize*data->lines.n);
			  //pt=flatpoint(0,data->fontsize/72),
			  //ptt=flatpoint(0,data->fontsize/72*data->lines.n);
	int height=(int)norm(dp->realtoscreen(pb)-dp->realtoscreen(pt));
	double totalheight=norm(dp->realtoscreen(pb)-dp->realtoscreen(ptt));
	if (totalheight<.5) return 0;


	if (!coc) dp->PushAndNewTransform(data->m());

	int texttoosmall=0;
	if (height==0) {
		texttoosmall=1;
		//*** just draw little lines
		//return 0;
	}

	 // find the screen box to draw into
	 // these still need the 'dp->' because dp was transformed to space immediately holding data already
	 // The viewport is the base transform, usually different then dp coords...
	flatpoint ul=dp->realtoscreen(flatpoint(data->minx,data->miny)), 
			  ur=dp->realtoscreen(flatpoint(data->maxx,data->miny)), 
			  ll=dp->realtoscreen(flatpoint(data->minx,data->maxy)), 
			  lr=dp->realtoscreen(flatpoint(data->maxx,data->maxy));
	//flatpoint cursor=ul;
	//flatpoint advance=ur-ul;
	flatpoint v=ll-ul;
	v=v/norm(v);
	//double boxtotalheight=norm(ul-ll);
	//double lineheight=boxtotalheight/data->lines.n;
	
	DBG fprintf(stderr,"draw caption scr coords: %ld: ul:%g,%g ur:%g,%g ll:%g,%g lr:%g,%g\n",
	DBG		data->object_id,ul.x,ul.y,ur.x,ur.y,ll.x,ll.y,lr.x,lr.y);
	DBG fprintf(stderr,"     caption bounds:    w:%g  h:%g\n",
	DBG		norm(ul-ur),norm(ul-ll));
	
	 // check for positive intersection of transformed image to dp view area
	DoubleBBox bbox(ul);
	bbox.addtobounds(ur);
	bbox.addtobounds(ll);
	bbox.addtobounds(lr);
	if (!bbox.intersect(dp->Minx,dp->Maxx,dp->Miny,dp->Maxy)) {
		DBG cerr <<"----------------CaptionData outside viewport"<<endl;
		if (!coc) dp->PopAxes();
		return -1;
	}
	//---or---
	//flatpoint pts[4]={dp->screentoreal(ul,ur,ll,lr)}
	//bbox.bounds(pts,4);
	//bbox.intersect(data->minx,data->maxx,data->miny,data->maxy, settointersection);
	
	 //draw the text
	if (!texttoosmall) {

		 //draw the stuff
		double x,y;
		//double width=data->maxx-data->minx;
		y=data->ycentering/100*data->fontsize*data->lines.n;
		for (int c=0; c<data->lines.n; c++, y+=data->fontsize) {
			if (isblank(data->lines.e[c])) continue;
			x=-data->xcentering/100*(data->linelengths[c]);
			dp->textout(x,y, data->lines.e[c],-1, LAX_TOP|LAX_LEFT);

			 //draw caret
			if (c==caretline && showdecs) {
				double ex=dp->textextent(NULL, data->lines.e[c],caretpos, NULL,NULL,NULL,NULL,0);
				dp->drawline(x+ex,y, x+ex,y+data->fontsize);
			}
		}

	} else {
		//*** text is too small, draw little lines
		DBG cerr <<"small text, draw little gray lines instead..."<<endl;
	}
	

	 // draw control points;
	if (showdecs&1) { 
		dp->DrawScreen();
		 // for a box outline? control decorations..
		dp->LineAttributes(1,LineOnOffDash,CapRound,JoinRound);
		dp->NewFG(255,0,0);
		dp->drawline(ul,ur);
		dp->drawline(ur,lr);
		dp->drawline(lr,ll);
		dp->drawline(ll,ul);
//		dp->NewFG(controlcolor);

		 //draw little circle around origin
		dp->LineAttributes(1,LineSolid,CapRound,JoinRound);
		flatpoint p=dp->realtoscreen(0,0);
		dp->drawpoint(p, 10,0);

		dp->DrawReal();
	}

	//DBG cerr<<"..Done drawing CaptionInterface"<<endl;
	if (!coc) dp->PopAxes();
	return 0;
}

ObjectContext *CaptionInterface::Context()
{
	return coc;
}

//! Sets data=NULL.
/*! That results in data->dec_count() being called somewhere along the line.
 */
void CaptionInterface::deletedata()
{
	if (data) data->dec_count();
	data=NULL;
}

//! Create and return new data, also calls viewport->newData(newdata,0).
/*! Please note that this function is not a redefinition of anything. It is used
 * internally to get a new instance of CaptionData and also tell the viewport about it.
 *
 * This function causes a new CaptionData to be created and to have a count of 2.
 * Currently, new CaptionData makes the maxx/maxy be the image pixel width/height..
 *
 * \todo need some flag somewhere to auto flip vertically (for +y is up rather than down)
 */
CaptionData *CaptionInterface::newData()
{//***
	CaptionData *ndata=NULL;
	if (somedatafactory) {
		ndata=static_cast<CaptionData *>(somedatafactory->newObject(LAX_CAPTIONDATA));
		ndata->SetText("Blah balh aoeunth323\nline 2\nthird line");
	} 
	cout << " *** CaptionInterface: need to grab a more generic installed font....."<<endl;
	if (!ndata) ndata=new CaptionData("Blah balh aoeunth323\nline 2\nthird line", 
						 "sans",NULL,
						 //"/home/tom/fonts/temp/usr/X11R6/lib/X11/fonts/TTF/temp/hrtimes_.ttf",
						 36,
						 0,  //xcenter,
						 0);
//	if (sgn(dp->m()[0])!=sgn(dp->m()[3])) ndata->m(3,-ndata->m(3)); //flip if dp is +y==up


	if (viewport) {
		ObjectContext *oc=NULL;
		viewport->NewData(ndata,&oc);//viewport adds only its own counts
		if (coc) delete coc;
		if (oc) coc=oc->duplicate();
	}

	return ndata;
}

//! If !data on LBDown, then make a new one...
int CaptionInterface::LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d)
{//***
	DBG cerr << "  in captioninterface lbd..";
	buttondown.down(d->id,LEFTBUTTON,x,y);
	mousedragged=0;

	lx=mx=x;
	ly=my=y;
	
	 //! Get rid of old data if not clicking in it.
	if (data) return 0;
//	if (data && !data->pointin(screentoreal(x,y)) && (state&LAX_STATE_MASK)==0) {
//		deletedata();
//	}
	
	 // make new one or find other one.
	if (!data) {
		CaptionData *obj=NULL;
		ObjectContext *oc=NULL;
		int c=viewport->FindObject(x,y,whatdatatype(),NULL,1,&oc);
		if (c>0) obj=dynamic_cast<CaptionData *>(oc->obj); //***actually don't need the cast, if c>0 then obj is CaptionData
	 	if (obj) { 
			 // found another CaptionData to work on.
			 // If this is primary, then it is ok to work on other images, but not click onto
			 // other types of objects.
			data=obj;
			data->inc_count();
			if (coc) delete coc;
			coc=oc->duplicate();
			
			if (viewport) viewport->ChangeObject(oc,0);
			needtodraw=1;
			return 0;
		} else if (c<0) {
			 // If there is some other non-image data underneath (x,y) and
			 // this is not primary, then switch objects, and switch tools to deal
			 // with that object.
			//******* this will have to be ChangeObject(oc,transfer lbdown) or some such
			if (!primary && c==-1 && viewport->ChangeObject(oc,1)) {
				buttondown.up(d->id,LEFTBUTTON);
				return 0;
			}
		}

		 // To be here, must want brand new data plopped into the viewport context
		if (viewport) viewport->ChangeContext(x,y,NULL);
		mode=1;
		data=newData(); 
		needtodraw=1;
		if (!data) return 0;
		if (data->maxx>data->minx && data->maxy>data->miny) mode=0;

		leftp=screentoreal(x,y);
		data->origin(leftp);
		data->xaxis(flatpoint(1,0)/Getmag()/2);
		data->yaxis(flatpoint(0,1)/Getmag()/2);
		DBG data->dump_out(stderr,6,0,NULL);
		return 0;
	}

	 // Set leftp to the point in the image that the mouse was clicked down on.
//	flatpoint oo=(screentoreal(x,y)-data->origin()); // real vector from data->origin() to mouse move to 
//	leftp.x=(oo*data->xaxis())/(data->xaxis()*data->xaxis());
//	leftp.y=(oo*data->yaxis())/(data->yaxis()*data->yaxis()); // leftp is in data coords now
//	data->minx=data->maxx=leftp.x;
//	data->miny=data->maxy=leftp.y;
	

////	if (state&ControlMask && state&ShiftMask && data) { // +^lb move around wildpoint
////		return 0;
////	} else if (state&ControlMask && data) { // ^lb focus or end angle
////		return 0;
////	} else if (state&ShiftMask && data) { // +lb start angle
////		return 0;
////	} else { // straight click
////		return 0;
////	}

	needtodraw=1;
	return 0;
	DBG cerr <<"..captioninterfacelbd done   ";
}

//! If data, then call viewport->ObjectMoved(data).
int CaptionInterface::LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{//***
	if (!buttondown.isdown(d->id,LEFTBUTTON)) return 1;
	if (mode==1 && !mousedragged && data) {
		DBG cerr <<"**CaptionInterface Clear() for no mouse move on new data"<<endl;
		if (viewport) viewport->DeleteObject();
		anInterface::Clear();
	} else if (data && viewport) viewport->ObjectMoved(coc,1);
	mode=0;
	buttondown.up(d->id,LEFTBUTTON);
	return 0;
}

int CaptionInterface::MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d) 
{//***
	if (!buttondown.any() || !data) return 0;
	if (x!=mx || y!=my) mousedragged=1;

	if (mode!=1) return 0;

	flatpoint oo=(screentoreal(x,y)-data->origin()); // real vector from data->origin() to mouse move to 
	flatpoint np;
	np.x=(oo*data->xaxis())/(data->xaxis()*data->xaxis());
	np.y=(oo*data->yaxis())/(data->yaxis()*data->yaxis()); // leftp is in data coords now
	if (np.x<data->minx) data->minx=np.x; else data->maxx=np.x;
	if (np.y<data->miny) data->miny=np.y; else data->maxy=np.y;

	mx=x; my=y;
	needtodraw|=2;
	return 0;
}

int CaptionInterface::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d) 
{

	if (!data) return 1;

	if (ch=='d' && (state&LAX_STATE_MASK)==ControlMask) {
		if (--showdecs<0) showdecs=3;
		needtodraw=1;
		return 0;

	//} else if (ch==LAX_Shift) { // shift
	//} else if (ch==LAX_Control) { // cntl

	} else if (ch==LAX_Del) { // delete
		data->DeleteChar(caretline,caretpos,1, &caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Bksp) { // backspace
		data->DeleteChar(caretline,caretpos,1, &caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Home) {
		caretpos=0;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_End) {
		caretpos=data->CharLen(caretline);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Left) { // left //***unmodified from before rect
		caretpos--;
		if (caretpos<0) caretpos=0;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Right) { // right
		caretpos++;
		if (caretpos>data->CharLen(caretline)) caretpos=data->CharLen(caretline);
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Up) { // up
		caretline--;
		if (caretline<0) caretline=0;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Down) { // down
		caretline++;
		if (caretline>=data->lines.n) caretline=data->lines.n-1;
		needtodraw=1;
		return 0;

	} else if (ch==LAX_Enter) {
		data->InsertChar('\n',caretline,caretpos,&caretline,&caretpos);
		needtodraw=1;
		return 0;

	} else if (ch>=32 && ch<0xff00 && (state&(ControlMask|AltMask|MetaMask))==0) {
		 // add character to text
		if (caretline<0) caretline=0;
		if (caretpos<0) caretpos=0;

		data->InsertChar(ch,caretline,caretpos,&caretline,&caretpos);
		needtodraw=1;
		return 0;
	}

	return 1; 
}

int CaptionInterface::KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d) 
{
	return 1; 
}


} // namespace LaxInterfaces


