//
//
//
//
// This file is for notes regarding implementing key shortcuts and mouse gestures
// with EventFilters, which would automatically process input before the usual
// device functions get called. It is debatable whether it's worth it.
//
//
//
//

#ifndef DOXYGEN_SHOULD_SKIP_THIS

***** Not an active file ***********

//---------------- using EventFilters: ----------------------

/*! 
 * When windows get input events, these can be parsed, and absorbed as necessary. For shortcuts, they
 * would trigger a ShortcutEvent, for instance. If there is a chained shortcut, then input is
 * absorbed until it can be determined if there is an active shortcut for the chain. If not,
 * then all events stored up are released.
 *
 *
 * A window would define a list of actions, and store a list of shortcuts pointing to those actions.
 * In the window's CharInput() function, the action number is found from its (redefinable) shortcuts
 * and something like thewindow->action(shortcut->action) is called.
 *
 *
 * \verbatim
 * shortcut
 *    keys   +^a,c,h,a,i,n   #reserved mods and comma are specified by putting 2 of them
 *    action 5               
 *    shift-alt-'c',h,a,i,n
 *    '+',''',','  someotheraction    # <-- a chain of '+' then apostrophe, then comma
 * \endverbatim

 * \todo perhaps key chaining can be done with a little popup up in the corner of the affected window?
 *   that would greatly relieve the particular window from having a special key chain mode.. it would
 *   just respond to a shortcut-command message..
 */

//----------------------------------- EventFilter ------------------------------------
/*! \class EventFilter
 * \brief Base class for filters windows can use to transform input event streams.
 *
 * For instance, see ShortcutFilter or GestureFilter.
 */
class EventFilter
{
  public:
	EventFilter *next;
	EventFilter() : next(NULL) {}
	virtual ~InputEventfilter() {}
	virtual int eventFilter(EventData **events_ret,EventData *event,anXWindow *target,int &isinput) = 0;
};

/*! \class ShortcutFilter
 * \brief Av event filter windows can use to detect keyboard shortcuts.
 */
class ShortcutFilter : public EventFilter
{
  protected:
	PtrStack<ShortcutDef> shortcuts;

	int index; //which is the active shortcut
	int keyindex; //how far into a shortcut chain to look
	int deviceid; //id of the active keyboard
  public:
	ShortcutFilter();
	virtual ~ShortcutFilter(); 

	virtual int ClearShortcut(unsigned int key, unsigned int state);
	virtual int ClearShortcut(const char *keys, unsigned int *states, int n);
	virtual unsigned int Shortcut(unsigned int key, unsigned int state, unsigned int action, unsigned int mode);
	virtual unsigned int ShortcutChain(const char *keys, unsigned int *states, int n, unsigned int action, unsigned int mode);

	virtual int eventFilter(EventData **events_ret,EventData *e,anXWindow *window);
};

ShortcutFilter::ShortcutFilter()
	: deviceid(0), index(-1), keyindex(0)
{}

ShortcutFilter::~ShortcutFilter()
{}

//! Filter events for shortcuts.
/*! Returns 0 for event ignored. events_ret will be set to NULL.
 * Return -1 for event absorbed, but they may be released in the future.
 * Return 1 for events released.
 * Return 2 for shortcut found and SimpleMessage(NULL,action,0,0,0, LAX_ShortcutEvent) is returned.
 *
 * The previous contents of events_ret are overwritten. 
 */
int ShortcutFilter::eventFilter(int current_mode, EventData **events_ret,const EventData *e,anXWindow *window)
{
	if (!shortcuts.n || e->type!=LAX_onKeyPress) { *events_ret=NULL; return 1; }

	KeyEventData *k=dynamic_cast<KeyEventData*>(e);
	if (!e) { *events_ret=NULL; return 1; }

	if (index>=0) {  //there is an active shortcut with index number of chars read already
		***;

		 //if shortcut found:
		SimpleMessage *m=new SimpleMessage(NULL,action,0,0,0, LAX_ShortcutEvent);
		*events_ret=m;
	}


	 //else no active shortcut, check for activating a new one
	for (int c=0; c<shortcuts.n; c++) {
		*** //if key==0 but buffer has info then...

		if (current_mode==shortcuts.e[c]->mode
			  && k->key==shortcuts.e[c]->key
			  && k->modifiers==shortcuts.e[c]->modifiers) {
			*** found a match;
		}
	}

	 //no match found, so release event
	*events_ret=NULL;
	return 1;
}




//----shortcut types:
//--keys:
//Shortcut_KeyDown
//Shortcut_KeyUp
//--mouse:
//Shortcut_BDown
//Shortcut_BUp
//Shortcut_BUpMoved
//Shortcut_Move
//Shortcut_BMove

int anXWindow::CharInput(unsigned int ch,const char *buffer,int len,unsigned int state, const LaxDevice *d)
{
	if (doEventFilters()) return 0; //key absorbed
	...//else we must still process the key

	------------
	action=getAction(ch,state,Shortcut_KeyDown);
	if (action<0) return 1;
	win_shortcut=ch;
	if (processAction(action,0,0)<=0) return 0;
	return 1;
}
int anXWindow::KeyUp(unsigned int ch, unsigned long state, const LaxDevice *d)
{
	if (win_shortcut>=0) {
		win_shortcut=-1;
		action=getAction(ch,state,Shortcut_KeyUp);
		if (processAction(win_shortcut,0,0)<=0) return 0;
	}
	action=getAction(ch,state,Shortcut_KeyUp);
	if (action>=0) if (processAction(action,0,0)<=0) return 0;
	return 1;
}

//---------------for mouse actions:

//------------------------------ GestureFilter ---------------------------------------
/*! \class GestureFilter
 * \brief Event filter to detect some simple multitouch mouse gestures.
 */
class GestureFilter : public EventFilter
{
  public:
	int deviceid;
	clock_t start_time;
	MouseInfo *mouseevents;

	GestureFilter();
	virtual ~GestureFilter();
};


//--------------------other: old code:...
//class MouseCutDef
//{
// protected:
//	unsigned int button;
//	unsigned long state;
//	int mode;
//	int action,upaction,upnomove,move,downmove;
//};

//! button=1,2,3,4,5. (0=no button)
int anXWindow::getMouseAction(int button, unsigned long state, int what, int x,int y, const LaxDevice *d)
{
	ShortcutDef *s=win_mousecuts[0];
	int c=0;
	while (s) {
		s=win_mouseshortcuts[c];
		if (button==s->button && state==s->state && win_mode==s->mode) return c;
		c++;
	}
	return -1;
}

int anXWindow::LBDown(int x,int y, unsigned long state, const LaxDevice *d)
{
	action=getAction(LEFTBUTTON,state,x,y);
	if (action<0) return 1;
	if (processAction(action,Shortcut_BDown,x,y)>0) return 1;
	win_mousecut=action;
	win_dragged=0;
	return 1;
}
int anXWindow::MouseMove(int x,int y, unsigned long state, const LaxDevice *d)
{
	if (win_mousecut>=0) {
		win_dragged=1;
		if (processAction(win_mousecut,Shortcut_BMove,x,y)>0) return 1;
		return 0;
	} else {
		action=getAction(0,state,x,y);
		if (action<0) return 1;
		if (processAction(action,Shortcut_Move,x,y)>0) return 1;
		return 1;
	}
	return 1;
}
int anXWindow::LBUp(int x,int y, unsigned long state, const LaxDevice *d)
{
	if (win_mousecut>0) {
		win_mousecut=-1;
		if (win_dragged) {
			if (processAction(win_mousecut,Shortcut_BUpMoved,x,y)<=0) return 0;
		} else if (processAction(win_mousecut,Shortcut_BUp,x,y)<=0) return 0;
	}
	return 1;
}



#endif //DOXYGEN_SHOULD_SKIP_THIS


