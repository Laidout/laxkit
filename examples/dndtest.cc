
//
// Preview all the built in "thing" objects.
//
// After installing the Laxkit, compile this program like this:
//
// g++ timers.cc -I/usr/include/freetype2 -L/usr/X11R6/lib -lX11 -lXft -lm  -lpng \
//         -lcups -llaxkit -o laxhello


#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/utf8string.h>
#include <lax/messagebar.h>

#include <iostream>
using namespace std;
using namespace Laxkit;

#define DBG


class Win : public MessageBar
{
  public:

    Win();
    //virtual void Refresh();
    //virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);

	bool DndWillAcceptDrop(int x, int y, const char *action, IntRectangle &rect, char **types, int *type_ret, anXWindow **child_ret);
	int selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which);

	virtual void DndHoverStart() { SetText("Dnd start"); }
	virtual void DndHoverMove(flatpoint p, char *action) { SetText("Dnd move"); }
	virtual void DndHoverCanceled() { SetText("Dnd canceled"); }
	virtual void DndHoverSuccess() { cout << "Dnd success!" << endl; }
};

Win::Win()
    :MessageBar(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER|ANXWIN_XDND_AWARE, 0,0,500,300,0, nullptr)
    //:MessageBar(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER|ANXWIN_XDND_AWARE, 0,0,500,300,0, NULL,0,NULL)
{

    InstallColors(THEME_Panel);
}

int Win::selectionDropped(const unsigned char *data,unsigned long len,const char *actual_type,const char *which)
{
	DBG cerr << "NodeInterface::selectionDropped, which: "<<(which ? which : "null")<<", actual_type: "<<(actual_type ? actual_type : "null")<<endl;

	Utf8String str;

	if (!strcmp(actual_type, "text/uri-list")) {
		cout << "Dropped:"<<endl;

		int n = 0;
		char **files = splitonnewline((const char *)data, &n);
		flatpoint pos;
		double th = 0;
		const char *ext;

		if (n) {
			for (int c=0; c<n; c++) {
				const char *fname = files[c];
				cout << "  " << fname <<endl;
				str.Append(fname);
				str.Append("\n");
			}
		}
		deletestrs(files, n);
		SetText(str.c_str());
		return 0;
	}

	if ( !strcmp(actual_type, "text/plain;charset=UTF-8") || !strcmp(actual_type, "UTF8_STRING")
	  || !strcmp(actual_type, "text/plain") || !strcmp(actual_type, "TEXT")) {

		Utf8String str((const char *)data,len);
		cout << "Dropped text: "<<str.c_str()<<endl;
		SetText(str.c_str());
		return 0;
	}

	return 1;
}

bool Win::DndWillAcceptDrop(int x, int y, const char *action, IntRectangle &rect, char **types, int *type_ret, anXWindow **child_ret)
{
	// hopefully file list...
	for (int c=0; types[c]; c++) {
		DBG cerr << "..compare dnd type "<<c<<": "<<types[c]<<endl;
		if (!strcmp(types[c], "text/uri-list")) {
			DBG cerr << "NodeInterface::DndWillAcceptDrop() "<<c<<": "<<types[c]<<endl;
			*type_ret = c;
			return true;
		}
	}

	 // ok for text to StringValue:
    for (int c=0; types[c]; c++) {
        if (!strcmp(types[c], "text/plain;charset=UTF-8") || !strcmp(types[c], "UTF8_STRING")) {
            *type_ret = c;
            return true;
        }
    }
    for (int c=0; types[c]; c++) {
        if (!strcmp(types[c], "text/plain") || !strcmp(types[c], "TEXT")) {
            *type_ret = c;
            return true;
        }
    }

	return false;
}


int main(int argc,char **argv)
{
    anXApp app;
    app.init(argc,argv);

    Win win;
    app.addwindow(&win,1,0);

    cerr <<"------Done adding initial windows in main() -------\n";
    app.run();

    cerr <<"------ App Close:  -------\n";
    app.close();
    
    cerr <<"------ Bye! -------\n";
    return 0;
}

