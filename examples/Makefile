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
LDFLAGS= -L$(LAXDIR) -L$(LAXIDIR) -llaxkit -llaxinterfaces `cups-config --libs` `pkg-config --libs ../laxkit.pc` $(EXTRA_LAX_LIBS)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(DEBUGFLAGS) -I$(LAXDIR)/.. -std=c++11 `pkg-config --cflags ../laxkit.pc`



examples= \
	laxhello \
	interfs \
	themeeditor \
	spirocurves \
	timers


all: $(examples)


cairotest: lax laxinterface cairotest.o
	$(LD) $@.o  -llaxinterfaces -llaxkit -lharfbuzz $(LDFLAGS) -o $@

interfs: lax laxinterface interfs.o
	$(LD) interfs.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

interfs2: lax laxinterface interfs2.o
	$(LD) interfs2.o normalinterface.o joininterface.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

laxhello: lax laxinterface laxhello.cc laxhello.o
	$(LD) $@.o  $(LDFLAGS) -o $@
	#$(LD) $@.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

widgets: lax laxinterface widgets.cc widgets.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

panel: lax laxinterface panel.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

spirocurves: lax laxinterface spirocurves.o
	$(LD) $@.o -llaxinterfaces -llaxkit  $(LDFLAGS) -o $@

stopwatch: lax laxinterface stopwatch.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

stationrotater: lax stationrotater.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

test: lax laxinterface test.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

test2: lax laxinterface test2.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

testevents: lax laxinterface testevents.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

themeeditor: lax laxinterface themeeditor.o
	$(LD) $@.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

thingpreviewer: lax thingpreviewer.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

timers: lax timers.o
	$(LD) $@.o -llaxkit $(LDFLAGS) -o $@

vfill: lax laxinterface vfill.o
	$(LD) vfill.o  -llaxinterfaces -llaxkit $(LDFLAGS) -o $@


depends:
	touch makedepend
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc
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
