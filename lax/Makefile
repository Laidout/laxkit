################################################
###########                       ##############
##########   Laxkit/lax Makefile   #############
############                      ##############
################################################
#
# You should not have to edit anything in this file.
#
#
# 
#Note:
# to make shared library,
# objects must be compiled without -g and:
# 	g++ -fPIC -c blah.cc -o blah.o
# and linked thus:
# 	g++ -shared -fPIC -o blah.so blah.o ...


 # Where to install stuff, currently:
 #   prefix/include
 #   prefix/lib
#PREFIX=/usr/local

LAX_USING_XLIB=1


## Comment out the next line to not compile in debugging info
## Or use the second one to  not compile in debugging info and also hide all the 
##   output that would otherwise scroll by on the terminal screen.
DEBUGFLAGS= -g -gdwarf-2
#DEBUGFLAGS= -DHIDEGARBAGE



 # the libs get installed here
LIBDIR=$(PREFIX)/lib

 # header files get installed in this directory like this: includedir/lax-version/lax/*.h
INCLUDEDIR=$(PREFIX)/include

### If you want to be sure that an install does not clobber anything that exists
### already, then uncomment the line with the '--backup=t' and comment out the other.
#INSTALL=install --backup=t 
INSTALL=install



include ../Makefile-toinclude

LAXDIR= .
LAXOBJDIR=objs

CC=g++
LD=g++
OPTIMIZATION=
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm  -lpng

## use the second one to make .so
## else the first for static libs only
## the -fPIC for static makes the static lib a little larger
#FPIC=
FPIC= -fPIC

CPPFLAGS= $(OPTIMIZATION) $(EXTRA_LAX_CFLAGS) $(DEBUGFLAGS) $(FPIC) -I$(LAXDIR)/.. -I/usr/X11R6/include -Wall `pkg-config --cflags freetype2`



objs= \
    debug.o \
	refcounted.o \
	anobject.o \
	lark.o \
	dump.o \
	errorlog.o \
	strmanip.o \
	utf8utils.o \
	utf8string.o \
	transformmath.o \
	vectors.o \
	vectors-out.o \
	doublebbox.o \
	fileutils.o \
	freedesktop.o \
	tagged.o \
	iobuffer.o \
	cssutils.o \
	attributes.o \
	units.o \
	undo.o \
	fontmanager.o \
	fontmanager-xlib.o \
	fontmanager-cairo.o \
	fontscanner.o \
	laximages.o \
	laximages-imlib.o \
	laximages-cairo.o \
	laximages-gm.o \
	laximlib.o \
	laxcairo.o \
	laxgm.o \
	laxzip.o \
	dateutils.o \
	screencolor.o \
	laxdevices.o \
	events.o \
	shortcuts.o \
	resources.o \
	objectfactory.o \
	themes.o \
	screeninformation.o \
	anxapp.o \
	anxapp-laxrc.o \
	laxoptions.o \
	previewable.o \
	gradientstrip.o \
	anxwindow.o \
	newwindowobject.o \
	tooltip.o \
	laxutils.o \
	bitmaputils.o \
	noise.o \
	iconmanager.o \
	misc.o \
	buttondowninfo.o \
	bezutils.o \
	displayer.o \
	displayer-xlib.o \
	displayer-cairo.o \
	messagebar.o \
	menuinfo.o \
	filemenuinfo.o \
	texteditbase-utf8.o \
	textxeditbase-utf8.o \
	funcframe.o \
	panuser.o \
	pancontroller.o \
	panpopup.o \
	scroller.o \
	boxarrange.o \
	winframebox.o \
	rowframe.o \
	boxselector.o \
	iconselector.o \
	tabframe.o \
	draggableframe.o \
	buttonbase.o \
	button.o \
	imageinfo.o \
	indexrange.o \
	lineedit.o \
	multilineedit.o \
	promptedit.o \
	lineinput.o \
	itemslider.o \
	iconslider.o \
	numslider.o \
	checkbox.o \
	progressbar.o \
	scrolledwindow.o \
	popupmenu.o \
	menubutton.o \
	sliderpopup.o \
	quickfileopen.o \
	colorspace.o \
	colors.o \
	colorevents.o \
	colorbase.o \
	colorsliders.o \
	colorbox.o \
	palettewindow.o \
	rulerwin.o \
	stackframe.o \
	messagebox.o \
	overwrite.o \
	inputdialog.o \
	filepreviewer.o \
	filedialog.o \
	imagedialog.o \
	simpleprint.o \
	fontdialog.o \
	dateselector.o \
	splitwindow.o \
	treeselector.o \
	keyboardwindow.o \
	shortcutwindow.o \
	shortcutwindow2.o \
	draggingdndwindow.o \
	curveinfo.o \
	pointset.o \
	printdialog.o \

#	tableframe.o \
#	colorselector.o \
#


# All subdirectories that have stuff we need to compile
dirs= \
	  spiro \
	  interfaces

# This is for all .o that need to be included from subdirectories
otherobjs= \
	spiro/rspline.o \
	spiro/spiro.o \
	interfaces/linestyle.o \
	interfaces/fillstyle.o \
	interfaces/somedata.o \
	interfaces/somedataref.o \
	interfaces/somedatafactory.o \
	interfaces/coordinate.o \
	interfaces/selection.o \
	interfaces/aninterface.o \
	interfaces/interfacemanager.o \
	interfaces/viewportwindow.o \
	interfaces/viewerwindow.o \
	interfaces/viewportwithstack.o \
	interfaces/beznet.o \
	interfaces/beznetinterface.o \
	interfaces/captioninterface.o \
	interfaces/characterinterface.o \
	interfaces/colorpatchinterface.o \
	interfaces/curvemapinterface.o \
	interfaces/curvewindow.o \
	interfaces/delaunayinterface.o \
	interfaces/ellipseinterface.o \
	interfaces/engraverfilldata.o \
	interfaces/engraverfillinterface.o \
	interfaces/freehandinterface.o \
	interfaces/gradientinterface.o \
	interfaces/gridselectinterface.o \
	interfaces/groupdata.o \
	interfaces/imageinterface.o \
	interfaces/imagepatchinterface.o \
	interfaces/interfacewindow.o \
	interfaces/lineprofile.o \
	interfaces/mirrorinterface.o \
	interfaces/objectinterface.o \
	interfaces/patchinterface.o\
	interfaces/pathinterface.o \
	interfaces/perspectiveinterface.o \
	interfaces/pressuremapinterface.o \
	interfaces/rectinterface.o \
	interfaces/roundedrectinterface.o \
	interfaces/shapebrush.o \
	interfaces/showkeysinterface.o \
	interfaces/sliderinterface.o \
	interfaces/simplepathinterface.o \
	interfaces/stackpartinterface.o \
	interfaces/svgcoord.o \
	interfaces/textonpathinterface.o \
	interfaces/textstreaminterface.o \
	interfaces/texttopath.o



all: $(objs) alldirs liblaxkit.a liblaxkit.so.$(LAXKITVERSION)
#	for NAME in $(dirs); do $(MAKE) -C $$NAME ; done
	@echo "  -----------Done!-------------" 

# #this compiles the core laxkit only, not subdirs
# almostall: $(objs) liblaxkit.a
# 	@echo "  -----------Done!-------------" 

compileonly: $(objs) compileinterfaces
	@echo "  -----------Done!-------------" 

# This changes in *.cc all occurrences of '(whitespace)DBG' to '(same ws)//DBG' and
# '(whitespace)//DBG' to '(same ws)////DBG'
# which is useful when assembling for a release, because it disables all the spewing
# of info to the terminal.. I couldn't figure out how to do it with a simple compile flag
# like #defining DBG to be: //
hidegarbage:
	./hidegarbage *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME hidegarbage ; done
	#$(MAKE) -C interfaces hidegarbage
	#$(MAKE) -C spiro hidegarbage

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	./hidegarbage -0 *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME unhidegarbage ; done
	#$(MAKE) -C interfaces unhidegarbage
	#$(MAKE) -C spiro unhidegarbage


docs:
	cd ../docs && doxygen

compileinterfaces:
	cd interfaces && $(MAKE) compileonly

interfaces:
	cd interfaces && $(MAKE) all

spiro:
	cd spiro && $(MAKE) all

alldirs:
	for NAME in $(dirs); do $(MAKE) -C $$NAME ; done



#----------------- Dynamic libs ---------------------------
# These .so makers require that all object files be compiled with -fPIC,
# which ultimately adds to the overall size of the static archive
liblaxkit.so.$(LAXKITVERSION): $(objs) alldirs
	$(LD) -shared -fPIC  $(objs) $(otherobjs) -o $@


#----------------- Static libs ---------------------------
#liblaxkit.a: $(dirs)spiro $(objs)
liblaxkit.a: alldirs $(objs)
	ar cr $@ $(objs) $(otherobjs)


#----------------- Other ---------------------------
depends:
	touch makedepend
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc
	for NAME in $(dirs); do $(MAKE) -C $$NAME depends; done
	#$(MAKE) -C interfaces depends
	#$(MAKE) -C spiro depends

# Note: the minus just means don't error if file not found
-include makedepend


#------- Testing Stuff ----------------

#test.o: $(coreincludes) panuser.h pancontroller.h test.h test.cc


#---------- clean up and go home -----------------

# Like clean, but also remove logs of any previous install
dist-clean: clean
	rm -f install.log

.PHONY: clean docs $(dirs) hidegarbage unhidegarbage
clean:
	rm -f *.o *.a *.so
	for NAME in $(dirs); do $(MAKE) -C $$NAME clean; done
	#$(MAKE) -C interfaces clean
	#$(MAKE) -C spiro clean

