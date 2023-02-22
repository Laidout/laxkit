################################################
#######                                  #######
######   Laxkit/lax/interfaces Makefile   ######
#######                                  #######
################################################

include ../../Makefile-toinclude

LAXDIR=..
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm 

OPTIMIZATION=
DEBUGFLAGS= -g -gdwarf-2

## use the second one to make .so
#FPIC=
FPIC= -fPIC

CPPFLAGS= $(OPTIMIZATION) $(EXTRA_LAX_CFLAGS) $(DEBUGFLAGS) $(FPIC) -I$(LAXDIR)/.. `pkg-config --cflags freetype2` -Wall


objs= \
	linestyle.o \
	fillstyle.o \
	somedata.o \
	somedataref.o \
	somedatafactory.o \
	coordinate.o \
	selection.o \
	aninterface.o \
	interfacemanager.o \
	viewportwindow.o \
	viewerwindow.o \
	viewportwithstack.o \
	beznet.o \
	beznetinterface.o \
	captioninterface.o \
	characterinterface.o \
	colorpatchinterface.o \
	curvemapinterface.o \
	curvewindow.o \
	delaunayinterface.o \
	ellipseinterface.o \
	engraverfilldata.o \
	engraverfillinterface.o \
	freehandinterface.o \
	gradientinterface.o \
	groupdata.o \
	imageinterface.o \
	imagepatchinterface.o\
	lineprofile.o \
	mirrorinterface.o \
	objectinterface.o \
	patchinterface.o\
	pathinterface.o \
	perspectiveinterface.o \
	pressuremapinterface.o \
	rectinterface.o \
	shapebrush.o \
	showkeysinterface.o \
	simplepathinterface.o \
	stackpartinterface.o \
	svgcoord.o \
	textonpathinterface.o \
	textstreaminterface.o \
	texttopath.o \

#	linesinterface.o \
#	measureinterface.o \


#----------------- Static lib ---------------------------
liblaxinterfaces.a: $(objs)
	ar cr $@ $(objs)
	@echo "  -----------Done with interfaces!-------------" 


#----------------- Dynamic lib ---------------------------
#these .so makers require that all object files be compiled with -fPIC,
#which ultimately adds to the overall size of the static archive
liblaxinterfaces.so.$(LAXKITVERSION): $(objs)
	$(LD) -shared -fPIC  $(objs) -o $@


all: liblaxinterfaces.a liblaxinterfaces.so.$(LAXKITVERSION)

compileonly: $(objs)

#------------------- Other --------------------------

docs:
	cd ../../docs/ && doxygen

depends:
	touch makedepend
	../../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc







.PHONY: clean docs hidegarbage unhidegarbage
clean:
	rm -f *.o *.a *.so

