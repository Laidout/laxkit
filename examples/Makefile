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
LDFLAGS= -L/usr/X11R6/lib -L$(LAXDIR) -L$(LAXIDIR) -lX11 -lm -lcairo `pkg-config --libs freetype2` `imlib2-config --libs` -lfontconfig -lXext -lXi -lXft -lXrandr -lcrypto -lharfbuzz $(OPTIONALLIBS) $(EXTRA_LAX_LIBS)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(DEBUGFLAGS) -I$(LAXDIR)/.. -std=c++11 `pkg-config --cflags freetype2`



examples= \
	laxhello \
	interfs \
	themeeditor \
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

themeeditor: lax laxinterface themeeditor.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

test: lax laxinterface test.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

test2: lax laxinterface test2.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@


laxhello: lax laxinterface laxhello.cc laxhello.o
	$(LD) $@.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

thingpreviewer: lax thingpreviewer.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

timers: lax timers.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

stopwatch: lax laxinterface stopwatch.o
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
