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
#ifndef _LAX_TEXTONPATHINTERFACE_H
#define _LAX_TEXTONPATHINTERFACE_H


#include <lax/interfaces/aninterface.h>
#include <lax/interfaces/pathinterface.h>
#include <lax/colors.h>
#include <lax/fontmanager.h>
#include <lax/lists.h>


namespace LaxInterfaces { 


//-------------------------- TextOnPath --------------------------------------

class OnPathGlyph : public Laxkit::GlyphPlace
{
  public:
	flatpoint position; //GlyphPlace x,y are the flat layout
	double rotation;
	double scaling;
	bool visible;
	int styleid; //when you have a run of glyphs, but they all come from different fonts

	OnPathGlyph() { rotation=0; scaling=1; styleid=0; visible=true; }
};

class TextOnPath : virtual public SomeData
{
  public:
	unsigned int flags; //what to dump, whether to save text on dump out or just start/end

	enum BaselineType {
		FROM_Default=0,
		FROM_Path,
		FROM_Offset,
		FROM_Stroke,
		FROM_Other_Stroke,
		FROM_Envelope, //encase in path's stroke outline, scaling glyphs accordingly
		FROM_MAX
	} baseline_type;

	double baseline;
	int baseline_units; //em==0, or physical unit
	double start_offset; //distance from start of path
	double end_offset;
	double rotation;
	double path_length;
	int pathdirection;

	int start, end; //byte index in text, used text is in range [start, end)
	char *text;

	int needtorecache;
	int numglyphs;
	Laxkit::PtrStack<OnPathGlyph> glyphs;
	double textpathlen; //distance along path that text actually occupies
	std::time_t cachetime;

	ObjectContext *pathcontext;
	PathsData *paths;
	int pathindex; //path to use within paths
	Path *path; //local path with offset/stroke already applied, this bath is the baseline of laid out text
	Path *offsetpath; //path with offset applied

	double scale_correction;
	Laxkit::LaxFont *font;
	Laxkit::Color *color;
    char *language; //id matching something in fontmanager
    char *script;
    int direction; //rtl, ltr, ttb, btt? use LAX_LRTB stuff?


	unsigned int dumpflags;

	TextOnPath();
	virtual ~TextOnPath();

	virtual const char *whattype() { return "TextOnPath"; }
	virtual void FindBBox();

	virtual void	   dump_out(FILE *f,int indent,int what, LaxFiles::DumpContext *context);
	virtual LaxFiles::Attribute *dump_out_atts(LaxFiles::Attribute *att,int what, LaxFiles::DumpContext *context); 
	virtual void dump_in_atts(LaxFiles::Attribute *att,int flag, LaxFiles::DumpContext *context);

	virtual int Text(const char *newtext, int nstart=0, int nend=-1);
	virtual int Reallocate(int newn);
	virtual int Remap();
	virtual int Font(Laxkit::LaxFont *newfont);
	virtual int UseThisPath(PathsData *newpaths, int path_index);
	virtual double Baseline(double newbaseline, bool diff);
	virtual double Baseline(double newbaseline, bool diff, flatpoint constant);
	virtual double StartOffset(double newoffset, bool diff);
	virtual int PathDirection() { return pathdirection; }
	virtual int PathDirection(int newdir);

	virtual int PointInfo(double position, flatpoint *point_ret, flatpoint *tangent_ret, double *size_ret);

	virtual int CharLen();
	virtual int DeleteChar(int pos,int after, int *newpos);
	virtual int DeleteSelection(int fpos, int tpos, int *newpos);
	virtual int InsertChar(unsigned int ch, int pos, int *newpos);
	virtual int InsertString(const char *txt,int len, int pos, int *newpos);


	virtual SomeData *ConvertToPaths(bool use_clones, Laxkit::RefPtrStack<SomeData> *clones_to_add_to);
};


//-------------------------- TextOnPathInterface --------------------------------------

enum TextOnPathActions {
	TPATH_None=0,
	TPATH_Kern = TextOnPath::FROM_MAX,
	TPATH_KernR,
	TPATH_Baseline,
	TPATH_BaselineUp,
	TPATH_BaselineDown,
	TPATH_Offset,
	TPATH_OffsetDec,
	TPATH_OffsetInc,
	TPATH_BaseAndOff,
	TPATH_EditPath,
	TPATH_ToggleDirection,
	TPATH_ConvertToPath,
	TPATH_Text,
	TPATH_MAX	
};

class TextOnPathInterface : public anInterface
{
  protected:
	int showdecs;

	int caretpos;
	int selstart, sellen;
	int firsttime;
	int showobj;
	 
	TextOnPath *textonpath;
	PathsData *paths;
	ObjectContext *toc;

	double defaultsize;

	flatpoint lasthover;
	int hover_type;
	double hoveralongpath;
	double hovert;

	PathInterface pathinterface;

	Laxkit::ShortcutHandler *sc;

	virtual int send();

  public:
	unsigned int textonpath_style;
	bool allow_edit_path;

	TextOnPathInterface(anInterface *nowner, int nid);
	virtual ~TextOnPathInterface();
	virtual anInterface *duplicate(anInterface *dup);
	virtual const char *IconId() { return "TextOnPath"; }
	const char *Name();
	const char *whattype() { return "TextOnPathInterface"; }
	const char *whatdatatype();
	Laxkit::MenuInfo *ContextMenu(int x,int y,int deviceid, Laxkit::MenuInfo *menu);
	virtual int Event(const Laxkit::EventData *data, const char *mes);
	virtual Laxkit::ShortcutHandler *GetShortcuts();
	virtual int PerformAction(int action);
	virtual ObjectContext *Context(); 

	virtual int UseThis(Laxkit::anObject *nlinestyle,unsigned int mask=0);
	virtual int UseThisObject(ObjectContext *oc);
	virtual int InterfaceOn();
	virtual int InterfaceOff();
	virtual void Clear(SomeData *d);
	virtual int Refresh();
	virtual int DrawData(Laxkit::anObject *ndata,Laxkit::anObject *a1=NULL,Laxkit::anObject *a2=NULL,int info=0);
	virtual int MouseMove(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int LBDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state, const Laxkit::LaxMouse *d);
	virtual int WheelUp  (int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count, const Laxkit::LaxMouse *d);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual int KeyUp(unsigned int ch,unsigned int state, const Laxkit::LaxKeyboard *d);
	virtual void ViewportResized();
	virtual int RemoveChild(); 
	virtual void FixCaret();

	virtual int scan(int x,int y,unsigned int state, double *alongpath, double *alongt, double *distto);
};

} // namespace LaxInterfaces

#endif

