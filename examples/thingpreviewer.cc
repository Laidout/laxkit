
//
// Preview all the built in "thing" objects.
//
// After installing the Laxkit, compile this program like this:
//
// g++ timers.cc -I/usr/include/freetype2 -L/usr/X11R6/lib -lX11 -lXft -lm  -lpng \
//         -lcups -llaxkit -o laxhello


#include <lax/laxutils.h>
#include <lax/transformmath.h>

#include <iostream>
using namespace std;
using namespace Laxkit;



const char *thing_name(int thing)
{
    if (thing==THING_None) return "THING_None";
    if (thing==THING_Circle) return "THING_Circle";
    if (thing==THING_Circle_X) return "THING_Circle_X";
    if (thing==THING_Circle_Plus) return "THING_Circle_Plus";
    if (thing==THING_Square) return "THING_Square";
    if (thing==THING_Diamond) return "THING_Diamond";
    if (thing==THING_Triangle_Up) return "THING_Triangle_Up";
    if (thing==THING_Triangle_Down) return "THING_Triangle_Down";
    if (thing==THING_Triangle_Left) return "THING_Triangle_Left";
    if (thing==THING_Triangle_Right) return "THING_Triangle_Right";
    if (thing==THING_To_Left) return "THING_To_Left";
    if (thing==THING_To_Right) return "THING_To_Right";
    if (thing==THING_To_Top) return "THING_To_Top";
    if (thing==THING_To_Bottom) return "THING_To_Bottom";
    if (thing==THING_Plus) return "THING_Plus";
    if (thing==THING_X) return "THING_X";
    if (thing==THING_Asterix) return "THING_Asterix";
    if (thing==THING_Pause) return "THING_Pause";
    if (thing==THING_Eject) return "THING_Eject";
    if (thing==THING_Double_Triangle_Up) return "THING_Double_Triangle_Up";
    if (thing==THING_Double_Triangle_Down) return "THING_Double_Triangle_Down";
    if (thing==THING_Double_Triangle_Left) return "THING_Double_Triangle_Left";
    if (thing==THING_Double_Triangle_Right) return "THING_Double_Triangle_Right";
    if (thing==THING_Arrow_Left) return "THING_Arrow_Left";
    if (thing==THING_Arrow_Right) return "THING_Arrow_Right";
    if (thing==THING_Arrow_Up) return "THING_Arrow_Up";
    if (thing==THING_Arrow_Down) return "THING_Arrow_Down";
    if (thing==THING_Double_Arrow_Horizontal) return "THING_Double_Arrow_Horizontal";
    if (thing==THING_Double_Arrow_Vertical) return "THING_Double_Arrow_Vertical";
    if (thing==THING_Pan_Arrows) return "THING_Pan_Arrows";
    if (thing==THING_Check) return "THING_Check";
    if (thing==THING_Locked) return "THING_Locked";
    if (thing==THING_Unlocked) return "THING_Unlocked";
    if (thing==THING_Open_Eye) return "THING_Open_Eye";
    if (thing==THING_Closed_Eye) return "THING_Closed_Eye";
    if (thing==THING_Octagon) return "THING_Octagon";
    if (thing==THING_Magnifying_Glass) return "THING_Magnifying_Glass";
    if (thing==THING_Wrench) return "THING_Wrench";
    if (thing==THING_Cancel) return "THING_Cancel";
    if (thing==THING_Star) return "THING_Star";
    if (thing==THING_Star) return "THING_Gear";

	return "(unknown name)";
}


class Win : public anXWindow
{
  public:
	DrawThingTypes current_thing;

    Win();
    virtual void Refresh();
    virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
};

Win::Win()
    :anXWindow(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER, 0,0,500,300,0, NULL,0,NULL)
{
	current_thing=THING_None;

    InstallColors(THEME_Panel);
}

int Win::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
    if (ch==LAX_Right) {
		int i=((int)current_thing+1);
		if (i>=(int)THING_MAX) i=THING_None;
        current_thing=(DrawThingTypes)i;
		needtodraw=1;
        return 0;

    } else if (ch==LAX_Left) {
		int i=((int)current_thing-1);
		if (i<(int)THING_None) i=(int)THING_MAX-1;
        current_thing=(DrawThingTypes)i;
		needtodraw=1;
        return 0;
    }

    return anXWindow::CharInput(ch,buffer,len,state,kb); //still need to return for default ESC behavior
}

void Win::Refresh()
{
    if (!needtodraw) return;

	Displayer *dp = MakeCurrent();
    dp->ClearWindow();


    dp->NewFG(win_themestyle->fg);
    dp->NewBG(coloravg(win_themestyle->bg, win_themestyle->fg, .1));

	int pad=win_h*.1;
	int w=(win_w-4*pad)/3;
	int banner = 2*dp->textheight();
	int h=win_h-banner-2*pad;
	if (h<w) w=h;

	 //draw in grid with different fill types
	dp->textout(1*win_w/6,win_h, "Fill==0",-1, LAX_BOTTOM|LAX_HCENTER);
	dp->drawthing(1*win_w/6,pad+banner+h/2,     w/2,-(w/2), 0, current_thing);

	dp->textout(3*win_w/6,win_h,"Fill==1",-1, LAX_BOTTOM|LAX_HCENTER);
	dp->drawthing(3*win_w/6,pad+banner+h/2,     w/2,-(w/2), 1, current_thing);

	dp->textout(5*win_w/6,win_h, "Fill==2",-1, LAX_BOTTOM|LAX_HCENTER);
	dp->drawthing(5*win_w/6,pad+banner+h/2,     w/2,-(w/2), 2, current_thing);



	//draw name
	char scratch[300];
	sprintf(scratch, "%s (%d/%d)", thing_name(current_thing), current_thing - THING_None, THING_MAX-THING_None-1);
	dp->textout(win_w/2,pad+banner/2, scratch,-1, LAX_CENTER);


    SwapBuffers();
    needtodraw=0;
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

