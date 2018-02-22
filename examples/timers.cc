//
// Test out timers by animating a rectangle.
//
// After installing the Laxkit, compile this program like this:
//
// g++ timers.cc -I/usr/include/freetype2 -L/usr/X11R6/lib -lX11 -lXft -lm  -lpng \
//         -lcups -llaxkit -o laxhello


#include <lax/laxutils.h>
#include <lax/transformmath.h>

//for times():
#include <ctime>
//for sysconf(_SC_CLK_TCK):
#include <unistd.h>


#include <iostream>
using namespace std;
using namespace Laxkit;

class Win : public anXWindow
{
  public:
    double angle;       //radians
    double angle_step; //per timer tick
    double scale;
    double cur_time;
    double period; //seconds for scale pulse
    double step;  //in seconds
	double offsetx,offsety; //for checkerboard pattern
	bool paused;
	int frame;

    Win(double time_step_seconds);
    virtual void Refresh();
    virtual int  Idle(int tid=0);
    virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);

	void Update(int frames_to_advance);
};


Win::Win(double time_step_seconds)
    :anXWindow(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER, 0,0,300,300,0, NULL,0,NULL)
{
    scale      = (win_w<win_h?win_w:win_h) /4;
    angle      = 0;
    angle_step = 3 /180.*M_PI;

    step       = time_step_seconds;
    cur_time   = 0;
	frame      = 0;
    period     = 2;
	paused     = true;

	offsetx    = 100;
	offsety    = 0;

    installColors(app->color_panel);

	Update(1);
}

int  Win::Idle(int tid)
{
	if (paused) return 0;

    //cerr <<"tick "<<frame<< "  time: "<<cur_time<<endl;
	Update(1);

    return 0;
}

void Win::Update(int frames_to_advance)
{
	frame    += frames_to_advance;

    cur_time += frames_to_advance * step;
    angle    += frames_to_advance * angle_step;

    scale     = (win_w<win_h?win_w:win_h)/2 * (1+sin(cur_time/period*2*M_PI))/2;

	offsetx   = 100*cos(angle);
	offsety   = 100*sin(angle);

    needtodraw=1;
}

int Win::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
    if (ch==LAX_Left) {
		if (paused) {
			 //frame backwards
			Update(-1);

		} else {
			 //speed up spin
			angle_step*=.9;
		}
        return 0;

    } else if (ch==LAX_Right) {
		if (paused) {
			 //frame Forwards
			Update(1);

		} else {
			 //slow down spin
			angle_step*=1.1;
		}
        return 0;

    } else if (ch==' ') {
		paused = !paused;
		return 0;
    }

    return anXWindow::CharInput(ch,buffer,len,state,kb); //still need to return for default ESC behavior
}

void Win::Refresh()
{
    if (!needtodraw) return;

	Displayer *dp = MakeCurrent();
    dp->ClearWindow();

	 //checkerboard
    dp->NewFG(coloravg(win_colors->fg, win_colors->bg, .4));
    dp->NewBG(coloravg(win_colors->fg, win_colors->bg, .6));
	dp->drawCheckerboard(0,0,win_w,win_h, 20, offsetx,offsety);

	 //spinning square
    Affine m;
    m.setRotation(angle);
    m.setScale(scale,scale);
    m.origin(flatpoint(win_w/2,win_h/2));

    flatpoint pts[4] = {{-1., 1., 0},
                        { 1., 1., 0},
                        { 1.,-1., 0},
                        {-1.,-1., 0}};
    
    for (int c=0; c<4; c++) pts[c]=m.transformPoint(pts[c]);


    dp->NewFG(win_colors->fg);
    dp->drawlines(pts, 4, 1, 1);


	//make a red ball cross the window diagonally
	long clock_ticks_per_second = sysconf(_SC_CLK_TCK);
	long duration = (1. /*seconds*/) * clock_ticks_per_second; //seconds for traversal

	time_t time = times(NULL);
	double pos = (time % duration) / (float)duration;
	dp->NewFG(1.,0.,0.);
	dp->drawpoint(win_w*pos, win_h*pos, 10, 1);



    SwapBuffers();
    needtodraw=0;
}

int main(int argc,char **argv)
{
    anXApp app;
    app.init(argc,argv);

    double fps=30;
    Win win(1/fps);
    app.addwindow(&win,1,0);
    app.addtimer(&win, 1/fps*1000, 1/fps*1000, -1);

    cerr <<"------Done adding initial windows in main() -------\n";
    app.run();

    cerr <<"------ App Close:  -------\n";
    app.close();
    
    cerr <<"------ Bye! -------\n";
    return 0;
}

