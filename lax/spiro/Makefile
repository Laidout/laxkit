###########################################
#######                             #######
######   Laxkit/lax/spiro Makefile   ######
#######                             #######
###########################################

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
	  rspline.o \
	  spiro.o 


all: $(objs)

#------------------- Other --------------------------

docs:
	cd ../../docs/ && doxygen

depends:
	touch makedepend
	../../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

-include makedepend

hidegarbage:
	../hidegarbage *.cc

unhidegarbage:
	../hidegarbage -0 *.cc







.PHONY: clean docs hidegarbage unhidegarbage
clean:
	rm -f *.o *.a *.so

