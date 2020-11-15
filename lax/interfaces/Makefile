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
	svgcoord.o \
	selection.o \
	aninterface.o \
	interfacemanager.o \
	viewportwindow.o \
	viewerwindow.o \
	viewportwithstack.o \
	rectinterface.o \
	objectinterface.o \
	gradientinterface.o \
	imageinterface.o \
	patchinterface.o\
	colorpatchinterface.o \
	imagepatchinterface.o\
	lineprofile.o \
	pathinterface.o \
	shapebrush.o \
	freehandinterface.o \
	captioninterface.o \
	engraverfilldata.o \
	engraverfillinterface.o \
	delaunayinterface.o \
	ellipseinterface.o \
	pressuremapinterface.o \
	curvemapinterface.o \
	curvewindow.o \
	showkeysinterface.o \
	characterinterface.o \
	textstreaminterface.o \
	textonpathinterface.o \
	texttopath.o \
	groupdata.o \
	perspectiveinterface.o \
	stackpartinterface.o \
	simplepathinterface.o \

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

