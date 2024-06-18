########################################
##############           ###############
#############  Laxinput   ##############
##############           ###############
########################################


###Set LAXDIR to where the Laxkit headers are:
LAXDIR= ../lax





################ You shouldn't need to set anything below here ############################


###Include options from configure
include ../Makefile-toinclude


OPTIONALLIBS=

ifeq ($(LAX_USES_SQLITE), yes)
OPTIONALLIBS= -lsqlite3
endif



LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)



LD=g++
LDFLAGS= -L$(LAXDIR) -L$(LAXIDIR) -llaxkit -llaxinterfaces `cups-config --libs` `pkg-config --libs ../laxkit.pc` $(EXTRA_LAX_LIBS)
#LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lcairo `pkg-config --libs freetype2` $(OPTIONALLIBS) $(EXTRA_LAX_LIBS) -lfontconfig -lXext -lXi -lXft -lXrandr -lssl -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(DEBUGFLAGS) ${EXTRA_LAX_CFLAGS} -I$(LAXDIR)/.. -std=c++11 `pkg-config --cflags ../laxkit.pc`

	


laxinput: lax laxinput.o
	echo "extra libs: $(EXTRA_LIBS)"
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

#$(LD) $(LDFLAGS) $@.o -llaxkit -o $@

depends:
	touch makedepend
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

#-----------------------------------------------


lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXDIR)/interfaces && $(MAKE)



#-----------------------------------------------

.PHONY: clean all install
clean:
	rm -f laxinput *.o

