//
//    $Id$
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
//    Copyright (C) 2011 by Tom Lechner
//

extern "C" {
	#define _ENABLE_TILT
	#define _ENABLE_FORCE

	//#include <wiimote.h>
	//#include <wiimote_api.h>
#include <libcwiimote/wiimote.h>
#include <libcwiimote/wiimote_api.h>
}

//#include <libcwiimote/wiimote.h>
//#include <libcwiimote/wiimote_api.h>

#include <cstdlib>
#include <sys/times.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>


#include <iostream>
using namespace std;
#define DBG

/*! Wiimotes have:  1  2  A  B  Home  +  -  l/r/u/d
 *   accelerometr data
 *   Also: Rumble, 4 leds, battery status
 *     ir: camera is 1024x768, 4 spots with x,y,on,size
 *  
 *  You can set the leds, rumble, and play something on its speaker!!
 *
 *  Wiimotes can provide key and mouse data in a variety of ways.
 * 
 *  1. arrow pad can produce mouse movement, other buttons keys
 *  2. arrows and buttons all produce key events
 *  3. each of 4 IR spots can describe a mouse (x,y,size), a la wii whiteboard
 *  4. the IR input can define a movement area, translated into mouse
 *  5. tilt can produce y movement, x movement hacked from rotating
 *
 *
 */



static struct { int mask; const char* name; int keysym; int laxkey } wiimote_buttons[] = {
	{WIIMOTE_KEY_1,     "1",      XK_1,      '1'},
	{WIIMOTE_KEY_2,     "2",      XK_2,      '2'},
	{WIIMOTE_KEY_A,     "A",      XK_A,      'A'},
	{WIIMOTE_KEY_B,     "B",      XK_B,      'B'},
	{WIIMOTE_KEY_PLUS,  "+",      XK_plus,   '+'},
	{WIIMOTE_KEY_MINUS, "-",      XK_minus,  '-'},
	{WIIMOTE_KEY_HOME,  "Home",   XK_Home,   LAX_Home},    // ⌂
	{WIIMOTE_KEY_LEFT,  "Left",   XK_Left,   LAX_Left},    // ◀
	{WIIMOTE_KEY_RIGHT, "Right",  XK_Right,  LAX_Right},   // ▶
	{WIIMOTE_KEY_UP,    "Up",     XK_Up,     LAX_Up},      // ▲
	{WIIMOTE_KEY_DOWN,  "Down",   XK_Down,   LAX_Down},    // ▼
	{0, "", 0, 0}
};

class WiiButtonMap
{
  public:
	int wiibutton;
	int towhat; //0 map to none, 1 map to key, 2 map to mouse nudge
	int key, info;   //key value+modifier, or mouse direction+amount
	WiiButtonMap *next;
	WiiButtonMap(int wiib, int what, int k, int k2);
	WiiButtonMap(int wiib, int what, int k, int k2);
	~WiiButtonMap() { if (next) delete next; }
};

WiiButtonMap::WiiButtonMap(int wiib, int what, int k, int k2)
  : wiibutton(wiib), towhat(what), key(k), info(k2), next(NULL)
{}

//! Return a wii button map where all the buttons map to the expected keys (capital A and B).
WiiButtonMap *AllNormalKeys()
{
	WiiButtonMap *m, *me=NULL, *ms=NULL;
	for(i=0 ; wiimote_buttons[i].mask; i++) {
		m=new WiiButtonMap(wiimote_buttons[i].mask, 1, wiimote_buttons[i].laxkey);
		if (ms) ms=m;
		if (!me) me=e;
		else { me->next=m; me=me->next; }
	}
	return ms;
}

//--------------------------------------- LaxWiimote ------------------------------------
class ButtonQueue
{
  public:
	int button;
	int value;
	ButtonQueue *next;
	ButtonQueue(int b,int v) { button=b; value=v; next=NULL; }
};

/*! \class LaxWiimote
 * \brief Class to use Wiimotes from lax based programs.
 */
class LaxWiimote
{
  protected:
	wiimote_t* m_p_wiimote;
	wiimote_t* m_p_wiimote_old;
	int number;
	ButtonQueue *events, *ee;
	void NewButtonEvent(int keysym, int value);
	double ir_transform[16];

  public:
	int poll_time_microseconds;

	LaxWiimote();
	virtual ~LaxWiimote();

	virtual int IsOpen();
	virtual int Connect();
	virtual int Update();
	virtual int Rumble(int on);
	virtual int Leds(int mask);
	virtual void Fps(int fps);
	virtual int NextEvent(int *button, int *value);

	void sleep();
};

LaxWiimote::LaxWiimote()
{
	 // Initialize wiimote structs, set to 0 (see WIIMOTE_INIT)
	m_p_wiimote = (wiimote_t*)calloc(sizeof(wiimote_t), 1);
	m_p_wiimote_old = (wiimote_t*)calloc(sizeof(wiimote_t), 1);
	number=1;

	poll_time_microseconds=5000;
	events=ee=NULL;
}

LaxWiimote::~LaxWiimote()
{
	if (wiimote_is_open(m_p_wiimote)) {
		wiimote_close(m_p_wiimote);
	}
}

int LaxWiimote::IsOpen()
{
	return wiimote_is_open(m_p_wiimote);
}

void LaxWiimote::NewButtonEvent(int keysym, int value)
{
	ButtonQueue *e=new ButtonQueue(button,value);
	if (!events) events=ee=e;
	else {
		ee->next=e;
		ee=e;
	}
}

//! Return 1 if there was a next event, else 0.
int LaxWiimote::NextEvent(int *button, int *value)
{
	if (!events) return 0;
	*button=events->button;
	*value=events->value;
	ButtonQueue *e=events;
	if (events->next==NULL) ee=NULL;
	events=events->next;
	delete e;
}

//! Poll the wiimote this many times per second.
void LaxWiimote::Fps(int fps)
{
	poll_time_microseconds=1000000/fps;
}

void LaxWiimote::sleep()
{
	usleep(poll_time_microseconds);
}

//! Turn on or off the rumble, or set to turn off after timer microseconds.
/*! Return 0 for success or 1 for error.
 */
int LaxWiimote::Rumble(int on)
{
	DBG cerr <<"rumble "<<on<<endl;
	m_p_wiimote->rumble = on?1:0;
	if ((!wiimote_is_open(m_p_wiimote)) || (wiimote_update(m_p_wiimote) == WIIMOTE_ERROR)) return 1;
	return 0;
}

//! Set the leds, with bits 1-4.
/*! Return 0 for success or 1 for error.
 */
int LaxWiimote::Leds(int mask)
{
	if ((mask&15)==(m_p_wiimote->led.bits&15)) return 0;
	m_p_wiimote->led.bits = (m_p_wiimote->led.bits&0xf0)|(mask&15);
	if ((!wiimote_is_open(m_p_wiimote)) || (wiimote_update(m_p_wiimote) == WIIMOTE_ERROR)) return 1;
	return 0;
}

//! Discover and connect to a new Wiimote. 
int LaxWiimote::Connect()
{
	int tries = 2;
	while(tries-- > 0) {
		if(wiimote_discover(m_p_wiimote, (uint8_t)1) > 0) {
			if(wiimote_connect(m_p_wiimote, m_p_wiimote->link.r_addr) >= 0) {

				// Enable features
				m_p_wiimote->mode.acc = 1;
				m_p_wiimote->mode.ir = 1; // HACK: enabling ir, even when we don't use it, seems to
										  // force certain shady wiimotes to send updates when otherwise
										  // it took firm shakes to see any data from them (confirmed in wmgui Jan 9 2010)

				m_p_wiimote->led.bits = 1 << (number-1);
				m_p_wiimote->rumble = 0;

				if((!wiimote_is_open(m_p_wiimote)) || (wiimote_update(m_p_wiimote) == WIIMOTE_ERROR)) {
					wiimote_close(m_p_wiimote);
					return false;
				}
				return true;

			} else {
				// Connection error
				cerr << "Could not connect to Wiimote!"<<endl;
				return false;
			}
		}
	}
	return false;
}

typedef struct
{
	int min;
	int max;
} TLimits;

float clamp_float(float value, float min, float max)
{
	return ((value >= max) ? max : ((value <= min) ? min : value));
}

float scale_and_expand_limits(int value, TLimits* limits)
{
	if(value < limits->min)
		limits->min = value;

	if(value > limits->max)
		limits->max = value;

	int range = (limits->max - limits->min);

	if(range == 0)
		return 0.0;

	return (float)(value - limits->min) / (float)range;
}

//! Return 0 on some kind of error, or 1 for success.
int LaxWiimote::Update()
{
	if((!wiimote_is_open(m_p_wiimote)) || (wiimote_update(m_p_wiimote) == WIIMOTE_ERROR)) {
		wiimote_close(m_p_wiimote);
		return false;
	}

	//---Update Buttons
	int i;
	for(i=0 ; wiimote_buttons[i].mask; i++) {
		if((m_p_wiimote->keys.bits & wiimote_buttons[i].mask) != (m_p_wiimote_old->keys.bits & wiimote_buttons[i].mask)) {
			DBG cerr <<"button:"<<wiimote_buttons[i].name<<" is "<<((m_p_wiimote->keys.bits & wiimote_buttons[i].mask) > 0 ? "on" : "off")<<endl;
			//send_integer(wiimote_buttons[i].name, (m_p_wiimote->keys.bits & wiimote_buttons[i].mask) > 0 ? 1 : 0);
			NewButtonEvent(wiimote_buttons[i].keysym, ((m_p_wiimote->keys.bits & wiimote_buttons[i].mask) > 0 ? 1 : 0));
		}
	}


	//---Update Force
	static TLimits force_x_limits = {10000,-10000};
	float force_x = scale_and_expand_limits(m_p_wiimote->axis.x, &force_x_limits);
	//send_float("Force / X", force_x);
	//DBG cerr<<"force x:"<<force_x<<endl;

	static TLimits force_y_limits = {10000,-10000};
	float force_y = scale_and_expand_limits(m_p_wiimote->axis.y, &force_y_limits);
	//send_float("Force / Z", force_y);		// NOTE: flipped y/z for Luz coordinate system
	//DBG cerr<<"force y:"<<force_y<<endl;

	static TLimits force_z_limits = {10000,-10000};
	float force_z = scale_and_expand_limits(m_p_wiimote->axis.z, &force_z_limits);
	//send_float("Force / Y", force_z);		// NOTE: flipped y/z for Luz coordinate system
	//DBG cerr<<"force z:"<<force_z<<endl;


	//---Update Pitch
	// Clamp to -1.0..1.0 g-force, because any more isn't a function of tilt but rather moving the Wiimote.
	float pitch = clamp_float(((float)(m_p_wiimote->axis.y - m_p_wiimote->cal.y_zero) / (float)(m_p_wiimote->cal.y_scale - m_p_wiimote->cal.y_zero)), -1.0, 1.0) ;
	pitch *= -1.0;		// We want positive values when Wiimote is vertical
	pitch += 1.0;			// -1.0..1.0 to 0.0..2.0
	pitch /= 2.0;			//  0.0..2.0 to 0.0..1.0
	//send_float("Pitch", pitch);
	//DBG cerr<<"pitch:"<<pitch<<endl;


	//---Update Roll
	// Clamp to -1.0..1.0 g-force, because any more isn't a function of tilt but rather moving the Wiimote.
	float roll = clamp_float(((float)(m_p_wiimote->axis.x - m_p_wiimote->cal.x_zero) / (float)(m_p_wiimote->cal.x_scale - m_p_wiimote->cal.x_zero)), -1.0, 1.0) ;
	roll += 1.0;		// -1.0..1.0 to 0.0..2.0
	roll /= 2.0;		//  0.0..2.0 to 0.0..1.0
	//send_float("Roll", roll);
	//DBG cerr<<"roll:"<<roll<<endl;


	wiimote_copy(m_p_wiimote, m_p_wiimote_old);
	return true;
}

//*** find a suitable master pointer
//XWarpPointer(Display *display,
//				Window src_w,
//				Window dest_w,
//				int src_x, int src_y, unsigned int src_width, unsigned int src_height,
//				int dest_x, int dest_y

//XTestFakeKeyEvent(anXApp::app->dpy, keycode, ispress, delay);




