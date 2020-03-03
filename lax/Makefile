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
	doublebbox.o \
	fileutils.o \
	freedesktop.o \
	tagged.o \
	iobuffer.o \
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
	buttonbase.o \
	button.o \
	imageinfo.o \
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
	colorbase.o \
	colorsliders.o \
	colorbox.o \
	palette.o \
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

#	printdialog.o \
#	tableframe.o \
#	colorselector.o \
#

otherobjs= \
	spiro/rspline.o \
	spiro/spiro.o



#this compiles the core laxkit only, not interfaces
almostall: $(objs) liblaxkit.a
	@echo "  -----------Done!-------------" 

all: $(objs) liblaxkit.a interfaces liblaxkit.so.$(LAXKITVERSION)
	@echo "  -----------Done!-------------" 

# This changes in *.cc all occurrences of '(whitespace)DBG' to '(same ws)//DBG' and
# '(whitespace)//DBG' to '(same ws)////DBG'
# which is useful when assembling for a release, because it disables all the spewing
# of info to the terminal.. I couldn't figure out how to do it with a simple compile flag
# like #defining DBG to be: //
hidegarbage:
	./hidegarbage *.cc
	$(MAKE) -C interfaces hidegarbage
	$(MAKE) -C spiro hidegarbage

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	./hidegarbage -0 *.cc
	$(MAKE) -C interfaces unhidegarbage
	$(MAKE) -C spiro unhidegarbage


docs:
	cd ../docs && doxygen

interfaces:
	cd interfaces && $(MAKE) all

spiro:
	cd spiro && $(MAKE) all


#----------------- Dynamic libs ---------------------------
#these .so makers require that all object files be compiled with -fPIC,
#which ultimately adds to the overall size of the static archive
liblaxkit.so.$(LAXKITVERSION): $(objs)
	$(LD) -shared -fPIC  $(objs) -o $@


#----------------- Static libs ---------------------------
liblaxkit.a: spiro $(objs)
	ar cr $@ $(objs) $(otherobjs)


#----------------- Other ---------------------------
depends:
	touch makedepend
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc
	$(MAKE) -C interfaces depends
	$(MAKE) -C spiro depends

-include makedepend


#------- Testing Stuff ----------------

#test.o: $(coreincludes) panuser.h pancontroller.h test.h test.cc


#---------- clean up and go home -----------------

# Like clean, but also remove logs of any previous install
dist-clean: clean
	rm -f install.log

.PHONY: clean docs interfaces spiro hidegarbage unhidegarbage
clean:
	rm -f *.o *.a *.so
	$(MAKE) -C interfaces clean
	$(MAKE) -C spiro clean

