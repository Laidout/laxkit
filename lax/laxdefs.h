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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_LAXDEFS_H
#define _LAX_LAXDEFS_H


namespace Laxkit {


//------------------------------------ Utility defines --------------------------------------
#ifndef MIN
#define MIN( a, b )  ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX( a, b )  ((a) > (b) ? (a) : (b))
#endif

#ifndef SGN
#define SGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))
#endif

//------------------------------ Laxkit capabilities ----------------------------------
 //these can be queried in anXApp::has()
#define LAX_HAS_IMLIB2  0
#define LAX_HAS_CAIRO   1
#define LAX_HAS_GL      2
#define LAX_HAS_XINPUT2 3
#define LAX_HAS_GRAPHICSMAGICK 4


//-------------------- Misc stuff that needs a better category name -----------------------
#define LAX_ISNOTLOCAL     0 
#define LAX_ISLOCAL        1
#define LAX_ISLOCAL_ARRAY  2
#define LAX_IS_REF_COUNTED 3

 // various flags to possibly be passed to dump_out()
 // LAX_DUMP_NORMAL will always be 0
#define LAX_DUMP_NORMAL   0
#define LAX_DUMP_SVG      1
#define LAX_DUMP_PS       2
#define LAX_DUMP_EPS      3
#define LAX_DUMP_PDF      4

 // note to developer that these 2 should Never be different values
#define LAX_ACCEPT    0
#define LAX_REJECT    1


//------------------------------------ Alignment -------------------------------------------
#define LAX_LEFT      (1<<0)
#define LAX_HCENTER   (1<<1)
#define LAX_RIGHT     (1<<2)
#define LAX_CHAR      (1<<3)
#define LAX_NUMERIC   (1<<4)
#define LAX_TOP       (1<<5)
#define LAX_VCENTER   (1<<6)
#define LAX_BOTTOM    (1<<7)
#define LAX_BASELINE  (1<<8)
#define LAX_CENTER    (1<<1|1<<6)
#define LAX_FLIP      (1<<9)

typedef enum {
	LAX_CUSTOM_ALIGNMENT=0,
	LAX_TOP_LEFT,
	LAX_TOP_MIDDLE,
	LAX_TOP_RIGHT,
	LAX_MIDDLE_LEFT,
	LAX_MIDDLE,
	LAX_MIDDLE_RIGHT,
	LAX_BOTTOM_LEFT,
	LAX_BOTTOM_MIDDLE,
	LAX_BOTTOM_RIGHT
} BBoxReferencePoint;


//----------------------------- Arrangement flow direction -----------------------------------
//***whats with this? should be enum-like?
#define LAX_LRTB        (0)
#define LAX_LRBT        (1)
#define LAX_RLTB        (2)
#define LAX_RLBT        (3)
#define LAX_TBLR        (4)
#define LAX_TBRL        (5)
#define LAX_BTLR        (6)
#define LAX_BTRL        (7)
#define LAX_CUSTOM_FLOW (8)
#define LAX_FLOW_MASK   (0x7)


//----------------------------- Menu item state --------------------------------------------
 // Usually, a menu item state can be only one of these, these are for convenience
 // a state variable can usually hold any state in range [0,255]
 // and mousein/open/etc flags are bits >255
 // Defining like this ensures that HIDDEN==0, OFF=1, ON==2, all else >2 and <=255
#define LAX_HIDDEN      (0)
#define LAX_OFF         (1<<0)
#define LAX_ON          (1<<1)
#define LAX_GRAY        (1<<2)
#define LAX_SEPARATOR   (1<<3)
#define LAX_MSTATE_MASK (0xff)

enum LaxSelect {
	LAX_ONE_ONLY = 1,
	LAX_ZERO_OR_ONE,
	LAX_ONE_OR_MORE,
	LAX_ZERO_OR_MORE
};

 // note to programmer: make sure this can be combined with above state
 // note to programmer: *** why are these here? need better way to organize this sort of state
#define LAX_OPEN        (1<<8)
#define LAX_CCUR        (1<<9)
#define LAX_MOUSEIN     (1<<10)
#define LAX_ISLEAF      (1<<11)
#define LAX_HAS_SUBMENU (1<<12)
#define LAX_ISTOGGLE    (1<<13)
#define LAX_CHECKED     (1<<14)


//----------------------------- Modifier State ----------------------------------
//common mouse button assignments
#define LEFTBUTTON       1
#define MIDDLEBUTTON     2
#define RIGHTBUTTON      4
#define WHEELUPBUTTON    8
#define WHEELDOWNBUTTON  16
#define WHEELLEFTBUTTON  32
#define WHEELRIGHTBUTTON 64
#define MOUSEBACK        128
#define MOUSEFORWARD     256

 //! The mask of basic modifier states.
 /*! 
  *  Currently, in Xlib terms this is: ControlMask|ShiftMask|Mod1Mask|Mod4Mask,
  *  which in Laxkit terms is ControlMask|ShiftMask|AltMask|MetaMask,
  *  <pre>
  *  From /usr/include/X11/X.h:
  *  #define ShiftMask       (1<<0)
  *  #define LockMask        (1<<1)
  *  #define ControlMask     (1<<2)
  *  #define Mod1Mask        (1<<3)
  *  #define Mod2Mask        (1<<4)
  *  #define Mod3Mask        (1<<5)
  *  #define Mod4Mask        (1<<6)
  *  #define Mod5Mask        (1<<7) 
  *  </pre>
  */
#define LAX_STATE_MASK ((1<<0)|(1<<2)|(1<<3)|(1<<6))

//        Lax             Xlib
//  ---------------      ------
//  AltMask          ==  Mod1Mask
//  NumLockMask      ==  Mod2Mask
//  CapsLockMask     ==  LockMask
//  MetaMask         ==  Mod4Mask
//  LeftButtonMask   ==  Button1Mask
//  MiddleButtonMask ==  Button2Mask 
//  RightButtonMask  ==  Button3Mask
//  WheelUpMask      ==  Button4Mask
//  WheelDownMask    ==  Button5Mask
//  KeypadMask       ==  (none)
//
// same as Xlib:
//#define ShiftMask   == ShiftMask
//#define ControlMask == ControlMask
//#define Mod3Mask    == Mod3Mask
//#define Mod5Mask    == Mod5Mask

#ifndef _LAX_PLATFORM_XLIB
# define ShiftMask         (1<<0)
# define ControlMask       (1<<2)
# define Mod3Mask          (1<<5)
# define Mod5Mask          (1<<7)
#endif

#define CapsLockMask      (1<<1)
#define AltMask           (1<<3) 
#define NumLockMask       (1<<4) 
#define MetaMask          (1<<6) 

// a mask for when a key has both left and right versions, like shift, control, and alt.
// If this is present in mods, then it is a right key, else a left key
#define RightKeyMask      (1<<13)

#define LeftButtonMask    (1<<8) 
#define MiddleButtonMask  (1<<9) 
#define RightButtonMask   (1<<10)
#define WheelUpMask       (1<<11)
#define WheelDownMask     (1<<12)

#define KeypadMask        (1<<15)


//----------------------------- Special Keys ----------------------------------
//---these are adapted slightly from Xlib keysyms.
//   See /usr/include/X11/keysymdef.h and /usr/include/X11/XF86keysym.h.
//   The equivalent unicode values are used when available.
//   Legacy keysyms that have no unicode and are less than 
//   0x10000000 get or'd with LAX_SPKEY. Note not all of them are
//   given corresponding LAX_* defines.

//value or'd to various x11 keysyms that don't correspond to unicode
#define LAX_SPKEY    0x20000000

#define LAX_NULL     0
#define LAX_Shift    (LAX_SPKEY | 0xffe1)
#define LAX_Control  (LAX_SPKEY | 0xffe3)
#define LAX_Esc      (LAX_SPKEY | 0xff1b)
#define LAX_Menu     (LAX_SPKEY | 0xff67)
#define LAX_Pause    (LAX_SPKEY | 0xff13)
#define LAX_Alt      (LAX_SPKEY | 0xffe9)
#define LAX_Meta     (LAX_SPKEY | 0xffe7)
#define LAX_Del      (LAX_SPKEY | 0xffff)
#define LAX_Bksp     (LAX_SPKEY | 0xff08)
#define LAX_Tab      9
#define LAX_Ins      (LAX_SPKEY | 0xff63)
#define LAX_Home     (LAX_SPKEY | 0xff50)
#define LAX_End      (LAX_SPKEY | 0xff57)
#define LAX_Enter    13
#define LAX_Pgup     (LAX_SPKEY | 0xff55)
#define LAX_Pgdown   (LAX_SPKEY | 0xff56)
#define LAX_F1       (LAX_SPKEY | 0xffbe)
#define LAX_F2       (LAX_SPKEY | 0xffbf)
#define LAX_F3       (LAX_SPKEY | 0xffc0)
#define LAX_F4       (LAX_SPKEY | 0xffc1)
#define LAX_F5       (LAX_SPKEY | 0xffc2)
#define LAX_F6       (LAX_SPKEY | 0xffc3)
#define LAX_F7       (LAX_SPKEY | 0xffc4)
#define LAX_F8       (LAX_SPKEY | 0xffc5)
#define LAX_F9       (LAX_SPKEY | 0xffc6)
#define LAX_F10      (LAX_SPKEY | 0xffc7)
#define LAX_F11      (LAX_SPKEY | 0xffc8)
#define LAX_F12      (LAX_SPKEY | 0xffc9)
#define LAX_Left     (LAX_SPKEY | 0xff51)
#define LAX_Up       (LAX_SPKEY | 0xff52)
#define LAX_Down     (LAX_SPKEY | 0xff54)
#define LAX_Right    (LAX_SPKEY | 0xff53)
#define LAX_Numlock  (LAX_SPKEY | 0xff7f)
#define LAX_Capslock (LAX_SPKEY | 0xffe5)
#define LAX_ScrollLock (LAX_SPKEY | 0xff14)

//The following are basically one for one with XF86XK_* from /usr/include/X11/XF86keysym.h
//They map various special keys found on different keyboards.

/* Backlight controls. */
#define LAX_MonBrightnessUp   0x1008FF02  /* Monitor/panel brightness */
#define LAX_MonBrightnessDown 0x1008FF03  /* Monitor/panel brightness */
#define LAX_KbdLightOnOff     0x1008FF04  /* Keyboards may be lit     */
#define LAX_KbdBrightnessUp   0x1008FF05  /* Keyboards may be lit     */
#define LAX_KbdBrightnessDown 0x1008FF06  /* Keyboards may be lit     */

/*
 * Keys found on some "Internet" keyboards.
 */
#define LAX_Standby          0x1008FF10   /* System into standby mode   */
#define LAX_AudioLowerVolume 0x1008FF11   /* Volume control down        */
#define LAX_AudioMute        0x1008FF12   /* Mute sound from the system */
#define LAX_AudioRaiseVolume 0x1008FF13   /* Volume control up          */
#define LAX_AudioPlay        0x1008FF14   /* Start playing of audio >   */
#define LAX_AudioStop        0x1008FF15   /* Stop playing audio         */
#define LAX_AudioPrev        0x1008FF16   /* Previous track             */
#define LAX_AudioNext        0x1008FF17   /* Next track                 */
#define LAX_HomePage         0x1008FF18   /* Display user's home page   */
#define LAX_Mail             0x1008FF19   /* Invoke user's mail program */
#define LAX_Start            0x1008FF1A   /* Start application          */
#define LAX_Search           0x1008FF1B   /* Search                     */
#define LAX_AudioRecord      0x1008FF1C   /* Record audio application   */

/* These are sometimes found on PDA's (e.g. Palm, PocketPC or elsewhere)   */
#define LAX_Calculator       0x1008FF1D   /* Invoke calculator program  */
#define LAX_Memo             0x1008FF1E   /* Invoke Memo taking program */
#define LAX_ToDoList         0x1008FF1F   /* Invoke To Do List program  */
#define LAX_Calendar         0x1008FF20   /* Invoke Calendar program    */
#define LAX_PowerDown        0x1008FF21   /* Deep sleep the system      */
#define LAX_ContrastAdjust   0x1008FF22   /* Adjust screen contrast     */
#define LAX_RockerUp         0x1008FF23   /* Rocker switches exist up   */
#define LAX_RockerDown       0x1008FF24   /* and down                   */
#define LAX_RockerEnter      0x1008FF25   /* and let you press them     */

/* Some more "Internet" keyboard symbols */
#define LAX_Back             0x1008FF26   /* Like back on a browser     */
#define LAX_Forward          0x1008FF27   /* Like forward on a browser  */
#define LAX_Stop             0x1008FF28   /* Stop current operation     */
#define LAX_Refresh          0x1008FF29   /* Refresh the page           */
#define LAX_PowerOff         0x1008FF2A   /* Power off system entirely  */
#define LAX_WakeUp           0x1008FF2B   /* Wake up system from sleep  */
#define LAX_Eject            0x1008FF2C   /* Eject device (e.g. DVD)    */
#define LAX_ScreenSaver      0x1008FF2D   /* Invoke screensaver         */
#define LAX_WWW              0x1008FF2E   /* Invoke web browser         */
#define LAX_Sleep            0x1008FF2F   /* Put system to sleep        */
#define LAX_Favorites        0x1008FF30   /* Show favorite locations    */
#define LAX_AudioPause       0x1008FF31   /* Pause audio playing        */
#define LAX_AudioMedia       0x1008FF32   /* Launch media collection app */
#define LAX_MyComputer       0x1008FF33   /* Display "My Computer" window */
#define LAX_VendorHome       0x1008FF34   /* Display vendor home web site */
#define LAX_LightBulb        0x1008FF35   /* Light bulb keys exist       */
#define LAX_Shop             0x1008FF36   /* Display shopping web site   */
#define LAX_History          0x1008FF37   /* Show history of web surfing */
#define LAX_OpenURL          0x1008FF38   /* Open selected URL           */
#define LAX_AddFavorite      0x1008FF39   /* Add URL to favorites list   */
#define LAX_HotLinks         0x1008FF3A   /* Show "hot" links            */
#define LAX_BrightnessAdjust 0x1008FF3B   /* Invoke brightness adj. UI   */
#define LAX_Finance          0x1008FF3C   /* Display financial site      */
#define LAX_Community        0x1008FF3D   /* Display user's community    */
#define LAX_AudioRewind      0x1008FF3E   /* "rewind" audio track        */
#define LAX_BackForward      0x1008FF3F   /* ??? */
#define LAX_Launch0          0x1008FF40   /* Launch Application          */
#define LAX_Launch1          0x1008FF41   /* Launch Application          */
#define LAX_Launch2          0x1008FF42   /* Launch Application          */
#define LAX_Launch3          0x1008FF43   /* Launch Application          */
#define LAX_Launch4          0x1008FF44   /* Launch Application          */
#define LAX_Launch5          0x1008FF45   /* Launch Application          */
#define LAX_Launch6          0x1008FF46   /* Launch Application          */
#define LAX_Launch7          0x1008FF47   /* Launch Application          */
#define LAX_Launch8          0x1008FF48   /* Launch Application          */
#define LAX_Launch9          0x1008FF49   /* Launch Application          */
#define LAX_LaunchA          0x1008FF4A   /* Launch Application          */
#define LAX_LaunchB          0x1008FF4B   /* Launch Application          */
#define LAX_LaunchC          0x1008FF4C   /* Launch Application          */
#define LAX_LaunchD          0x1008FF4D   /* Launch Application          */
#define LAX_LaunchE          0x1008FF4E   /* Launch Application          */
#define LAX_LaunchF          0x1008FF4F   /* Launch Application          */

#define LAX_ApplicationLeft  0x1008FF50   /* switch to application, left */
#define LAX_ApplicationRight 0x1008FF51   /* switch to application, right*/
#define LAX_Book             0x1008FF52   /* Launch bookreader           */
#define LAX_CD               0x1008FF53   /* Launch CD/DVD player        */
#define LAX_Calculater       0x1008FF54   /* Launch Calculater           */
#define LAX_Clear            0x1008FF55   /* Clear window, screen        */
#define LAX_Close            0x1008FF56   /* Close window                */
#define LAX_Copy             0x1008FF57   /* Copy selection              */
#define LAX_Cut              0x1008FF58   /* Cut selection               */
#define LAX_Display          0x1008FF59   /* Output switch key           */
#define LAX_DOS              0x1008FF5A   /* Launch DOS (emulation)      */
#define LAX_Documents        0x1008FF5B   /* Open documents window       */
#define LAX_Excel            0x1008FF5C   /* Launch spread sheet         */
#define LAX_Explorer         0x1008FF5D   /* Launch file explorer        */
#define LAX_Game             0x1008FF5E   /* Launch game                 */
#define LAX_Go               0x1008FF5F   /* Go to URL                   */
#define LAX_iTouch           0x1008FF60   /* Logitch iTouch- don't use   */
#define LAX_LogOff           0x1008FF61   /* Log off system              */
#define LAX_Market           0x1008FF62   /* ??                          */
#define LAX_Meeting          0x1008FF63   /* enter meeting in calendar   */
#define LAX_MenuKB           0x1008FF65   /* distingush keyboard from PB */
#define LAX_MenuPB           0x1008FF66   /* distinuish PB from keyboard */
#define LAX_MySites          0x1008FF67   /* Favourites                  */
#define LAX_New              0x1008FF68   /* New (folder, document...    */
#define LAX_News             0x1008FF69   /* News                        */
#define LAX_OfficeHome       0x1008FF6A   /* Office home (old Staroffice)*/
#define LAX_Open             0x1008FF6B   /* Open                        */
#define LAX_Option           0x1008FF6C   /* ?? */
#define LAX_Paste            0x1008FF6D   /* Paste                       */
#define LAX_Phone            0x1008FF6E   /* Launch phone; dial number   */
#define LAX_Q                0x1008FF70   /* Compaq's Q - don't use      */
#define LAX_Reply            0x1008FF72   /* Reply e.g., mail            */
#define LAX_Reload           0x1008FF73   /* Reload web page, file, etc. */
#define LAX_RotateWindows    0x1008FF74   /* Rotate windows e.g. xrandr  */
#define LAX_RotationPB       0x1008FF75   /* don't use                   */
#define LAX_RotationKB       0x1008FF76   /* don't use                   */
#define LAX_Save             0x1008FF77   /* Save (file, document, state */
#define LAX_ScrollUp         0x1008FF78   /* Scroll window/contents up   */
#define LAX_ScrollDown       0x1008FF79   /* Scrool window/contentd down */
#define LAX_ScrollClick      0x1008FF7A   /* Use XKB mousekeys instead   */
#define LAX_Send             0x1008FF7B   /* Send mail, file, object     */
#define LAX_Spell            0x1008FF7C   /* Spell checker               */
#define LAX_SplitScreen      0x1008FF7D   /* Split window or screen      */
#define LAX_Support          0x1008FF7E   /* Get support (??)            */
#define LAX_TaskPane         0x1008FF7F   /* Show tasks */
#define LAX_Terminal         0x1008FF80   /* Launch terminal emulator    */
#define LAX_Tools            0x1008FF81   /* toolbox of desktop/app.     */
#define LAX_Travel           0x1008FF82   /* ?? */
#define LAX_UserPB           0x1008FF84   /* ?? */
#define LAX_User1KB          0x1008FF85   /* ?? */
#define LAX_User2KB          0x1008FF86   /* ?? */
#define LAX_Video            0x1008FF87   /* Launch video player       */
#define LAX_WheelButton      0x1008FF88   /* button from a mouse wheel */
#define LAX_Word             0x1008FF89   /* Launch word processor     */
#define LAX_Xfer             0x1008FF8A
#define LAX_ZoomIn           0x1008FF8B   /* zoom in view, map, etc.   */
#define LAX_ZoomOut          0x1008FF8C   /* zoom out view, map, etc.  */

#define LAX_Away             0x1008FF8D   /* mark yourself as away     */
#define LAX_Messenger        0x1008FF8E   /* as in instant messaging   */
#define LAX_WebCam           0x1008FF8F   /* Launch web camera app.    */
#define LAX_MailForward      0x1008FF90   /* Forward in mail           */
#define LAX_Pictures         0x1008FF91   /* Show pictures             */
#define LAX_Music            0x1008FF92   /* Launch music application  */

#define LAX_Battery          0x1008FF93   /* Display battery information */
#define LAX_Bluetooth        0x1008FF94   /* Enable/disable Bluetooth    */
#define LAX_WLAN             0x1008FF95   /* Enable/disable WLAN         */
#define LAX_UWB              0x1008FF96   /* Enable/disable UWB            */

#define LAX_AudioForward     0x1008FF97   /* fast-forward audio track    */
#define LAX_AudioRepeat      0x1008FF98   /* toggle repeat mode          */
#define LAX_AudioRandomPlay  0x1008FF99   /* toggle shuffle mode         */
#define LAX_Subtitle         0x1008FF9A   /* cycle through subtitle      */
#define LAX_AudioCycleTrack  0x1008FF9B   /* cycle through audio tracks  */
#define LAX_CycleAngle       0x1008FF9C   /* cycle through angles        */
#define LAX_FrameBack        0x1008FF9D   /* video: go one frame back    */
#define LAX_FrameForward     0x1008FF9E   /* video: go one frame forward */
#define LAX_Time             0x1008FF9F   /* display, or shows an entry for time seeking */
#define LAX_Select           0x1008FFA0   /* Select button on joypads and remotes */
#define LAX_View             0x1008FFA1   /* Show a view options/properties */
#define LAX_TopMenu          0x1008FFA2   /* Go to a top-level menu in a video */

#define LAX_Red              0x1008FFA3   /* Red button                  */
#define LAX_Green            0x1008FFA4   /* Green button                */
#define LAX_Yellow           0x1008FFA5   /* Yellow button               */
#define LAX_Blue             0x1008FFA6   /* Blue button                 */

#define LAX_Suspend          0x1008FFA7   /* Sleep to RAM                */
#define LAX_Hibernate        0x1008FFA8   /* Sleep to disk               */
#define LAX_TouchpadToggle   0x1008FFA9   /* Toggle between touchpad/trackstick */
#define LAX_TouchpadOn       0x1008FFB0   /* The touchpad got switched on */
#define LAX_TouchpadOff      0x1008FFB1   /* The touchpad got switched off */

/* Keys for special action keys (hot keys) */
/* Virtual terminals on some operating systems */
#define LAX_Switch_VT_1      0x1008FE01
#define LAX_Switch_VT_2      0x1008FE02
#define LAX_Switch_VT_3      0x1008FE03
#define LAX_Switch_VT_4      0x1008FE04
#define LAX_Switch_VT_5      0x1008FE05
#define LAX_Switch_VT_6      0x1008FE06
#define LAX_Switch_VT_7      0x1008FE07
#define LAX_Switch_VT_8      0x1008FE08
#define LAX_Switch_VT_9      0x1008FE09
#define LAX_Switch_VT_10     0x1008FE0A
#define LAX_Switch_VT_11     0x1008FE0B
#define LAX_Switch_VT_12     0x1008FE0C

#define LAX_Ungrab           0x1008FE20   /* force ungrab               */
#define LAX_ClearGrab        0x1008FE21   /* kill application with grab */
#define LAX_Next_VMode       0x1008FE22   /* next video mode available  */
#define LAX_Prev_VMode       0x1008FE23   /* prev. video mode available */
#define LAX_LogWindowTree    0x1008FE24   /* print window tree to log   */
#define LAX_LogGrabInfo      0x1008FE25   /* print all active grabs to log */


} // namespace Laxkit

#endif

