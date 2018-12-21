
// After installing the Laxkit, compile this program like this:
//
// g++ laxinput.cc -I/usr/include/freetype2 -L/usr/X11R6/lib -lX11 -lXft -lm  -lpng -lcups -llaxkit -o laxinput
//
//Todo:
//  command line:
//    merge masters
//    create/remove masters
//    float/reattach/move slaves:
//      laxinput move 13 new


#define VERSION_NUMBER "0.1"
#define CONFIG_FILE  "~/.config/laxinput.conf"


#include <lax/anxapp.h>
#include <lax/transformmath.h>
#include <lax/doublebbox.h>
#include <lax/buttondowninfo.h>
#include <lax/strmanip.h>
#include <lax/laxutils.h>
#include <lax/laxoptions.h>
#include <lax/fileutils.h>
#include <lax/language.h>

#include <lax/lists.cc>

#include <sys/file.h>
#include <errno.h>


#include <iostream>
using namespace std;
using namespace Laxkit;
using namespace LaxFiles;
#define DBG

#define ADD_NEW_DEVICE (-100)
#define FLOAT_DEVICE (-101)


//
//typedef struct {
//    int                 type;
//    char*               name;
//    Bool                send_core;
//    Bool                enable;
//} XIAddMasterInfo;
//
//typedef struct {
//    int                 type;
//    int                 deviceid;
//    int                 return_mode; /* AttachToMaster, Floating */
//    int                 return_pointer;
//    int                 return_keyboard;
//} XIRemoveMasterInfo;
//
//typedef struct {
//    int                 type;
//    int                 deviceid;
//    int                 new_master;
//} XIAttachSlaveInfo;
//
//typedef struct {
//    int                 type;
//    int                 deviceid;
//} XIDetachSlaveInfo;
//
//typedef union {
//    int                   type; /* must be first element: XIAddMaster, XIRemoveMaster, XIAttachSlave or XIDetachSlave */
//    XIAddMasterInfo       add;
//    XIRemoveMasterInfo    remove;
//    XIAttachSlaveInfo     attach;
//    XIDetachSlaveInfo     detach;
//} XIAnyHierarchyChangeInfo;
//
//
//extern Status   XIChangeHierarchy(
//    Display*            display,
//    XIAnyHierarchyChangeInfo*  changes,
//    int                 num_changes
//);
//
//





//! For 1..10, return First, Second, Third, ..., Tenth. Else something like "Number 42".
const char *rankname(int i)
{
	if (i==1) return "First";
	if (i==2) return "Second";
	if (i==3) return "Third";
	if (i==4) return "Fourth";
	if (i==5) return "Fifth";
	if (i==6) return "Sixth";
	if (i==7) return "Seventh";
	if (i==8) return "Eighth";
	if (i==9) return "Ninth";
	if (i==10) return "Tenth";

	static char str[100];
	sprintf(str,"Number %d",i);
	return str;
}




//-------------------------------------- DevInfo ---------------------------------------
//these go to DevInfo::locked
#define LOCK_CLICKABLE 2
#define LOCK_MOVABLE   1
#define LOCK_DROPPABLE 4

//these go to DevInfo::flags
#define DEV_IGNORE      (1<<0)
#define DEV_DONT_FLOAT  (1<<1)
#define DEV_DONT_MOVE   (1<<2)
#define DEV_DONT_PRUNE  (1<<3)


/*! \class DevInfo
 * \brief Class to hold device info and an ui block.
 */
class DevInfo : public DoubleBBox
{
  public:
	 //ui stuff
	int offsetx, offsety;

	 //device stuff
	string name,alias;
	int xid;
	int type;
	int master_xid;
	XIDeviceInfo *deviceinfo;
	int group;
	int flags;

	int locked; //refers to ui boxes
	int touched;

	DevInfo();
	DevInfo(XIDeviceInfo *dev);
	DevInfo(const char *name, int id,int lock=0);
	~DevInfo();
};

DevInfo::DevInfo()
  : offsetx(0),offsety(0), name(""), alias(""), xid(-1), type(0), master_xid(-1), deviceinfo(NULL), locked(0)
{
	flags=0;
	group=0;
	touched=0;
}

DevInfo::DevInfo(XIDeviceInfo *dev)
  : offsetx(0),offsety(0), 
	name(dev->name),
	alias(""),
	xid(dev->deviceid),
	type(dev->use),
	master_xid(dev->attachment),
	deviceinfo(NULL),
	locked(0)
{
	flags=0;
	group=0;
	touched=0;

	if (!strcmp(name.c_str(),"Virtual core pointer"))             locked=(LOCK_CLICKABLE|LOCK_MOVABLE);
	else if (!strcmp(name.c_str(),"Virtual core keyboard"))       locked=(LOCK_CLICKABLE|LOCK_MOVABLE);
	else if (!strcmp(name.c_str(),"Virtual core XTEST pointer"))  locked=(LOCK_CLICKABLE|LOCK_MOVABLE);
	else if (!strcmp(name.c_str(),"Virtual core XTEST keyboard")) locked=(LOCK_CLICKABLE|LOCK_MOVABLE);
	else if (strstr(name.c_str(),"XTEST")) locked=(LOCK_CLICKABLE|LOCK_MOVABLE);

//	if (dev->use==XIMasterPointer) {
//
//	} else if (dev->use==XIMasterKeyboard) {
//		dm->devices.push(new XInput2Keyboard(dev));
//
//	} else if (dev->use==XISlavePointer) {
//	} else if (dev->use==XISlaveKeyboard) {
//	} else if (dev->use==XIFloatingSlave) {
//	}
}

DevInfo::DevInfo(const char *str, int id, int lock)
  : offsetx(0),offsety(0), 
	name(str),
	alias(""),
	xid(id),
	type(id),
	master_xid(-1),
	deviceinfo(NULL),
	locked(lock)
{
	flags=0;
	group=0;
	touched=0;
}

DevInfo::~DevInfo()
{
	if (deviceinfo) XIFreeDeviceInfo(deviceinfo);
}

//-------------------------------------- LaxInputManager ---------------------------------------
/*! \class LaxInputManagerWindow
 * \brief Window class of the manager.
 */
class LaxInputManagerWindow : public anXWindow
{
  public:
	char *message;
	double view_m[6];
	PtrStack<DevInfo> devices;
	int number_master_pairs;
	int addandremove;

	ButtonDownInfo buttondown;
	int mouse_hover;
	int mouse_grab;

	DevInfo *newpair, *floating;

	LaxInputManagerWindow(anXWindow *parnt, const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsend);
	virtual ~LaxInputManagerWindow();

	virtual int init();
	virtual int DeviceChange(const DeviceEventData *e);
	virtual int CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb);
	virtual int LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int LBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int RBUp(int x,int y,unsigned int state,const LaxMouse *d);
	virtual int MouseMove (int x,int y,unsigned int state, const LaxMouse *m);
	virtual int WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual int WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d);
	virtual void Refresh();

	virtual int getdev(int x,int y);
	virtual int finddevindex(int xid);
	virtual DevInfo *finddev(int xid);
	virtual int RemapHierarchy();
	virtual int RepositionDeviceBoxes();
	virtual int type_of_floating_device(int xid);
	virtual int getMasterXid(int index,  int *p_xid, int *k_xid);
	virtual int getMasterIndex(const char *name,  int *p_index, int *k_index);
	virtual void Message(const char *mes);

	virtual int PruneAll();
	virtual int PruneMaster(int xid, int index);
	virtual int AddMaster(const char *name);
	virtual int FloatDevice(int xid, int index);
	virtual int MergeMasters(int fromxid, int fromindex, int toxid, int toindex);
	virtual int NewMasterFromDevice(int xid, int index);
	virtual int MoveDevice(int fromxid, int fromindex, int toxid, int toindex);

	virtual int LoadConfig(const char *file);
	virtual int SaveConfig(const char *file);
	virtual int SaveConfig(FILE *f, int intro, int ids);
	virtual int getflags(const char *str);
	virtual void outputflags(FILE *f,int indent,int flags);
};

LaxInputManagerWindow::LaxInputManagerWindow(anXWindow *parnt, const char *nname, const char *ntitle,
			unsigned long nstyle,
			int xx,int yy,int ww,int hh,int brder,
			anXWindow *prev,unsigned long nowner,const char *nsend)
  : anXWindow(parnt,nname,ntitle,nstyle|ANXWIN_DOUBLEBUFFER,xx,yy,ww,hh,brder,prev,nowner,nsend)
{
	transform_identity(view_m);
	mouse_hover=-1;
	mouse_grab=-1;
	InstallColors(THEME_Panel);
	addandremove=1;
	message=NULL;

	number_master_pairs=0;
}

LaxInputManagerWindow::~LaxInputManagerWindow()
{
}

void LaxInputManagerWindow::Message(const char *mes)
{
	makestr(message,mes);
	needtodraw=1;
}

//! Load a config from file, or from ~/.config/laxinput.conf if file==NULL.
/*! Return 0 for success or nonzero for error.
 *
 * \todo currently, the config is consumed, and leftover info is not remembered.
 */
int LaxInputManagerWindow::LoadConfig(const char *file)
{
	if (!file) file=CONFIG_FILE;
	char *conf=expand_home(file);

	int fd=open(conf,O_RDONLY);
	delete[] conf;
	if (fd<0) return 1;
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"r");
	if (!f) return 2;
	
	Attribute att;
	att.dump_in(f,0);

	flock(fd,LOCK_UN);
	fclose(f);


	 //try to map the current devices to the config, as possible
	for (int c=0; c<devices.n; c++) {
		if (devices.e[c]->flags&DEV_IGNORE) devices.e[c]->touched=1;
		else devices.e[c]->touched=0;
	}

	char *name, *value, *mnamep;
	string mname;
	int c2;
	int mdev, mpdev, mkdev, sdev;
	for (int c=0; c<att.attributes.n; c++) {
		name =att.attributes.e[c]->name;
		value=att.attributes.e[c]->value;

		if (!strcmp(name,"floating")) {
			for (int c2=0; c2<devices.n; c2++) {
				if (devices.e[c2]->touched) continue;
				if (strcmp(devices.e[c2]->name.c_str(),value)) continue;
				FloatDevice(0,c2);
				att.remove(c);
				c--;
				break;
			}

		} else if (!strcmp(name,"master")) {
			mdev=mkdev=mpdev=0;
			mnamep=value;

			 //find a target master device
			mname=mnamep;
			mname=mname+" pointer";
			for (mdev=0; mdev<devices.n; mdev++) {
				if (devices.e[mdev]->touched) continue;
				if (devices.e[mdev]->type!=XIMasterPointer) continue;
				if (strcmp(devices.e[mdev]->name.c_str(),mname.c_str())) continue;
				break;
			}
			if (mdev==devices.n) {
				 //device not found, so add new one
				 //***maybe should compare 1:1 for devices rather than by name... if there
				 //   are extraneous devices, they stick around
				AddMaster(mnamep);
			}
			getMasterIndex(mnamep, &mpdev, &mkdev);
			mdev=mpdev;

			 //for each slave device in the config for that master...
			if (mdev>=0) for (c2=0; c2<att.attributes.e[c]->attributes.n; c2++) {
				name =att.attributes.e[c]->attributes.e[c2]->name;
				value=att.attributes.e[c]->attributes.e[c2]->value;

				if (!strcmp(name,"flags")) {
					 //master flags
					devices.e[mdev]->flags=getflags(value);

				} else if (!strcmp(name,"keyboard") || !strcmp(name,"pointer")) {
					if (strstr(value,"XTEST")) continue; //ignore the xtest devices

					 //find a slave device somewhere in the device tree
					for (sdev=0; sdev<devices.n; sdev++) {
						if (!strcmp(value, devices.e[sdev]->name.c_str())) break;
					}
					if (sdev==devices.n) continue;
					for (int c3=0; c3<att.attributes.e[c]->attributes.e[c2]->attributes.n; c3++) {
						name =att.attributes.e[c]->attributes.e[c2]->attributes.e[c3]->name;
						value=att.attributes.e[c]->attributes.e[c2]->attributes.e[c3]->value;
						if (!strcmp(name,"flags")) devices.e[sdev]->flags=getflags(value);
					}
					if (devices.e[sdev]->master_xid!=devices.e[mkdev]->xid) {
						 //slave exists, but is under wrong parent;
						MoveDevice(0,sdev, 0,!strcmp(name,"pointer") ? mpdev : mkdev);
					}

					 //refind index for master, since the device list order is now changed
					getMasterIndex(mnamep, &mpdev, &mkdev);

				} //was slave keyboard or pointer
			} //master attributes
		} //found master in config
	} //config attributes

	return 0;
}

//! Save the current configuration to f.
/*! If intro, then output a brief commented description of the file too.
 * If ids, then also output the X ids of the devices.
 */
int LaxInputManagerWindow::SaveConfig(FILE *f, int intro, int ids)
{
	if (!f) return 1;

	if (intro) {
		fprintf(f,"#This file was created by Laxinput version %s.\n",VERSION_NUMBER);
		fprintf(f,"#\n"
				  "#Please note the first master is always considered to be \"core\"\n"
				  "#If you specify further master devices, and those master devices already exist,\n"
				  "#then their names are not changed when you specify another name.\n"
				  "#For any device you list, Laxinput attempts to place it in an appropriate master\n"
				  "#device. Any device not listed is not moved.\n"
				  "#\n"
				  "#When Laxinput uses this file at startup, it will by default only save back\n"
				  "#the listed devices, and any new devices created while running. Other devices\n"
				  "#are just ignored.\n"
				  "#\n"
				  "#Devices can have various combinations of flags to say what cannot be done with them.\n"
				  "#\"dontdelete\" can only be used on master devices. The others can be used on any.\n"
				  "#\n"
				  "#Format is:\n"
				  "#  master (master-name)\n"
				  "#    flags dontdelete\n"
				  "#    pointer (pointer-name)\n"
				  "#      flags dontfloat, dontmove\n"
				  "#    keyboard (keyboard-name)\n"
				  "#      flags ignore\n"
				  "#  floating (name)\n\n");
	}

	DevInfo *d;
	char *tmp;
	const char *ptr,*nme;
	for (int c=0; c<devices.n; c++) {
		d=devices.e[c];

		if (d->type==XIFloatingSlave) {
			if (ids) fprintf(f,"floating  id=%-2d  %s\n",d->xid, d->name.c_str());
			else fprintf(f,"floating %s\n",d->name.c_str());
			if (d->flags) outputflags(f,2,d->flags);

		} else if (d->type==XIMasterPointer) {
			ptr=nme=d->name.c_str();
			while (ptr) {
				ptr=strstr(ptr," pointer");
				if (!ptr) { cerr <<" *** warning! malformed master name!"<<endl; continue; }
				if (ptr[8]=='\0') break;
				ptr++;
			}
			if (!*ptr) { cerr <<" *** warning! malformed master name!"<<endl; continue; }
			tmp=newnstr(nme,ptr-nme+1);

			if (ids) fprintf(f,"master  ptrid=%d keyid=%d   %s\n", d->xid,d->master_xid, tmp);
			else fprintf(f,"master %s\n",tmp);
			if (d->flags) outputflags(f,2,d->flags);
			delete[] tmp;

		} else if (d->type==XIMasterKeyboard) {
			//ignore

		} else if (d->type==XISlavePointer) {
			if (ids) fprintf(f,"  pointer   id=%-2d  %s\n",d->xid,d->name.c_str());
			else fprintf(f,"  pointer  %s\n",d->name.c_str());
			if (d->flags) outputflags(f,4,d->flags);

		} else if (d->type==XISlaveKeyboard) {
			if (ids) fprintf(f,"  keyboard  id=%-2d  %s\n",d->xid,d->name.c_str());
			else fprintf(f,"  keyboard %s\n",d->name.c_str());
			if (d->flags) outputflags(f,4,d->flags);
		}

	}
	return 0;
}

//! Retrieve flags from a space separated list.
int LaxInputManagerWindow::getflags(const char *str)
{
	if (!str) return 0;
	int n;
	char **strs=splitspace(str,&n);
	int f=0;
	for (int c=0; c<n; c++) {
		if (!strcmp(strs[0],"ignore"))           f|=DEV_IGNORE;
		else if (!strcmp(strs[0]," dontfloat"))  f|=DEV_DONT_FLOAT;
		else if (!strcmp(strs[0]," dontmove"))   f|=DEV_DONT_MOVE;
		else if (!strcmp(strs[0]," dontdelete")) f|=DEV_DONT_PRUNE;
	}
	deletestrs(strs,n);
	return f;
}

void LaxInputManagerWindow::outputflags(FILE *f,int indent,int flags)
{
	if (!flags) return;
	char spc[indent+1]; memset(spc,' ',indent); spc[indent]='\0';
	fprintf(f,"%sflags ",spc);
	if (flags&DEV_IGNORE)     fprintf(f," ignore");
	if (flags&DEV_DONT_FLOAT) fprintf(f," dontfloat");
	if (flags&DEV_DONT_MOVE)  fprintf(f," dontmove");
	if (flags&DEV_DONT_PRUNE) fprintf(f," dontdelete");
}

//! Save the current configuration to file, or to "~/.config/laxinput.conf" if file==NULL.
int LaxInputManagerWindow::SaveConfig(const char *file)
{
	if (!file) file=CONFIG_FILE;
	char *conf=expand_home(file);

	int fd=open(conf,O_CREAT|O_TRUNC|O_WRONLY,S_IRUSR|S_IWUSR);
	delete[] conf;
	DBG  cerr<<"errno:"<<errno<<endl;
	DBG perror(NULL);
	if (fd<0) return 1;
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"w");
	if (!f) return 2;
	
	SaveConfig(f,1,0);

	flock(fd,LOCK_UN);
	fclose(f);

	cerr << "Config saved to "<<(file?file:CONFIG_FILE)<<endl;
	return 0;
}

int LaxInputManagerWindow::init()
{
	anXWindow::init();

	DBG cerr <<"------------------- Getting X device info ---------------------------"<<endl;

	 //do some XInput2 initialization
	 // We support XI 2.0 
	int event, error;
	int xinput2_opcode;
	if (!XQueryExtension(anXApp::app->dpy, "XInputExtension", &xinput2_opcode, &event, &error)) {
		DBG cerr<<"X Input extension not available!"<<endl;
		return 0;
	}

	int major=2, minor=0;
	int rc = XIQueryVersion(anXApp::app->dpy, &major, &minor);
	if (rc == BadRequest) {
		printf("No XI2 support. Server supports version %d.%d only.\n", major, minor);
		return 0;
	} else if (rc != Success) {
		DBG cerr <<  "XIQueryVersion Internal Error! This is a bug in Xlib."<<endl;
	}
	DBG cerr << "XI2 supported. Server provides version "<<major<<'.'<<minor<<endl;


	 //define the devices list
	RemapHierarchy();

	 //place the ui boxes for each device
	RepositionDeviceBoxes();


	DBG cerr <<"-------------done XInput2 add devices--------------"<<endl;
	return 0;
}

int LaxInputManagerWindow::DeviceChange(const DeviceEventData *ee)
{
	if (ee->subtype==LAX_DeviceHierarchyChange) {
		 //if add/removed a slave device, that is, if something was just plugged in or unplugged
		DBG cerr <<" *** device change hierarchy event"<<endl;

		const DeviceEventData *e=dynamic_cast<const DeviceEventData*>(ee);
		if (!e) return 0;

		if (!addandremove) {
			RemapHierarchy();
			RepositionDeviceBoxes();
			return 0;
		}

		if (e->xflags&XISlaveAdded) {
			 //If a slave is added, we need to create a new master device and move it there.
			RemapHierarchy();
			RepositionDeviceBoxes();
			NewMasterFromDevice(e->xdev, -1);

		} else if (e->xflags&XISlaveRemoved) {
			 //If the master has no slave devices other than xtest devices, then we
			 //need to remove that master.
			DBG cerr <<"XISlaveRemoved "<<e->xdev<<", attachment:"<<e->xattachment<<endl;

			//*** should only prune the one that the slave came from!!
			RemapHierarchy();
			RepositionDeviceBoxes();
			PruneAll();
		}
		return 0;
	}
	return 0;
}

//! Rescan the current X device tree.
/*! Currently, this will flush devices, and repopulate.
 *
 * This does NOT reconfigure the ui elements. You will need to specially call RepositionDeviceBoxes().
 */
int LaxInputManagerWindow::RemapHierarchy()
{
	 //don't flush yet, because we want to preserve flags
	PtrStack<DevInfo> olddevices;
	DevInfo **olddevs;
	int oldn;
	char *oldlocal;
	olddevs=devices.extractArrays(&oldlocal, &oldn);
	olddevices.insertArrays(olddevs, oldlocal, oldn);


	XIDeviceInfo *info, *dev;
	int ndevices;
	int i;


	info = XIQueryDevice(app->dpy, XIAllDevices, &ndevices);

	number_master_pairs=0;
	for(i = 0; i < ndevices; i++) {
		dev = &info[i];
		DBG cerr <<"Adding "<<dev->name<<", id:"<<dev->deviceid<<endl;

		if (dev->use==XIMasterPointer) number_master_pairs++;
		devices.push(new DevInfo(dev));
	}

	XIFreeDeviceInfo(info);


	 //construct device hierarchy...
	int c=0,c2,c3;
	while (c!=devices.n) {
		 //one iteration for each master pointer/keyboard pair

		for (c2=c; c2<devices.n; c2++) {
			if (devices.e[c2]->type==XIMasterPointer) break;
		}

		if (c2==devices.n) break;

		if (c2!=c) devices.swap(c2,c);

		//now device[c] is an XIMasterPointer
		//we need to have all its slave devices under it
		i=c+1; //i points to the 1st device that is not a slave of the master pointer at c
		for (c2=i; c2<devices.n; c2++) {
			if (devices.e[c2]->type==XISlavePointer && devices.e[c2]->master_xid==devices.e[c]->xid) {
				i++;
				continue;
			}
			for (c3=c2+1; c3<devices.n; c3++) {
				if (devices.e[c3]->type==XISlavePointer && devices.e[c3]->master_xid==devices.e[c]->xid) {
					//swap with a wrongly placed device at c2
					devices.swap(c2,c3);
					i++;
					break;
				}
			}
			if (c3==devices.n) break; //no further slave devices found
		}

		if (i==devices.n) break;

		//now at position c2==i, need to position the XIMasterKeyboard that is paired with the pointer at c

		for (c2=i; c2<devices.n; c2++) {
			if (devices.e[c2]->type==XIMasterKeyboard && devices.e[c2]->master_xid==devices.e[c]->xid) break;
		}
		if (c2==devices.n) {
			cerr <<"Did not find a matching keyboard for "<<devices.e[c]->name<<"!"<<endl;
			continue;
		}

		if (c2!=i) devices.swap(c2,i);


		//now device[i] is an XIMasterKeyboard paired with device[c].
		//we need to have all its slave devices under it
		for (c2=i+1; c2<devices.n; c2++) {
			if (devices.e[c2]->type==XISlaveKeyboard && devices.e[c2]->master_xid==devices.e[i]->xid) {
				continue;
			}
			for (c3=c2+1; c3<devices.n; c3++) {
				if (devices.e[c3]->type==XISlaveKeyboard && devices.e[c3]->master_xid==devices.e[i]->xid) {
					//swap with a wrongly placed device at c2
					devices.swap(c2,c3);
					break;
				}
			}
			if (c3==devices.n) break; //no further slave devices found
		}

		if (c2==devices.n) break;

		c=c2;
	}

	//So now we have arranged the list of devices into some kind of order. Any floating devices should all
	//be at the end of the stack. Now we need to allocate the ui boxes corresponding to the devices.

	floating=new DevInfo("Floating Devices",FLOAT_DEVICE,1);
	c2=devices.n;
	while (c2>0 && devices.e[c2-1]->type==XIFloatingSlave) c2--;
	devices.push(floating,1,c2); //pushes ahead of floating devices

	 //add extra boxes for New Master Pointer/Keyboard, and Floating Devices
	newpair=new DevInfo("New Master Pointer/Keyboard",ADD_NEW_DEVICE,1);
	devices.push(newpair);


	 //preserve flags and touched
	for (c=0; c<olddevices.n; c++) {
		for (c2=0; c2<devices.n; c2++) {
			if (olddevices.e[c]->xid==devices.e[c2]->xid) {
				devices.e[c2]->touched=olddevices.e[c]->touched;
				devices.e[c2]->flags  =olddevices.e[c]->flags;
				break;
			}
		}
	}
	olddevices.flush();


	return 0;
}

//! Place the ui boxes for each device.
int LaxInputManagerWindow::RepositionDeviceBoxes()
{
	int y=0, off=0;
	int w=0,ww;
	int textheight=app->defaultlaxfont->textheight();
	char *text=NULL;
	for (int c=0; c<devices.n; c++) {
		makestr(text,devices.e[c]->name.c_str());
		if (devices.e[c]->type==XISlavePointer
				|| devices.e[c]->type==XISlaveKeyboard
				|| devices.e[c]->type==XIFloatingSlave)
			off=textheight*3;
		else off=0;

		if (devices.e[c]->type==ADD_NEW_DEVICE) y+=textheight/2;
		devices.e[c]->minx=off;

		
		ww = win_themestyle->normal->Extent(text, -1) + textheight;
		//ww = getextent(text,-1,NULL,NULL,NULL,NULL,1)+textheight;
		if (ww>w) w=ww;
		devices.e[c]->maxx=off+ww;
		devices.e[c]->miny=y;
		devices.e[c]->maxy=y+textheight*1.5;

		y+=textheight*1.5;
	}

	needtodraw=1;
	return 0;
}


int LaxInputManagerWindow::CharInput(unsigned int ch, const char *buffer,int len,unsigned int state, const LaxKeyboard *kb)
{
	DBG cerr <<"laxinput: char "<<(char)ch<<endl;

	if (ch==' ') {
		RemapHierarchy();
		RepositionDeviceBoxes();
		needtodraw=1;
		return 0;

	} else if (ch=='s' && (state&LAX_STATE_MASK)==ControlMask) {
		int status=SaveConfig(NULL);
		DBG cerr <<"SaveConfig status:"<<status<<endl;
		Message("Config saved.");
		//ToolTip *notify=new ToolTip("Config Saved",0);
		//notify->win_style|=ANXWIN_CENTER;
		//app->addwindow(notify);
		// *** post message that config saved!!
		return 0;

	} else if (ch=='p') {
		for (int c=0; c<devices.n; c++) {
			if (devices.e[c]->type==XIMasterPointer) {
				if (PruneMaster(-1,c)==0) c--;
			}
		}
	}

	return anXWindow::CharInput(ch,buffer,len,state,kb);
}

int LaxInputManagerWindow::LBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id,LEFTBUTTON,x,y);
	if (mouse_hover>=0 && !(devices.e[mouse_hover]->locked&LOCK_CLICKABLE)) {
		mouse_grab=mouse_hover;
		needtodraw=1;
	}
	return 1;
}

//! Return the device index with the given xid, or -1 if it doesn't exist.
int LaxInputManagerWindow::finddevindex(int xid)
{
	if (xid<=0) return -1;
	for (int c=0; c<devices.n; c++) if (devices.e[c]->xid==xid) return c;
	return -1;
}

//! Return the device with the given xid.
DevInfo *LaxInputManagerWindow::finddev(int xid)
{
	for (int c=0; c<devices.n; c++) if (devices.e[c]->xid==xid) return devices.e[c];
	return NULL;
}

//! Move a slave device somewhere.
/*! If toxid is a slave device, then move to the appropriate master of that slave.
 *
 * Return 0 for success, nonzero for error.
 */
int LaxInputManagerWindow::MoveDevice(int fromxid, int fromindex, int toxid, int toindex)
{
	 //validate inputs
	if (fromindex<0 && fromxid<=0) return 1;
	if (fromindex<0) { for (fromindex=0; fromindex<devices.n; fromindex++) if (devices.e[fromindex]->xid==fromxid) break; }
	if (fromindex>=devices.n) return 2;
	fromxid=devices.e[fromindex]->xid;

	if (toindex<0 && toxid<=0) return 3;
	if (toindex<0) { for (toindex=0; toindex<devices.n; toindex++) if (devices.e[toindex]->xid==toxid) break; }
	if (toindex>=devices.n) return 4;
	toxid=devices.e[toindex]->xid;

	if (devices.e[fromindex]->type!=XIFloatingSlave
			&& devices.e[fromindex]->type==XISlaveKeyboard
			&& devices.e[fromindex]->type==XISlavePointer)
		return 5;

	int topxid=0, tokxid=0;
	if (getMasterXid(toindex, &topxid, &tokxid)!=0) return 6;

	int devtype=devices.e[fromindex]->type;
	if (devtype==XIFloatingSlave) devtype=type_of_floating_device(devices.e[fromindex]->xid);

	if (devices.e[fromindex]->type!=XIFloatingSlave 
		  && devices.e[fromindex]->master_xid==(devtype==XISlaveKeyboard?tokxid:topxid))
		return 7; //device already at that parent

	XIAnyHierarchyChangeInfo info;
	info.type=XIAttachSlave;
	info.attach.deviceid=devices.e[fromindex]->xid;
	info.attach.new_master=(devtype==XISlaveKeyboard?tokxid:topxid);

	int status=XIChangeHierarchy(app->dpy, &info, 1);
	DBG cerr <<"Attach slave: "<<status<<endl;

	RemapHierarchy();
	RepositionDeviceBoxes();

	return 0;
}

//! Add a new master device with name, or a default name if name==NULL.
/*! Return 0 for success or nonzero for error.
 */
int LaxInputManagerWindow::AddMaster(const char *name)
{
	XIAnyHierarchyChangeInfo info;
	info.type=XIAddMaster;
	info.add.name=const_cast<char *>(name?name:rankname(number_master_pairs+1));
	info.add.send_core=1;
	info.add.enable=1;

	int status=XIChangeHierarchy(app->dpy, &info, 1);
	DBG cerr <<"Add master: "<<status<<endl;

	RemapHierarchy();
	RepositionDeviceBoxes();

	return status;
}

//! Make a new master device, and move the specified slave device to it.
int LaxInputManagerWindow::NewMasterFromDevice(int xid, int index)
{
	if (index<0 && xid<=0) return 1;
	if (index<0) index=finddevindex(xid);
	if (index<0) return 2;
	xid=devices.e[index]->xid;

	int devtype=0;
	if (devices.e[index]->type==XIFloatingSlave) { //for floating, we need to know what type it is
		devtype=type_of_floating_device(devices.e[index]->xid);
	} else if (devices.e[index]->type==XISlaveKeyboard) devtype=XISlaveKeyboard;
	else if (devices.e[index]->type==XISlavePointer) devtype=XISlavePointer;

	if (devtype==0) return 3;

	string newmastername=rankname(number_master_pairs+1);
	AddMaster(newmastername.c_str());

	if (devtype==XISlavePointer) newmastername+=" pointer";
	else newmastername+=" keyboard";

	 //find the newly created master device
	int toxid, toindex;
	for (toindex=0; toindex<devices.n; toindex++) {
		if (!strcmp(devices.e[toindex]->name.c_str(), newmastername.c_str())) break;
	}
	if (toindex==devices.n) return 4;
	toxid=devices.e[toindex]->xid;

	XIAnyHierarchyChangeInfo info;
	info.type=XIAttachSlave;
	info.attach.deviceid=xid;
	info.attach.new_master=toxid;

	XIChangeHierarchy(app->dpy, &info, 1);

	RemapHierarchy();
	RepositionDeviceBoxes();

	return 0;
}

//! Remove any empty master devices.
/*! Returns the number of master devices pruned.
 *
 * If a device is tagged with DEV_DONT_PRUNE, then that device will not be removed if empty.
 */
int LaxInputManagerWindow::PruneAll()
{
	int n;
	for (int c=0; c<devices.n; c++) {
		if (devices.e[c]->type==XIMasterPointer && !(devices.e[c]->flags&DEV_DONT_PRUNE)) {
			if (PruneMaster(-1,c)==0) {
				c--;
				n++;
			}
		}
	}
	return n;
}

//! Remove a master device if it and paired device only have XTEST devices.
/*! If a device is tagged with DEV_DONT_PRUNE, then that device WILL be removed if empty.
 */
int LaxInputManagerWindow::PruneMaster(int xid, int index)
{
	DBG cerr <<"PruneMaster "<<xid<<","<<index<<"..."<<endl;

	if (index<0 && xid<=0) return 1;
	if (index<0) index=finddevindex(xid);
	if (index<0) return 2;
	xid=devices.e[index]->xid;

	if (devices.e[index]->type!=XIMasterPointer && devices.e[index]->type!=XIMasterKeyboard)
		return 3;

	 //cannot prune the core device
	if (!strcmp(devices.e[index]->name.c_str(),"Virtual core pointer"))  return 4;
	if (!strcmp(devices.e[index]->name.c_str(),"Virtual core keyboard")) return 4;

	if (devices.e[index]->type==XIMasterKeyboard)
		while (index>0 && devices.e[index]->type!=XIMasterPointer) index--;

	int n=0, c=index+1;
	for (c=index+1; c<devices.n && devices.e[c]->type==XISlavePointer; c++) {
		if (strstr(devices.e[c]->name.c_str(),"XTEST")) continue;
		DBG cerr <<"contains slave"<<devices.e[c]->name<<endl;
		n++;
	}
	if (devices.e[c]->type!=XIMasterKeyboard) return 5; //this shouldn't happen
	for ( ; c<devices.n && devices.e[c]->type==XISlaveKeyboard; c++) {
		if (strstr(devices.e[c]->name.c_str(),"XTEST")) continue;
		DBG cerr <<"contains slave"<<devices.e[c]->name<<endl;
		n++;
	}

	if (n) return 6; //master device is not empty

	XIAnyHierarchyChangeInfo info;
	info.type=XIRemoveMaster;
	info.remove.deviceid=xid;
	info.remove.return_mode=XIFloating;
	info.remove.return_pointer=0;
	info.remove.return_keyboard=0;

	int status=XIChangeHierarchy(app->dpy, &info, 1);
	DBG cerr <<"Add master: "<<status<<endl;

	RemapHierarchy();
	RepositionDeviceBoxes();

	return 0;
}

//! Float a slave keyboard or slave pointer.
/*! If xid<Return 0 for success or nonzero for error.
 */
int LaxInputManagerWindow::FloatDevice(int xid, int index)
{
	if (index<0 && xid<=0) return 1;
	if (index<0) index=finddevindex(xid);
	if (index<0) return 2;
	if (devices.e[index]->type!=XISlavePointer && devices.e[index]->type!=XISlaveKeyboard) return 3;
	xid=devices.e[index]->xid;

	XIAnyHierarchyChangeInfo info;
	info.type=XIDetachSlave;
	info.detach.deviceid=xid;

	int status=XIChangeHierarchy(app->dpy, &info, 1);
	DBG cerr <<"float slave: "<<status<<endl;

	RemapHierarchy();
	RepositionDeviceBoxes();

	return status;
}

//! Merge one master device into another master.
/*! If xid<Return 0 for success or nonzero for error.
 *
 * If toxid is a slave device, then its parent master device is used as the target device.
 */
int LaxInputManagerWindow::MergeMasters(int fromxid, int fromindex, int toxid, int toindex)
{
	 //validate inputs
	if (fromindex<0 && fromxid<=0) return 3;
	if (fromindex<0) { for (fromindex=0; fromindex<devices.n; fromindex++) if (devices.e[fromindex]->xid==fromxid) break; }
	if (fromindex>=devices.n) return 4;
	fromxid=devices.e[fromindex]->xid;

	if (toindex<0 && toxid<=0) return 1;
	if (toindex<0) { for (toindex=0; toindex<devices.n; toindex++) if (devices.e[toindex]->xid==toxid) break; }
	if (toindex>=devices.n) return 2;
	toxid=devices.e[toindex]->xid;

	if (devices.e[fromindex]->type!=XIMasterPointer && devices.e[fromindex]->type!=XIMasterKeyboard)
		return 5;
	if (devices.e[toindex]->type!=XIMasterPointer && devices.e[toindex]->type!=XIMasterKeyboard)
		return 6;


	int topxid=0, tokxid=0;
	if (getMasterXid(toindex, &topxid, &tokxid)!=0) {
		mouse_grab=-1;
		needtodraw=1;
		return 7;
	}

	 //cannot merge device with itself, it ends up removing the device, and floating slaves!
	if (fromxid==devices.e[toindex]->xid || fromxid==devices.e[toindex]->master_xid) return 8;

	XIAnyHierarchyChangeInfo info;
	info.type=XIRemoveMaster;
	info.remove.deviceid=fromxid;
	info.remove.return_mode=XIAttachToMaster;
	info.remove.return_pointer=topxid;
	info.remove.return_keyboard=tokxid;

	int status=XIChangeHierarchy(app->dpy, &info, 1);
	DBG cerr <<"Remove master, merge with another: "<<status<<endl;

	RemapHierarchy();
	RepositionDeviceBoxes();

	return status;
}

int LaxInputManagerWindow::LBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	int dragged=buttondown.up(d->id,LEFTBUTTON);
	if (!dragged) {
		 //determine if a simple click action is allowed!
		int hover=getdev(x,y);
		if (hover>=0) {
			DBG cerr <<"Add new device..."<<endl;
			if (devices.e[hover]->type==ADD_NEW_DEVICE) AddMaster(NULL);
		}
		mouse_grab=-1;
		needtodraw=1;
		return 0;

	 //dragged:
	} else if (mouse_hover!=mouse_grab && mouse_hover>=0 && mouse_grab>=0) {

		 //float a slave device
		if (devices.e[mouse_hover]->type==FLOAT_DEVICE &&
				(devices.e[mouse_grab]->type==XISlavePointer
				 || devices.e[mouse_grab]->type==XISlaveKeyboard)
		   	  ) {
			DBG cerr <<"Float device "<<devices.e[mouse_grab]->xid<<endl;

			FloatDevice(-1,mouse_grab);

			mouse_grab=-1;
			needtodraw=1;
			return 0;


		 //create a new master pair, and move the slave to it
		} else if (devices.e[mouse_hover]->type==ADD_NEW_DEVICE &&
				(devices.e[mouse_grab]->type==XISlavePointer
				 || devices.e[mouse_grab]->type==XISlaveKeyboard
				 || devices.e[mouse_grab]->type==XIFloatingSlave)
		   	  ) {
			DBG cerr <<"Create new master from a device "<<devices.e[mouse_grab]->xid<<endl;

			NewMasterFromDevice(devices.e[mouse_grab]->xid,mouse_grab);

			mouse_grab=-1;
			needtodraw=1;
			return 0;


		 //Merge a master pointer/keyboard into some other one...
		} else if ((devices.e[mouse_grab]->type==XIMasterPointer || devices.e[mouse_grab]->type==XIMasterKeyboard)
					&& devices.e[mouse_hover]->type!=FLOAT_DEVICE && devices.e[mouse_hover]->type!=ADD_NEW_DEVICE) {

			MergeMasters(-1,mouse_grab, -1,mouse_hover);
			mouse_grab=-1;
			needtodraw=1;
			return 0;


		 //Move a floating device or other slave device into an existing master
		} else if ((devices.e[mouse_grab]->type==XISlavePointer
					 || devices.e[mouse_grab]->type==XISlaveKeyboard
					 || devices.e[mouse_grab]->type==XIFloatingSlave)
				    && devices.e[mouse_hover]->type!=FLOAT_DEVICE
				 	&& devices.e[mouse_hover]->type!=XIFloatingSlave
					&& devices.e[mouse_hover]->type!=ADD_NEW_DEVICE) {

			int topxid=0, tokxid=0;
			if (getMasterXid(mouse_hover, &topxid, &tokxid)!=0) {
				mouse_grab=-1;
				needtodraw=1;
				return 0;
			}

			int devtype=devices.e[mouse_grab]->type;
			if (devtype==XIFloatingSlave) devtype=type_of_floating_device(devices.e[mouse_grab]->xid);

			if (devices.e[mouse_grab]->type!=XIFloatingSlave 
				  && devices.e[mouse_grab]->master_xid==(devtype==XISlaveKeyboard?tokxid:topxid))
				return 0; //device already at that parent

			XIAnyHierarchyChangeInfo info;
			info.type=XIAttachSlave;
			info.attach.deviceid=devices.e[mouse_grab]->xid;
			info.attach.new_master=(devtype==XISlaveKeyboard?tokxid:topxid);

			int status=XIChangeHierarchy(app->dpy, &info, 1);
			DBG cerr <<"Attach slave: "<<status<<endl;

			RemapHierarchy();
			RepositionDeviceBoxes();

			mouse_grab=-1;
			needtodraw=1;
			return 0;

		}
	}

	mouse_grab=-1;
	needtodraw=1;
	return 0;
}

//! Find the indices in devices of the master pointer and keyboard corresponding to name.
/*! If name is something like "blah", then the actual pointer and keyboard are
 *  "blah pointer" and "blah keyboard".
 *
 *  Return 0 for success, nonzero for not found.
 */
int LaxInputManagerWindow::getMasterIndex(const char *name,  int *p_index, int *k_index)
{
	string mname=string(name)+" pointer";
	int mpdev=0, mkdev=0;
	for (mpdev=devices.n-1; mpdev>=0; mpdev--) {
		if (!strcmp(devices.e[mpdev]->name.c_str(),mname.c_str())) break;
	}
	mname=string(name)+" keyboard";
	for (mkdev=devices.n-1; mkdev>=0; mkdev--) {
		if (!strcmp(devices.e[mkdev]->name.c_str(),mname.c_str())) break;
	}
	if (p_index) *p_index=mpdev;
	if (k_index) *k_index=mkdev;

	return mpdev<0 || mkdev<0 ? 1 : 0;
}

//! Get the master keyboard or pointer for the device at index. Return 0 on success.
/*! Returns 1 if index out of range or index is a floating device.
 */
int LaxInputManagerWindow::getMasterXid(int index,  int *p_xid, int *k_xid)
{
	if (index<0 || index>=devices.n || devices.e[index]->type==XIFloatingSlave) return 1;

	if (devices.e[index]->type==XIMasterKeyboard) {
		*p_xid=devices.e[index]->master_xid;
		*k_xid=devices.e[index]->xid;

	} else if (devices.e[index]->type==XIMasterPointer) {
		*p_xid=devices.e[index]->xid;
		*k_xid=devices.e[index]->master_xid;

	} else if (devices.e[index]->type==XISlavePointer) {
		*p_xid=devices.e[index]->master_xid;
		for (int c=0; c<devices.n; c++) {
			if (devices.e[c]->type==XIMasterPointer && devices.e[index]->master_xid==devices.e[c]->xid) {
				*k_xid=devices.e[c]->master_xid;
				break;
			}
		}

	} else if (devices.e[index]->type==XISlaveKeyboard) {
		*k_xid=devices.e[index]->master_xid;
		for (int c=0; c<devices.n; c++) {
			if (devices.e[c]->type==XIMasterKeyboard && devices.e[index]->master_xid==devices.e[c]->xid) {
				*p_xid=devices.e[c]->master_xid;
				break;
			}
		}
	}

	return 0;
}

//! Return XISlavePointer or XISlaveKeyboard.
/*! This tries to determine what type of device xid is. If a device is floating, there IS NO EASY WAY
 * to see if it is a keyboard or mouse!!!!!!!!!!!! So we check the device classes, and if there
 * are keys, then we assume it is a keyboard, otherwise if buttons, assume a mouse.
 *
 * If we really can't tell, return 0;
 */
int LaxInputManagerWindow::type_of_floating_device(int xid)
{
	XIDeviceInfo *dev;
	int ndevices;

	dev = XIQueryDevice(app->dpy, xid, &ndevices);

	int type=0;
	for (int c=0; c<dev->num_classes; c++) {
		if (dev->classes[c]->type==XIButtonClass) { type=XISlavePointer; break; }
		if (dev->classes[c]->type==XIKeyClass) { type=XISlaveKeyboard; break; }
	}

	XIFreeDeviceInfo(dev);
	return type;
}

int LaxInputManagerWindow::MBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, MIDDLEBUTTON, x,y);
	return 0;
}

int LaxInputManagerWindow::MBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id,MIDDLEBUTTON);
	return 0;
}

int LaxInputManagerWindow::RBDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	buttondown.down(d->id, RIGHTBUTTON, x,y);
	return 0;
}

int LaxInputManagerWindow::RBUp(int x,int y,unsigned int state,const LaxMouse *d)
{
	buttondown.up(d->id, RIGHTBUTTON);
	return 0;
}

//! Scroll the screen.
int LaxInputManagerWindow::WheelDown(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	view_m[5]-=win_h*.1;
	needtodraw=1;
	return 0;
}

//! Scroll the screen.
int LaxInputManagerWindow::WheelUp(int x,int y,unsigned int state,int count,const LaxMouse *d)
{
	view_m[5]+=win_h*.1;
	needtodraw=1;
	return 0;
}


//! Return the index of the device the coordinate is over.
int LaxInputManagerWindow::getdev(int x,int y)
{
	for (int c=0; c<devices.n; c++) {
		if (devices.e[c]->boxcontains(x,y)) return c;
	}
	return -1;
}

int LaxInputManagerWindow::MouseMove(int x,int y,unsigned int state, const LaxMouse *m)
{
	if (message) Message(NULL);

	int lastx,lasty;
	buttondown.move(m->id, x,y, &lastx,&lasty);

	int hover=getdev(x-view_m[4], y-view_m[5]);
	DBG cerr <<"mouse over "<<hover<<endl;
	if (hover!=mouse_hover) {
		mouse_hover=hover;
		needtodraw=1;
	}

	if (buttondown.isdown(m->id,LEFTBUTTON)) {
		needtodraw=1;
		return 0;
	}

	if (buttondown.isdown(m->id,MIDDLEBUTTON) || buttondown.isdown(m->id,RIGHTBUTTON)) {
		if (lastx!=x || lasty!=y) {
			view_m[4]+=x-lastx;
			view_m[5]+=y-lasty;
			needtodraw=1;
		}
		return 0;
	}
	return 0;
}

void LaxInputManagerWindow::Refresh()
{
	if (!needtodraw) return;

	Needtodraw(0);

	Displayer *dp = MakeCurrent();


	dp->ClearWindow();

	flatpoint p1,p2;
	int textheight=app->defaultlaxfont->textheight();
	unsigned long fillbg;
	unsigned long bg1, bg2;
	bg1 = win_themestyle->bg.Pixel();
	bg2 = coloravg(bg1,win_themestyle->fg.Pixel(),.1);
	int whichbg=1;
	int fill;


	 //draw each device box
	for (int c=0; c<devices.n; c++) {
		if (devices.e[c]->type==XIMasterPointer) whichbg=!whichbg;
		else if (devices.e[c]->type==FLOAT_DEVICE) whichbg=!whichbg;
		else if (devices.e[c]->type==ADD_NEW_DEVICE) whichbg=!whichbg;

		p1=transform_point(view_m, devices.e[c]->minx,devices.e[c]->miny);
		p2=transform_point(view_m, devices.e[c]->maxx,devices.e[c]->maxy);

		fill=0;
		fillbg = win_themestyle->bg.Pixel();
		if (!(devices.e[c]->locked&LOCK_CLICKABLE)) {
			if (mouse_hover == c) {
				if (buttondown.isdown(0,LEFTBUTTON)) {
					fillbg = coloravg(win_themestyle->bg.Pixel(),rgbcolor(0,255,0),.2); //greenish tint
					fill = 1;
				} else {
					fillbg = coloravg(win_themestyle->bg.Pixel(),win_themestyle->fg,.2); //tint toward fg
					fill = 1;
				}
			} else if (mouse_grab==c) {
				 //mouse_grab>=0 implies button is down
				fillbg = coloravg(win_themestyle->bg.Pixel(),rgbcolor(0,255,0),.2); //greenish tint
				fill = 1;
			}
		}
		
		 //draw alternating master group bg
		dp->NewFG(whichbg?bg2:bg1);
		dp->drawrectangle(0,p1.y+1, win_w, p2.y-p1.y-1, 1);

		 //draw boxes
		if (fill) {
			dp->NewFG(fillbg);
			dp->drawrectangle(p1.x,p1.y, p2.x-p1.x, p2.y-p1.y, 1);
		}

		dp->NewFG(rgbcolor(0,0,0));
		dp->drawrectangle(p1.x,p1.y, p2.x-p1.x, p2.y-p1.y, 0);
		dp->textout(p1.x+textheight/2,p1.y+textheight/4, devices.e[c]->name.c_str(), -1, LAX_TOP|LAX_LEFT);

		//DBG cerr <<"textout: "<<devices.e[c]->name.c_str()<<endl;
	}

	 //hover the device to relocate
	if (mouse_grab>=0 && !(devices.e[mouse_grab]->locked&LOCK_MOVABLE)) {
		int xx,yy, x,y;
		int mid=0;
		buttondown.any(0,LEFTBUTTON, &mid);
		if (mid>0) {
			DevInfo *dev=devices.e[mouse_grab];
			buttondown.getinitial(mid,LEFTBUTTON, &xx,&yy);
			buttondown.getcurrent(mid,LEFTBUTTON, &x ,&y );

			p1=transform_point(view_m, dev->minx,dev->miny);
			p2=transform_point(view_m, dev->maxx,dev->maxy);
			p1.x+=(x-xx);
			p2.x+=(x-xx);
			p1.y+=(y-yy);
			p2.y+=(y-yy);

			dp->NewFG(coloravg(win_themestyle->bg,rgbcolor(0,255,0),.2)); //greenish tint
			dp->drawrectangle(p1.x,p1.y, p2.x-p1.x, p2.y-p1.y, 1);

			dp->NewFG(rgbcolor(0,0,0));
			dp->drawrectangle(p1.x,p1.y, p2.x-p1.x, p2.y-p1.y, 0);
			dp->textout(p1.x+textheight/2,p1.y+textheight/4, dev->name.c_str(), -1, LAX_TOP|LAX_LEFT);
		}
	}

	if (message) {
		int w = win_themestyle->normal->Extent(message,-1) + textheight;

		dp->NewFG(coloravg(win_themestyle->bg.Pixel(),rgbcolor(0,255,0),.2)); //greenish tint
		dp->drawrectangle(0,win_h-2*textheight, w,win_h, 1);

		dp->NewFG(rgbcolor(0,0,0));
		dp->textout(textheight/2,win_h-textheight, message,-1, LAX_VCENTER|LAX_LEFT);
	}

	SwapBuffers();

}

#define COMMAND_Normal      0
#define COMMAND_Background  1
#define COMMAND_List        2
#define COMMAND_Init        3
#define COMMAND_Set         4

const char *usage_text()
{
	string text="Laxinput, version " VERSION_NUMBER "\n";
	text=text+string(_("A simple gui based XInput2 configurator\n"
					 "by Tom Lechner, 2011\n"
					 "\n"
					 "Simply click and drag devices to reposition them.\n"
					 "Drop to the \"Floating Devices\" block to float.\n"
					 "Drag to the \"Add new...\" block to create a new master device pair from that device.\n"
					 "Drag a master device to another device area to merge."));
	return text.c_str();
}

int main(int argc,char **argv)
{
	 //parse options
	int run_in_bg=0;
	int ignore_config=0;
	int command=COMMAND_Normal;

	LaxOptions options;
	options.Add("bg",           'b',0, "Runs in background mode. Set from config file, then add and remove new devices as necessary.");
	options.Add("list",         'l',0, "List current device configuration, then exit.");
	options.Add("initialize",    0 ,0, "Make laxinput remember the current device configuration, then exit.");
	options.Add("set",           0 ,0, "Configure according to the config file, then exit.");
	options.Add("ignore-config",'i',0, "By default the config file is used. This ignores it.");
	options.Add("help",         'h',0, "Show this summary and exit.");
	options.Add("version",      'v',0, "Show version information and exit.");

	int e,n=options.Parse(argc,argv,&e);
	if (n==-1) {
		cerr << "Unknown option "<<argv[e]<<endl;
		exit(1);
	}
	if (n==-2) {
		cerr << "Missing parameter for "<<argv[e]<<endl;
		exit(1);
	}
	if (n) {
		for (LaxOption *o=options.start(); o; o=options.next()) {
			if (o->chr()=='h' || o->chr()=='v') {
				options.HelpHeader(usage_text());
				options.Help();
				exit(0);

			} else if (o->chr()=='i') {
				ignore_config=1;

			} else if (o->chr()=='b') { //run in bg
				run_in_bg=COMMAND_Background;

			} else if (o->chr()=='l') { //list
				run_in_bg=command=COMMAND_List;

			} else if (!strcmp(o->str(),"initialize")) {
				run_in_bg=command=COMMAND_Init;

			} else if (!strcmp(o->str(),"set")) {
				run_in_bg=command=COMMAND_Set;
			}
		}
	}




	 //run!
	anXApp app;
	app.init(argc,argv);

	LaxInputManagerWindow *w= new LaxInputManagerWindow(NULL,"Lax Input Manager","Lax Input Manager",ANXWIN_ESCAPABLE,
														5,5,500,700,0,
														NULL,0,NULL);
	app.addwindow(w, !run_in_bg);

	if (command==COMMAND_Init) {
		 //save the current config to the default config location
		w->SaveConfig(NULL);
		app.close();
		return 0;

	} else if (command==COMMAND_Set) {
		 //set from the default config file
		w->LoadConfig(NULL);
		app.close();
		return 0;

	} else if (command==COMMAND_List) {
		 //print out current config (without loading config file)
		w->SaveConfig(stdout, 0, 1);
		app.close();
		return 0;
	}

	if (!ignore_config) w->LoadConfig(NULL);
	

	cout <<"------Done adding initial windows in main() -------\n";
	app.run();

	cout <<"------ App Close:  -------\n";
	app.close();
	
	cout <<"------ Bye! -------\n";
	return 0;
}

