//
// A simple stopwatch.
//
// After installing the Laxkit, compile this program like this:
//
// g++ stopwatch.cc `pkg-config laxkit --libs`  `pkg-config laxkit --libs` -o stopwatch


#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/language.h>

//template implementation:
#include <lax/lists.cc>


//#include <cmath>

//for times():
#include <ctime>
//for sysconf(_SC_CLK_TCK):
#include <unistd.h>

#include <iostream>
using namespace std;
using namespace Laxkit;


//
//
// ( time hh:mm:ss.sss )
//
// Start
// Reset
//

//
// ( time hh:mm:ss.sss )
//
// Pause
// Lap
//

// right click to save time list to file / copy to system clipboard:
// (date at start of run)
// time 1
// time 2
// ...


static double secs_per_tick = 1.0 / sysconf(_SC_CLK_TCK);
static double ticks_per_sec = sysconf(_SC_CLK_TCK) / 1.0;


//--------------------- StopWatch ---------------------------

/*! A stopwatch to time things on the order of hours.
 */
class StopWatch
{
  public:
	Laxkit::NumStack<double> laps;

	clock_t start_clock;
	clock_t pause_clock;

	double start_time;
	double pause_time;

	bool playing;

    StopWatch();

	virtual void Start();
	virtual double Pause();
	virtual void Reset();
	virtual void Lap(); //add time to laps list
	virtual void SetTime(double time);

	double CurTime();
};

StopWatch::StopWatch()
{
	start_clock = 0;
	pause_clock = 0;

	start_time = 0;
	pause_time = 0;

	playing = false;
}

void StopWatch::Start()
{
	if (playing) return;
	playing = true;
	if (start_clock == 0) {
		start_clock = pause_clock = times(NULL);
		start_time = pause_time = 0;

	} else {
		//need to add offset 
		clock_t current = times(NULL);
		clock_t diff = current - pause_clock;
		start_clock += diff;
		pause_clock = current;
		//double tdiff = diff * secs_per_tick;
		//for (int c=0; c<laps.n; c++) laps[c] += tdiff;

		pause_time = (pause_clock - start_clock) * secs_per_tick;
	}
}

/*! Pause timer. Returns the CurTime().
 */
double StopWatch::Pause()
{
	if (!playing) return pause_time;

	playing = false;
	pause_clock = times(NULL);
	pause_time = (pause_clock - start_clock) * secs_per_tick;
	return pause_time - start_time;
}

void StopWatch::Reset()
{
	playing = false;
	laps.flush();
	start_time = 0;
	pause_time = 0;
}

void StopWatch::Lap()
{
	double current_time = CurTime();
	laps.push(current_time);
}

 //add time to laps list
void StopWatch::SetTime(double time)
{
//	if (playing) {
//		pause_time = time;
//		pause_clock = times(NULL);
//	}

	pause_clock = times(NULL);
	start_clock = pause_clock - time * ticks_per_sec;
	pause_time = time;

	while (laps.n > 0 && laps[laps.n-1] > pause_time)
		laps.pop();
}

/*! Return time in seconds since start_clock.
 */
double StopWatch::CurTime()
{
	clock_t clock_time = (playing ? times(NULL) : pause_clock);
	double curtime = (clock_time - start_clock) * secs_per_tick;
	return curtime;
}


//--------------------- StopWatchWindow ---------------------------

class StopWatchWindow : public anXWindow, public StopWatch
{
	char scratch[200];

  public:
	double fps;
	int timerid;

    StopWatchWindow();
    virtual void Refresh();
    virtual int  Idle(int tid, double delta);
    virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);

	virtual void Start();
	virtual double Pause();
	virtual void Reset();
	virtual void Lap(); //add time to laps list
	virtual void SetTime(double time);
};


StopWatchWindow::StopWatchWindow()
  : anXWindow(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER, 400,200,350,300,0, NULL,0,NULL)
{
    InstallColors(THEME_Panel);

	//double th = dp->textheight();
	//add display for time
	//laps list
	//play/pause button
	//lap/reset button
	
	fps = 30;
}

int  StopWatchWindow::Idle(int tid, double delta)
{
	if (!playing) return 0;

	needtodraw=1;
    return 0;
}

int StopWatchWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
    if (ch == ' ' && state == 0) {
		if (playing) Pause();
		else Start();

		cout << (playing ? "playing" : "paused") <<endl;
		return 0;

    } else if (ch == LAX_Bksp || ch == LAX_Del) {
		Reset();
		return 0;


	} else if (ch == LAX_Up) {
		SetTime(CurTime() + 10);
	
    } else if (ch != LAX_Esc && (state&LAX_STATE_MASK) == 0) {
		if (playing) Lap();
		return 0;
	}

    return anXWindow::CharInput(ch,buffer,len,state,kb); //still need to return for default ESC behavior
}

int StopWatchWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	if (playing) Pause();
	else Start();
}

void FormattedTime(double time, char *buffer)
{
	if (time < 60) sprintf(buffer, "%.02f", time);
	else if (time < 60 * 60)
		sprintf(buffer, "%d:%02d.%02d", int(time/60), int(fmod(time, 60)), int(fmod(time, 1)*100));
	else {
		//sprintf(buffer, "%d:%02d:%.02f", int(time/60/60), int(fmod(time, 60*60)*60), fmod(time, 60));

		int h = time / 60 / 60;
		time = fmod(time, 60*60);
		sprintf(buffer, "%d:%02d:%02d.%02d", h, int(time/60), int(fmod(time, 60)), int(fmod(time, 1)*100));
	}
}

void StopWatchWindow::Refresh()
{
    if (!needtodraw) return;

	Displayer *dp = MakeCurrent();
    dp->ClearWindow();

	double th = dp->textheight();
	double y = th/2;


	//------------ laps
	for (int c=0; c<laps.n; c++) {
		FormattedTime(laps[c], scratch);
		sprintf(scratch + strlen(scratch), " - ");
		FormattedTime(laps[c] - (c>0 ? laps[c-1] : 0), scratch + strlen(scratch));
		dp->textout(win_w/2, y, scratch, -1, LAX_CENTER);
		y += th;
	}


	//----------- current time
	//draw hh:mm:ss.sssss
	//clock_t curtime = times(NULL);
	double time = CurTime();
	FormattedTime(time, scratch); 

	dp->textout(win_w/2,y, scratch,-1);
	y += th;


	//-------- buttons
	dp->textout(win_w/2, y, playing ? _("Pause") : _("Start"), -1, LAX_CENTER);
	y += th;

	dp->textout(win_w/2, y, playing ? _("Lap") : _("Reset"), -1, LAX_CENTER);
	y += th;

    SwapBuffers();
    needtodraw=0;
}

void StopWatchWindow::Start()
{
	if (playing) return;
	StopWatch::Start();
    app->addtimer(this, 1/fps*1000, 1/fps*1000, -1);
	needtodraw = 1;
}

double StopWatchWindow::Pause()
{
	if (!playing) return pause_time;
	StopWatch::Pause();
	app->removetimer(this, timerid);
	timerid = 0;
	needtodraw = 1;
	return pause_time;
}

void StopWatchWindow::Reset()
{
	StopWatch::Reset();
	if (timerid != 0) app->removetimer(this, timerid);
	needtodraw = 1;
}

void StopWatchWindow::Lap()
{
	StopWatch::Lap();
	needtodraw = 1;
}

void StopWatchWindow::SetTime(double time)
{
	StopWatch::SetTime(time);
	needtodraw = 1;
}




//------------------------------------- Main -----------------------------------------

int main(int argc,char **argv)
{
	cerr << "secs_per_tick: " <<secs_per_tick<<endl;
	cerr << "ticks_per_sec: " <<ticks_per_sec<<endl;

    anXApp app;
    app.init(argc,argv);

    StopWatchWindow win;
    app.addwindow(&win,1,0);

    cerr <<"------Done adding initial windows in main() -------\n";
    app.run();

    cerr <<"------ App Close:  -------\n";
    app.close();
    
    cerr <<"------ Bye! -------\n";
    return 0;
}

