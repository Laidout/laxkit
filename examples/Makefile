#### Laxkit Examples/Makefile


LAXDIR= ../lax
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)

LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lcairo -lfontconfig -lpng -lcups `imlib2-config --libs` -lXi -lXft -lcrypto -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g -gdwarf-2
CPPFLAGS= $(DEBUGFLAGS) -I. -I.. -I$(LAXDIR)/ -I../include -I../src -I/usr/include/freetype2



	

examples= \
	laxhello \
	interfs


all: laxhello interfs timers


interfs: lax laxinterface interfs.o
	$(LD) $(LDFLAGS) interfs.o -llaxinterfaces -llaxkit -o $@

interfs2: lax laxinterface interfs2.o
	$(LD) $(LDFLAGS) interfs2.o normalinterface.o joininterface.o -llaxinterfaces -llaxkit -o $@

vfill: lax laxinterface vfill.o
	$(LD) $(LDFLAGS) vfill.o  -llaxinterfaces -llaxkit -o $@

#vfill.o: vfill.cc /home/tom/other/p/github/laxkit/lax/interfaces/engravermapping.cc /home/tom/other/p/github/laxkit/lax/interfaces/RESEARCH/blur.cc

cairotest: lax laxinterface cairotest.o
	$(LD) $(LDFLAGS) $@.o  -llaxinterfaces -llaxkit -o $@

test: lax laxinterface test.o
	$(LD) $(LDFLAGS) $@.o  -llaxinterfaces -llaxkit -o $@


laxhello: lax laxhello.o
	$(LD) $(LDFLAGS) $@.o -llaxkit -o $@

thingpreviewer: lax thingpreviewer.o
	$(LD) $(LDFLAGS) $@.o -llaxkit -o $@

timers: lax timers.o
	$(LD) $(LDFLAGS) $@.o -llaxkit -o $@

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
	rm -f x *.o
	rm -f $(examples)
