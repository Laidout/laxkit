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
		tms tms_;
		start_clock = pause_clock = times(&tms_);
		start_time = pause_time = 0;

	} else {
		//need to add offset 
		tms tms_;
		clock_t current = times(&tms_);
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
	tms tms_;
	pause_clock = times(&tms_);
	pause_time = (pause_clock - start_clock) * secs_per_tick;
	return pause_time - start_time;
}

void StopWatch::Reset()
{
	playing = false;
	laps.flush();
	start_time = 0;
	pause_time = 0;
	start_clock = 0;
	pause_clock = 0;
}

void StopWatch::Lap()
{
	double current_time = CurTime();
	laps.push(current_time);
}

void StopWatch::SetTime(double time)
{
	tms tms_;
	pause_clock = times(&tms_);
	start_clock = pause_clock - time * ticks_per_sec;
	pause_time = time;

	while (laps.n > 0 && laps[laps.n-1] > pause_time)
		laps.pop();
}

/*! Return time in seconds since start_clock.
 */
double StopWatch::CurTime()
{
	tms tms_;
	clock_t clock_time = (playing ? times(&tms_) : pause_clock);
	double curtime = (clock_time - start_clock) * secs_per_tick;
	return curtime;
}


//--------------------- StopWatchWindow ---------------------------

class StopWatchWindow : public anXWindow, public StopWatch
{
  protected:
	char scratch[200];
	LaxFont *font, *time_font;

	enum ClickOn {
		Nothing,
		Laps,
		StartPause,
		LapReset
	};
	ClickOn mover;
	char *laptext;

  public:
	double fps;
	int timerid;

    StopWatchWindow();
    virtual ~StopWatchWindow();
    virtual void Refresh();
    virtual int  Idle(int tid, double delta);
    virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	virtual ClickOn scan(int x,int y);

	virtual void Start();
	virtual double Pause();
	virtual void Reset();
	virtual void Lap(); //add time to laps list
	virtual void SetTime(double time);
	virtual void UpdateLapText(int how);

	virtual char *getSelectionData(int *len,const char *property,const char *targettype,const char *selection);
};


StopWatchWindow::StopWatchWindow()
  : anXWindow(NULL,"win","win",ANXWIN_ESCAPABLE|ANXWIN_DOUBLEBUFFER|ANXWIN_CENTER, -1,-1,350,300,0, NULL,0,NULL)
{
    InstallColors(THEME_Panel);

	//double th = dp->textheight();
	//add display for time
	//laps list
	//play/pause button
	//lap/reset button
	
	font = win_themestyle->normal;
	font->inc_count();
	time_font = win_themestyle->monospace->duplicate();
	time_font->Resize(time_font->textheight()*2);

	fps = 30;
	mover = Nothing;
	laptext = nullptr;
}

StopWatchWindow::~StopWatchWindow()
{
	font->dec_count();
	time_font->dec_count();
	delete[] laptext;
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

StopWatchWindow::ClickOn StopWatchWindow::scan(int x,int y)
{
	double th  = font->textheight();
	double th2 = time_font->textheight();
	double oy;

	double L = laps.n * th;
	if (L + th2 + 2*th > win_h) {
		oy = win_h - 2*th - th2 - L;
	} else if (L < win_h/2 - th2/2) {
		oy = win_h/2 - th2/2 - L;
	} else {
		oy = 0;
	}

	if (y >= oy && y < oy + L) return Laps;
	if (y >= oy + L && y < oy + L + th2 + th) return StartPause;
	if (y >= oy + L + th2 + th && y < oy + L + th2 + 2*th) return LapReset;
	return Nothing;
}

int StopWatchWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	mover = scan(x,y);

	if (mover == StartPause || mover == Nothing) {
		if (playing) Pause();
		else Start();

	} else if (mover == LapReset) {
		if (playing) Lap();
		else Reset();

	} else if (mover == Laps) {
		UpdateLapText(0);
		if (laptext != nullptr) {
			app->CopytoBuffer(laptext,-1);
			selectionCopy(0);
			selectionCopy(1);
		}
	}

	needtodraw = 1;
	return 0;
} 

char *StopWatchWindow::getSelectionData(int *len,const char *property,const char *targettype,const char *selection)
{
	if (!laptext) return nullptr;

	char *str = nullptr;
	makestr(str, laptext);
	*len = strlen(str);
	return str;
}

/*! Under 60 seconds, return just seconds.
 * Under 60 minutes, return "m:ss.ss".
 * Else return "h:mm:ss.ss".
 */
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

void StopWatchWindow::UpdateLapText(int how)
{
	char scratch[100];
	delete[] laptext;
	laptext = nullptr;
	for (int c=0; c<laps.n; c++) {
		FormattedTime(laps[c], scratch);
		appendline(laptext, scratch);
	}
}

int StopWatchWindow::MouseMove(int x,int y,unsigned int state, const LaxMouse *m)
{
	ClickOn mo = scan(x,y);
	if (mo != mover) {
		mover = mo;
		needtodraw = 1;
	}
	return 0;
}


void StopWatchWindow::Refresh()
{
    if (!needtodraw) return;

	Displayer *dp = MakeCurrent();
    dp->ClearWindow();
	dp->font(font);



	double y = 0;
	double th = dp->textheight();
	double th2 = time_font->textheight();


	double L = laps.n * th;
	if (L + th2 + 2*th > win_h) {
		y = win_h - 2*th - th2 - L;
	} else if (L < win_h/2 - th2/2) {
		y = win_h/2 - th2/2 - L;
	} else {
		y = 0;
	}



	//------------ laps
	for (int c=0; c<laps.n; c++) {
		FormattedTime(laps[c], scratch);
		sprintf(scratch + strlen(scratch), " - ");
		FormattedTime(laps[c] - (c>0 ? laps[c-1] : 0), scratch + strlen(scratch));
		dp->textout(win_w/2, y, scratch, -1, LAX_HCENTER|LAX_TOP);
		y += th;
	}


	//----------- current time
	//draw hh:mm:ss.sssss
	double time = CurTime();
	FormattedTime(time, scratch); 

	if (mover == StartPause) {
		dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .9));
		dp->drawrectangle(0,y, win_w,th2+th, 1);
		dp->NewFG(win_themestyle->fg);
	}

	dp->font(time_font);
	dp->textout(win_w/2,y, scratch,-1, LAX_HCENTER|LAX_TOP);
	y += dp->textheight();
	dp->font(font);


	//-------- buttons
	dp->textout(win_w/2, y, playing ? _("Pause") : _("Start"), -1, LAX_HCENTER|LAX_TOP);
	y += th;

	if (mover == LapReset) {
		dp->NewFG(coloravg(win_themestyle->fg, win_themestyle->bg, .9));
		dp->drawrectangle(0,y, win_w,th, 1);
		dp->NewFG(win_themestyle->fg);
	}
	dp->textout(win_w/2, y, playing ? _("Lap") : _("Reset"), -1, LAX_HCENTER|LAX_TOP);
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

