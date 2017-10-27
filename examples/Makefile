##############################################
##############                 ###############
#############  Laxkit/examples  ##############
##############                 ###############
##############################################

include ../Makefile-toinclude

LAXDIR= ../lax
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)


OPTIONALLIBS=

ifeq ($(LAX_USES_SQLITE), yes)
OPTIONALLIBS= -lsqlite3
endif



LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lcairo `freetype-config --libs` `imlib2-config --libs` $(OPTIONALLIBS) -lfontconfig -lXext -lXi -lXft -lcrypto -lharfbuzz -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(DEBUGFLAGS) -I$(LAXDIR)/.. `freetype-config --cflags`



examples= \
	laxhello \
	interfs \
	timers


all: laxhello interfs timers


interfs: lax laxinterface interfs.o
	$(LD) interfs.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

interfs2: lax laxinterface interfs2.o
	$(LD) interfs2.o normalinterface.o joininterface.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

vfill: lax laxinterface vfill.o
	$(LD) vfill.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

#vfill.o: vfill.cc /home/tom/other/p/github/laxkit/lax/interfaces/engravermapping.cc /home/tom/other/p/github/laxkit/lax/interfaces/RESEARCH/blur.cc

cairotest: lax laxinterface cairotest.o
	$(LD) $@.o  -llaxinterfaces -llaxkit -lharfbuzz $(LDFLAGS) -o $@

testevents: lax laxinterface testevents.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

themes: lax laxinterface themes.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

test: lax laxinterface test.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@


laxhello: lax laxhello.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

thingpreviewer: lax thingpreviewer.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

timers: lax timers.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

stationrotater: lax stationrotater.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

depends:
	touch makedepend
	makedepend -fmakedepend -I$(LAXDIR)/.. -Y *.cc
	#cd interfaces && $(MAKE) depends

#-----------------------------------------------


lax:
	cd $(LAXDIR) && $(MAKE)

laxinterface:
	cd $(LAXDIR)/interfaces && $(MAKE)



#-----------------------------------------------

.PHONY: clean all
clean:
	rm -f *.o
	rm -f $(examples)
