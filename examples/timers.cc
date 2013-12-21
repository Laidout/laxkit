//
// Test out timers by animating a rectangle.
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

class Win : public anXWindow
{
  public:
    double angle; //radians
    double angle_step;
    double scale;
    double cur_time;
    double period; //seconds for scale pulse
    double step; //in seconds

    Win(double time_step_seconds);
    virtual void Refresh();
    virtual int  Idle(int tid=0);
    virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
};


Win::Win(double time_step_seconds)
    :anXWindow(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER, 0,0,300,300,0, NULL,0,NULL)
{
    scale=(win_w<win_h?win_w:win_h) /4;
    angle=0;
    angle_step= 3 /180.*M_PI;

    step=time_step_seconds;
    cur_time=0;
    period=2;

    installColors(app->color_panel);
}

int  Win::Idle(int tid)
{
    static int frame=0;

    //cerr <<"tick "<<frame<< "  time: "<<cur_time<<endl;
    frame++;

    angle+=angle_step;

    cur_time+=step;
    scale=(win_w<win_h?win_w:win_h)/2 * (1+sin(cur_time/period*2*M_PI))/2;

    needtodraw=1;

    return 0;
}

int Win::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
    if (ch==LAX_Left) {
        angle_step*=.9;
        return 0;
    } else if (ch==LAX_Right) {
        angle_step*=1.1;
        return 0;
    }

    return anXWindow::CharInput(ch,buffer,len,state,kb); //still need to return for default ESC behavior
}

void Win::Refresh()
{
	if (!needtodraw) return;

    clear_window(this);


    Affine m;
    m.setRotation(angle);
    m.setScale(scale,scale);
    m.origin(flatpoint(win_w/2,win_h/2));

    flatpoint pts[4] = {{-1., 1., 0},
                        { 1., 1., 0},
                        { 1.,-1., 0},
                        {-1.,-1., 0}};
    
    for (int c=0; c<4; c++) pts[c]=m.transformPoint(pts[c]);

    foreground_color(win_colors->fg);
    fill_polygon(this, pts, 4);

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

