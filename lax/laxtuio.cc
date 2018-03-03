//
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

#include <lo/lo.h>
#include <unistd.h>

#include <lax/anxapp.h>
#include <lax/laxtuio.h>

#include <lax/refptrstack.cc>

#include <sys/times.h>
#include <sys/types.h>
//#include <sys/stat.h>
#include <fcntl.h>


#include <iostream>
using namespace std;
#define DBG

namespace Laxkit {


/*! \defgroup osc TUIO and OSC
 * 
 * TUIO (http://www.tuio.org/?specification) is a protocol for sending touch table messages
 * inside Open Sound Control messages.
 *
 * @{
 */




//! Define and install a new TUIOTouchPointer into your application.
/*! Default port is 3333.
 * 
 * See nuigroup.com for lots of ways to make your own big touch table.
 */
int SetupTUIOListener(const char *port) //default port is 3333
{
	TUIOListener *tuio=new TUIOListener(port);
	tuio->Start();
	return anXApp::app->devicemanager->devices.push(tuio);
}


//-------------------------------------- TouchObject --------------------------------

/*! \class TouchObject
 * \brief Basically a LaxMouse for touch objects.
 */


//! Initialize a new touch object, with active==1.
TouchObject::TouchObject(int touchid)
{
	touch_id=touchid;
	x=y=xv=yv=a=0;
	active=1;
	firsttime=1;
	focus=NULL;
}

TouchObject::~TouchObject()
{
	if (focus) focus->dec_count();
}

int TouchObject::getInfo(anXWindow *win,
							 int *screen, anXWindow **child,
							 double *x, double *y, unsigned int *mods,
							 double *pressure, double *tiltx, double *tilty) //extra goodies
{ 
	cerr <<"*** need to implement TouchObject::getInfo()!!"<<endl;
	return 1;
}

//! Update the object's state.
/*! If firsttime==1, then return 1 after setting. Otherwise return 0.
 */
int TouchObject::Set(double xx,double yy,double xxv,double yyv,double aa)
{
	DBG cerr <<"Setting for "<<touch_id<<": "<<xx<<","<<yy<<" v:"<<xxv<<","<<yyv<<" a:"<<aa<<endl;
	x=xx;
	y=yy;
	xv=xxv;
	yv=yyv;
	a=aa;

	if (firsttime==1) {
		firsttime=0;
		return 1;
	}
	return 0;
}



//------------------------------------- TUIOListener ----------------------------------

//forward declare lo_server methods
void error_handler(int num, const char *msg, const char *where);
int tuio_handler(const char *path, const char *types, lo_arg **argv, int argc, void * , void *user_data);


/*! \class TUIOListener
 * \brief Class to listen to TUIO events on a specified port.
 *
 * TUIO is a protocol for passing touch events from a variety of touch enabled surface.
 */



TUIOListener::TUIOListener(const char *port)
{
	filedescriptor=0;

	current_frame=0;
	last_fseq=0;
	firstinactive=0; //index in points of the first TouchObject that is available

	st=lo_server_thread_new_with_proto(
						port,         //const char * port,
						LO_UDP,       //int proto,
						error_handler //lo_err_handler err_h 
					);

    lo_server_thread_add_method(st,
								NULL, // null means any path
								NULL,  //types
								tuio_handler, //the function
								static_cast<void *>(this));  //user data to pass to function
}


TUIOListener::~TUIOListener()
{
	Stop();
	lo_server_thread_free(st);  	
}

//! Error handling for TUIOListener.
void error_handler(int num, const char *msg, const char *where)
{
	cerr <<"Received error #"<<num<<": "<<(msg?msg:"")<<"\n  in: "<<(where?where:"")<<endl;
}

//! Start listening for events.
/*! Return 1 for unable to start, 0 for success.
 */
int TUIOListener::Start()
{
	int error=lo_server_thread_start(st);

	if (error!=0) {
		cerr<< "Could not start tuio server thread!"<<endl;
		return 1;
	}
	cerr <<"Started tuio server thread..."<<endl;
	return 0;
}

//! Stop listening for events.
/*! Return 1 for unable to stop, 0 for success.
 */
int TUIOListener::Stop()
{
	int error=lo_server_thread_stop(st);
	if (error!=0) {
		cerr<< "Could not stop server thread!"<<endl;
		return 1;
	}
	return 0;
}

//! Return a file descriptor used to break anXApp out of its select() call.
int TUIOListener::fd()
{
	return -1;
//	if (filedescriptor<=0) {
//		char path[200];
//		sprintf(path,"/tmp/.lax-%d",getpid());
//		filedescriptor=open(path,O_RDWR|O_CREAT);
//		unlink(path);
//	}
//	return filedescriptor;
}

/*! Basically, just pass off to TUIOListener::tuio_handler().
 */
int tuio_handler(const char *path, 
					const char *types,
					lo_arg **argv, 
					int argc,
					void * /*data*/,
					void *user_data) 
{ 
	if (!path || strcmp(path,"/tuio/2Dcur")) return 1; //spec also defines /tuio/3Dcur, 2Dobj, 2Dblb, and others

    //DBG cerr << "tuio path: "<<path<<endl; //TUIO is "/tuio/2Dcur"
    //DBG for (int i = 0; i < argc; ++i) { 
    //DBG 	printf("arg %d '%c' ", i, types[i]); 
    //DBG     lo_arg_pp(static_cast<lo_type>(types[i]), argv[i]); 
    //DBG     printf("\n"); 
    //DBG } 

	TUIOListener *tuio=static_cast<TUIOListener*>(user_data);
	if (!tuio) return 1;

	//return 0;
	return tuio->tuioHandler(argc,argv,types);
}


//! Handler for tuio events.
/*! TUIO messages can be "set", "alive", or "fseq". (as of the 1.1 specification, anyway)
 * 
 * Idling will produce alternating "alive" and "fseq" messages.
 * "alive" says what tuio objects exist. "fseq" wraps up preceding "set" messages
 * into one frame.
 *
 * For alive, argument indices > 0 will contain integer id numbers of tuio objects.
 * It is up to the application to figure out if this means there is a new object
 * or an object has gone away.
 *
 * fseq messages contain one argument with the current frame number of all set
 * messages between the fseq and the last alive message.
 *
 * set messages have:
 * <pre>
 *  /tuio/2Dobj set id i x y a X Y A m r
 *  /tuio/2Dcur set id xpos ypos Xvelocity Yvelocity acceleration
 *  /tuio/2Dblb set id x y a w h f X Y A m r 
 * </pre>
 *
 * \todo *** a set might occur before an alive for it, should be able to cope with that!
 */
int TUIOListener::tuioHandler( int argc, lo_arg **argv, const char *types)
{
	if (types[0]!='s') return 1;
	if (!strcmp(&argv[0]->s,"alive")) {
		 //update which points are active
		int id;
		TouchObject *obj;

		 //for each touch object, remove any that no longer exist, sending button up messages
		int obji=-1;
		for (int c=0; c<firstinactive; c++) {
			obj=points.e[c];
			obji=c;
			for (int c2=1; c2<argc; c2++) {
				id=argv[c2]->i;
				if (id==obj->touch_id) {
					 //touch object is still alive, we can return
					//DBG cerr <<"Object "<<obj->touch_id<<" still alive, keeping."<<endl;
					obj=NULL;
					break;
				}
			}
			if (!obj) continue;

			DBG cerr <<"Object "<<obj->touch_id<<" not alive, removing.."<<endl;
			DBG cerr <<"tuio button up to "<<obj->buttonwindow<<endl;

			 //touch object was not found to be alive, so we must remove it
			MouseEventData *b=new MouseEventData(LAX_onButtonUp);
			b->to=obj->buttonwindow;
			b->target=NULL;
			b->button=1;
			b->x=(int)obj->x;
			b->y=(int)obj->y;
			b->device=obj;
			anXApp::app->SendMessage(b,obj->buttonwindow,NULL,0);

			 //make obj an inactive touch container
			DBG cerr <<"Moving touch obj "<<obj->touch_id<<" to inactive"<<endl;
			points.pop(obji);
			points.push(obj);
			firstinactive--;
			c--;

			DBG cerr <<"Number of alive after remove objects: "<<firstinactive<<endl;
			DBG cerr <<"Number of total after remove objects: "<<points.n<<endl;
		}


		 //foreach alive object, see if there is an active object assigned to it
		for (int c=1; c<argc; c++) {
			id=argv[c]->i;
			obj=NULL;
			for (int c2=0; c2<firstinactive; c2++) {
				if (id==points.e[c2]->touch_id) {
					obj=points.e[c2];
					break;
				}
			}
			if (obj) continue;

			 //unknown id, so this is a new touch
			if (firstinactive==points.n) {
				points.push(new TouchObject(id));
			} else {
				points.e[firstinactive]->touch_id=id;
				points.e[firstinactive]->firsttime=1;
			}
			firstinactive++;
			points.e[firstinactive-1]->buttonwindow=0;
			DBG cerr <<"Added new touch object "<<points.e[firstinactive-1]->touch_id<<endl;
		}

		//DBG cerr <<"Number of alive objects: "<<firstinactive<<endl;
		//DBG cerr <<"Number of total objects: "<<points.n<<endl;
		return 0;

	} else if (!strcmp(&argv[0]->s,"fseq")) {
		current_frame=argv[1]->i;

		//DBG long last=last_fseq;

		last_fseq=times(NULL)*1000/sysconf(_SC_CLK_TCK); //this is milliseconds

		//DBG cerr <<"tuio fps: "<<1000.0/(last_fseq-last) <<endl;

	} else if (!strcmp(&argv[0]->s,"set")) {
		if (argc!=7) {
			DBG cerr <<"Malformed tuio set message!"<<endl;
			return 1; //malformed set!
		}
		int id=argv[1]->i;
		TouchObject *obj=NULL;
		for (int c=0; c<firstinactive; c++) {
			if (points.e[c]->touch_id==id) {
				obj=points.e[c];
				break;
			}
		}
		if (!obj) return 0; // *** a set might occur before an alive for it, should be able to cope with that!
		int sw,sh;
		double xx,yy,xxv,yyv,aa;

		anXApp::app->ScreenInfo(obj->screen,NULL,NULL,&sw,&sh,NULL,NULL,NULL,NULL);
		//sw=1600; sh=1200;

		 //Arguments thus:  /tuio/2Dcur 0:set 1:id xpos ypos Xvelocity Yvelocity acceleration
		xx =argv[2]->f*sw;
		yy =argv[3]->f*sh;
		xxv=argv[4]->f*sw;
		yyv=argv[5]->f*sh;
		aa =argv[6]->f; // *** how should this be scaled!?!?

		if (obj->firsttime) {
			obj->Set(xx,yy,xxv,yyv,aa);
			
			 //was first time setting location info, so need to determine owning window, and send a buttondown
			anXWindow *win=NULL, *twin;
			anXApp::app->findDropCandidate(NULL,(int)xx,(int)yy,&win,NULL);
			if (win) {
				int dx=0,dy=0;
				twin=win;
				obj->buttonwindow=win->object_id;
				while (twin) {
					dx+=twin->win_x;
					dy+=twin->win_y;
					twin=twin->win_parent;
				}

				DBG cerr <<"tuio button down to "<<win->WindowTitle()<<endl;

				MouseEventData *b=new MouseEventData(LAX_onButtonDown);
				b->to=obj->buttonwindow;
				b->target=NULL;
				b->button=1;
				b->count=1;
				b->x=(int)obj->x-dx;
				b->y=(int)obj->y-dy;
				b->device=obj;
				anXApp::app->SendMessage(b,obj->buttonwindow,NULL,0);
			}

		} else {
			 //just send a mouse move event
			if (obj->buttonwindow && (xx!=obj->x || yy!=obj->y)) {
				obj->Set(xx,yy,xxv,yyv,aa);
				anXApp::app->bump();
				DBG cerr <<"tuio mousemove to window id "<<obj->buttonwindow<<endl;

				MouseEventData *b=new MouseEventData(LAX_onMouseMove);
				b->to=obj->buttonwindow;
				b->target=NULL;
				b->x=(int)obj->x;
				b->y=(int)obj->y;
				b->device=obj;
				anXApp::app->SendMessage(b,obj->buttonwindow,NULL,0);
			}
		}

		return 0;
	}

    return 0;
} 


//! @}


} //namespace Laxkit



