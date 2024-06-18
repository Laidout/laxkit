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
#LDFLAGS= -L$(LAXDIR) -L$(LAXIDIR) -llaxkit -llaxinterfaces `cups-config --libs` `pkg-config --libs ../laxkit.pc` $(EXTRA_LAX_LIBS)
LDFLAGS= -L$(LAXDIR) -L$(LAXIDIR) `cups-config --libs` `pkg-config --libs ../laxkit.pc` $(EXTRA_LAX_LIBS)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(DEBUGFLAGS) ${EXTRA_LAX_CFLAGS} -I$(LAXDIR)/.. -std=c++11 `pkg-config --cflags ../laxkit.pc`



examples= \
	laxhello \
	interfs \
	themeeditor \
	spirocurves \
	timers \
	widgets


all: $(examples)


cairotest: lax cairotest.o
	$(LD) $@.o $(LDFLAGS) -o $@

interfs: lax interfs.o
	$(LD) interfs.o $(LDFLAGS) -o $@

interfs2: lax interfs2.o
	$(LD) interfs2.o normalinterface.o joininterface.o -llaxinterfaces -llaxkit $(LDFLAGS) -o $@

attxml: lax attxml.cc attxml.o
	$(LD) $@.o $(LDFLAGS) -o $@

laxhello: lax laxhello.cc laxhello.o
	$(LD) $@.o $(LDFLAGS) -o $@

widgets: lax widgets.cc widgets.o
	$(LD) $@.o $(LDFLAGS) -o $@

panel: lax panel.o
	$(LD) $@.o $(LDFLAGS) -o $@

spirocurves: lax spirocurves.o
	$(LD) $@.o $(LDFLAGS) -o $@

stopwatch: lax stopwatch.o
	$(LD) $@.o $(LDFLAGS) -o $@

stationrotater: lax stationrotater.o
	$(LD) $@.o $(LDFLAGS) -o $@

test: lax test.o test.cc
	$(LD) $@.o $(LDFLAGS) -o $@

test2: lax test2.o
	$(LD) $@.o $(LDFLAGS) -o $@

testevents: lax testevents.o
	$(LD) $@.o $(LDFLAGS) -o $@

themeeditor: lax themeeditor.o themeeditor.cc
	$(LD) $@.o $(LDFLAGS) -o $@

thingpreviewer: lax thingpreviewer.o
	$(LD) $@.o $(LDFLAGS) -o $@

timers: lax timers.o
	$(LD) $@.o $(LDFLAGS) -o $@

vfill: lax vfill.o
	$(LD) vfill.o $(LDFLAGS) -o $@


depends:
	touch makedepend
	../utils/makedependencies -fmakedepend -I$(LAXDIR)/.. *.cc

#-----------------------------------------------


lax:
	cd $(LAXDIR) && $(MAKE)




#-----------------------------------------------

.PHONY: clean all
clean:
	rm -f *.o
	rm -f $(examples)
