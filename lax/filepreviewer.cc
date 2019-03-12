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
//    Copyright (c) 2004-2010,2012 Tom Lechner
//

#include <lax/filepreviewer.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/fileutils.h>
#include <lax/language.h>
#include <lax/doublebbox.h>

//template implementation:
#include <lax/lists.cc>

#include <sys/stat.h>
#include <cstdio>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace LaxFiles;

namespace Laxkit {


//------------------------------------------ PreviewerFunction ---------------------------------------

/*! \class PreviewerFunction
 * \brief File previewer module to handle different kinds of files.
 *
 * One might handle images, others movies, others 3-d shapes. Previewers can optionally
 * allow animation when mouse over at a single specified frame rate, and/or respond to
 * simple mouse movement.
 *
 * This class is meant only for making dynamic thumbnails of files, not for overlaying
 * file name or metadata. It is used, for instance, in FilePreviewer.
 */
class PreviewerFunction
{
  public:
	char *extensions;
	double frames_per_second;
	int pannable;

	PreviewerFunction(const char *ext, double fps, int canpan);
	virtual ~PreviewerFunction();

	virtual int Handles(const char *filename) = 0;
	virtual void Refresh(aDrawable *dest, const char *filename, DoubleBBox *box) = 0;
	virtual void FitTo(double x,double y, double w,double h) = 0;

	virtual void Move(double x,double y, double dx,double dy) {}
};

PreviewerFunction::PreviewerFunction(const char *ext, double fps, int canpan)
{
	extensions=newstr(ext);
	frames_per_second=fps;
	pannable=canpan;
}

PreviewerFunction::~PreviewerFunction()
{
	if (extensions) delete[] extensions;
}




static PtrStack<PreviewerFunction> previewers;

//! Return a PreviewerFunction object that can display filename, or NULL.
PreviewerFunction *FindPreviewer(const char *filename)
{
	for (int c=0; c<previewers.n; c++) {
		if (previewers.e[c]->Handles(filename)) return previewers.e[c];
	}
	return NULL;
}

//! Add previewer to list of file previewer objects.
int AddPreviewer(PreviewerFunction *previewer)
{
	previewers.push(previewer);
	return 1;
}

//! If previewers.n>0, then nothing is done. Returns number added.
int InitializeDefaultPreviewers()
{
	if (previewers.n>0) return 0;

	// *** for instance, add image viewer, hedron viewer, movie viewer
	return 0;
}



//------------------------------------------ FilePreviewer ---------------------------------------

/*! \class FilePreviewer
 * \brief Previews images if possible, otherwise shows a snippet of text.
 *
 * This class only shows a basic box with file name written in it. For a previewer
 * with some changeable dialog info see ImageDialog.
 * 
 * If a file cannot be recognized as an image, the previewer puts
 * some text roughly corresponding to the contents of the file. Utf8, latin-1,
 * and ascii text it prints as it is.. If there are '\\0' bytes in the file,
 * it is assumed to be binary, and each non-ascii is printed as '*' 
 * or 'FF' or some such.
 * 
 * If FILEPREV_SHOW_DIMS is passed in as a style, then when showing images, also 
 * write out the dimensions and file size.
 * 
 * \todo ***if text, should be able to toggle between ascii(latin-1)/utf-8, binary hex.
 */
/*! \var long FilePreviewer::sizelimit
 * \brief File size above which images are not loaded, in kilobytes
 */
/*! \fn const char *FilePreviewer::Preview()
 * \brief Return name of file being previewed.
 */
/*! \var int FilePreviewer::state
 * \brief What is being previewed
 * - 0=question mark (usually NULL filename)
 * - 1=bad filename
 * - 2=text
 * - 3=image
 */ 

#define QUESTIONMARK  0
#define BADFILE       1
#define TEXT          2
#define IMAGE         3


//MessageBar::MessageBar(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
//						int nx,int ny,int nw,int nh,int brder,const char *newtext) 
FilePreviewer::FilePreviewer(anXWindow *pwindow,const char *nname,const char *ntitle,unsigned long nstyle,
						int nx,int ny,int nw,int nh,int brder,
						const char *file,int sz) 
				: MessageBar(pwindow,nname,ntitle,nstyle|MB_LEFT, nx,ny, nw,nh,brder, NULL) 
{
	filename=NULL;
	image=NULL;

	state=0;

	Preview(file);
}

FilePreviewer::~FilePreviewer()
{
	if (filename) delete[] filename;
	if (image) image->dec_count();
}

//int FilePreviewer::init()
//{
//	XSetWindowBackground(app->dpy,window,app->color_bg);
//	return 0;
//}

//! Change what is being previewed to file. ***this function currently rather sucks
int FilePreviewer::Preview(const char *file)
{
	if (!file) { // remove preview
		if (image) {
			image->dec_count();
			image=NULL;
		}

		SetText("?");
		makestr(filename,"");
		state=0;
		needtodraw=1;
		return 0;
	}

	needtodraw=1;
	//if (filename && !strcmp(file,filename)) return 0;***always regenerate so skip this line
	makestr(filename,file);
	
	if (image) {
		image->dec_count();
		image=NULL;
	}

	if (file_exists(file,1,NULL) == S_IFDIR) {
		SetText(_("(directory)"));
		state=1;
		return 0; 
	}

	image = ImageLoader::LoadImage(file);
	if (image) { state=3; return 0; }
	
	char blah[25+strlen(file)];
	if (file_exists(file,1,NULL)!=S_IFREG) {
		sprintf(blah,"File doesn't exist:\n%s",file);
		SetText(blah);
		state=1;
		return 0; 
	}
	FILE *f=fopen(file,"r"); 
	if (!f) { 
		sprintf(blah,"Bad File:\n%s",file);
		SetText(blah);
		state=1;

		//--------------
		return 0; 
	}
	state=2; // text

	//***read in a bit of the file and convert non-printing to ???
	//	figure out if is binary, utf8, latin1, ascii, etc...
	char buffer[1024];
	int c;
	c=fread(buffer,sizeof(char),1024,f);
	buffer[c]='\0';
	for (int cc=0; cc<c; cc++) {
		if (buffer[cc]<32 && buffer[cc]!='\n') buffer[cc]='.';
	}
	fclose(f);
	SetText(buffer);//***todo: fill window, not do standard messagebar? redefine SetText?
	return 0;
}

////! ***warning:broken func..Return whether the first n chars of buffer seem to be ascii, latin-1, utf8, or binary.
// * Return 0 for binary, 1 for ascii, 2 for latin-1, 3 for utf8
// *
// * Treats \\r\\n and \\n and \\r as valid line breaks.
// * 
// * \todo *** see fontconfig or fltk for some utf functions, this one will most likely
// * be superfluous.
// */
//int typeoftext(char *buffer,int n)
//{
//	int t=1; //assume ascii to start
//	unsigned char ch;
//	for (int c=0; c<n; c++) {
//		ch=(unsigned char) buffer[c];
//		if (isascii(ch)) continue;
//		if (ch<32 && ch!=13 && ch!=10 && ch!=9) return 0; //binary (non-printing) char
////		if (ch>127) *** might be latin-1, might be utf-8... fontconfig has utf stuff...
////			*** see also utf8test of fltk, utf8from/toa, and other fltk non-fltk functions!!
//	}
//	return t;
//}


/*! *** should show image x/y/maybe other exif stuff? zooming?
 */
void FilePreviewer::Refresh()
{
	if (!win_on || !needtodraw) return;

	Displayer *dp=MakeCurrent();
	
	if (state!=3) { 
		MessageBar::Refresh(); 
		needtodraw=0;

	} else {
		needtodraw=0;

		dp->NewBG(win_themestyle->bg);
		dp->NewFG(win_themestyle->fg);
		dp->ClearWindow();
		
		if (image) {
			int w,h;
			w=image->w();
			h=image->h();
			double scale;
			if (w<win_w && h<win_h) scale=1;
			else if (w<win_w && h>=win_h) scale=(double)win_h/h;
			else if (w>=win_w && h<win_h) scale=(double)win_w/w;
			else {
				scale=(double)win_h/h;
				if (w*scale>=win_w) scale=(double)win_w/w;
			}

			w=int(w*scale);
			h=int(h*scale);
			dp->imageout(image, win_w/2-w/2,win_h/2-h/2, w,h);

		} else {
			MessageBar::Refresh();
		}
	}

	 // draw filename with image
	if (Preview()) {
		char *text=const_cast<char*>(strrchr(Preview(),'/'));
		if (!text) text=newstr(Preview()); 
		else {
			text=newstr(text);
			prependstr(text,"...");
		}

		if (win_style&FILEPREV_SHOW_DIMS && image) {
			 // add on dimensions..
			char extra[50];
			sprintf(extra,", %dx%d",image->w(),image->h());
			appendstr(text,extra);
		}

		double w,h;
		dp->textextent(text,-1,&w,&h);
		dp->NewFG(win_themestyle->bg);
		dp->drawrectangle(win_w/2-w/2 - 2,win_h-h - 4,w+4,h+4, 1);
		dp->drawthing(win_w/2-w/2-2,win_h-h/2-2, h/2,h/2+2, 1,THING_Circle);
		dp->drawthing(win_w/2+w/2+2,win_h-h/2-2, h/2,h/2+2, 1,THING_Circle);
		dp->NewFG(win_themestyle->fg);
		dp->textout(win_w/2,win_h-2, text,-1, LAX_HCENTER|LAX_BOTTOM);
		delete[] text;

		if (image) {
			 //write out file pixel size
			char size[50];
			sprintf(size,"%d x %d",image->w(),image->h());

			dp->textextent(size,-1,&w,&h);
			dp->NewFG(win_themestyle->bg);
			dp->drawrectangle(win_w/2-w/2 - 2,win_h-2*h - 4,w+4,h+4, 1);
			dp->drawthing(win_w/2-w/2-2,win_h-h/2-2-h, h/2,h/2+2, 1,THING_Circle);
			dp->drawthing(win_w/2+w/2+2,win_h-h/2-2-h, h/2,h/2+2, 1,THING_Circle);
			dp->NewFG(win_themestyle->fg);
			dp->textout(win_w/2,win_h-2-h, size,-1, LAX_HCENTER|LAX_BOTTOM);
		}
		
	}
	
	return;
}


////! Should regenerate preview?
//int FilePreviewer::LBDown(int x,int y,unsigned int state,int count)
//{***
//	buttondown|=LEFTBUTTON;
////cout << "thetext:"<<thetext;
//	return 1;
//}
//
//int FilePreviewer::LBUp(int x,int y,unsigned int state)
//{***
//	buttondown|=LEFTBUTTON;
//	if (!(win_style&MB_COPY))return 1;
//	 //	copy to clipboard
//	char *str=GetText();
//	app->CopytoBuffer(str,strlen(str)); 
//cout <<"(mb copy: "<<str<<")\n";
//	delete[] str;
//	return 0;
//}
//
//int FilePreviewer::RBDown(int x,int y,unsigned int state,int count)
//{//***
//	buttondown|=RIGHTBUTTON;
//	msx=x;
//	msy=y;
//	return 0;
//}
//
//int FilePreviewer::RBUp(int x,int y,unsigned int state)
//{//***
//	buttondown&=~RIGHTBUTTON;
//	return 0;
//}
//
//int FilePreviewer::MoveResize(int nx,int ny,int nw,int nh)
//{//***
//	anXWindow::MoveResize(nx,ny,nw,nh);
//	SetupMetrics();
//	needtodraw=1;
//	return 0;
//}
//
//int FilePreviewer::Resize(int nw,int nh)
//{//***
//	anXWindow::Resize(nw,nh);
//	SetupMetrics();
//	needtodraw=1;
//	return 0;
//}
//

	
} // namespace Laxkit
