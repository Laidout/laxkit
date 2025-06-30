//
// A basic analog clock.
//
// todo:
// - tick vs smooth
// - drag to set
// - am/pm indication
// - optional hour labels?
// - timezones?
// - click to copy timestamp
//


#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/laxdefs.h>
#include <lax/drawingdefs.h>
#include <lax/dateutils.h>

//for times():
//#include <ctime>
//for sysconf(_SC_CLK_TCK):
//#include <unistd.h>

#include <iostream>
using namespace std;
using namespace Laxkit;

class ClockWindow : public anXWindow
{
  public:
    //double update_interval_seconds = .1;
    //double margin = 0.0;

    ClockWindow();
    virtual void Refresh();
    virtual int  Idle(int tid, double delta);
    virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);

	void Update(int frames_to_advance);

};


ClockWindow::ClockWindow()
    :anXWindow(nullptr,"Clock","Clock",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER, 400,200,300,300,0, nullptr,0,nullptr)
{
    InstallColors(THEME_Panel);
}

int  ClockWindow::Idle(int tid, double delta)
{
	Update(1);
    return 0;
}

void ClockWindow::Update(int frames_to_advance)
{
    needtodraw=1;
}

int ClockWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
//    if (ch==LAX_Left) {
//		if (paused) {
//			 //frame backwards
//			Update(-1);
//
//		} else {
//			 //speed up spin
//			angle_step*=.9;
//		}
//        return 0;
//
//    } else if (ch==LAX_Right) {
//		if (paused) {
//			 //frame Forwards
//			Update(1);
//
//		} else {
//			 //slow down spin
//			angle_step*=1.1;
//		}
//        return 0;
//
//    } else if (ch==' ') {
//		paused = !paused;
//		cout << (paused ? "paused" : "play") <<endl;
//		return 0;
//
//    } else if (ch=='m') {
//		Resize(700, 100);
//    }

    return anXWindow::CharInput(ch,buffer,len,state,kb); //still need to return for default ESC behavior
}

void ClockWindow::Refresh()
{
    if (!needtodraw) return;

	Displayer *dp = MakeCurrent();
    dp->ClearWindow();

    dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .4));
    dp->NewBG(coloravg(win_themestyle->fg, win_themestyle->bg, .6));
	
	LaxTime time;
	time.SetToNow();
	flatpoint center(win_w/2, win_h/2);
	double radius = MIN(win_w/2, win_h/2);
	dp->LineCap(LAXCAP_Round);

	// ticks
	double angle;
	dp->LineWidth(2);
    dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .7));
	for (int c=0; c<12; c++) {
		angle = M_PI/2 + c/12.0 * 2*M_PI;
		flatpoint v = flatpoint(-cos(angle), -sin(angle));
		dp->drawline(center + .95 * radius * v, center + .7 * radius * v);
	}

	// Hours
	dp->NewFG(1.,0.,0.);
	angle = M_PI/2 + fmod(time.Hour(), 12.0)/12.0 * 2*M_PI;
	dp->LineWidth(10);
	dp->drawline(center, center + .3*radius*flatpoint(-cos(angle), -sin(angle)));

	// Minutes
	dp->NewFG(0.,1.,0.);
	angle = M_PI/2 + time.Minute()/60.0 * 2*M_PI;
	dp->LineWidth(5);
	dp->drawline(center, center + .9*radius*flatpoint(-cos(angle), -sin(angle)));
	
	// Seconds
	dp->NewFG(0.,0.,1.);
	angle = M_PI/2 + time.Second()/60.0 * 2*M_PI;
	dp->LineWidth(1);
	dp->drawline(center, center + .9*radius*flatpoint(-cos(angle), -sin(angle)));
	


    SwapBuffers();
    needtodraw = 0;
}



int main(int argc,char **argv)
{
    anXApp app;
    app.init(argc,argv);

    double fps = 20;
    ClockWindow *win = new ClockWindow();
    app.addwindow(win,1,1);
    app.addtimer(win, 1/fps*1000, 1/fps*1000, -1);

    cerr <<"------Done adding initial windows in main() -------\n";
    app.run();

    cerr <<"------ App Close:  -------\n";
    app.close();
    
    cerr <<"------ Bye! -------\n";
    return 0;
}


