### lax/interfaces/Makefile

include ../../Makefile-toinclude

LAXDIR=..
LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm 

OPTIMIZATION=
DEBUGFLAGS= -g

## use the second one to make .so
#FPIC=
FPIC= -fPIC

CPPFLAGS= $(OPTIMIZATION) $(EXTRAS) $(DEBUGFLAGS) $(FPIC) -I. -I.. -I$(LAXDIR)/.. -I/usr/include/freetype2 -Wall


objs= \
	linestyle.o \
	fillstyle.o \
	dumpcontext.o \
	somedata.o \
	somedataref.o \
	somedatafactory.o \
	coordinate.o \
	svgcoord.o \
	aninterface.o \
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
	pathinterface.o \
	freehandinterface.o \
	captioninterface.o \
	engraverfillinterface.o \

#	linesinterface.o \
#	ellipseinterface.o \
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
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc

include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc







.PHONY: clean docs hidegarbage unhidegarbage
clean:
	rm -f *.o *.a *.so

