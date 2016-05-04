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
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lcairo `imlib2-config --libs` `freetype-config --libs` $(OPTIONALLIBS) -lfontconfig -lXext -lXi -lXft -lssl -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -Wall
CPPFLAGS= $(DEBUGFLAGS) -I$(LAXDIR)/.. `freetype-config --cflags`



	


laxinput: lax laxinput.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

#$(LD) $(LDFLAGS) $@.o -llaxkit -o $@

depends:
	touch makedepend
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc

#-----------------------------------------------


lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXDIR)/interfaces && $(MAKE)



#-----------------------------------------------

.PHONY: clean all install
clean:
	rm -f laxinput *.o

