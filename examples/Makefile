#### Laxkit Examples/Makefile


LAXDIR= ../lax
LAXIDIR=$(LAXDIR)/interfaces
LAXOBJDIR=$(LAXDIR)

LD=g++
LDFLAGS= -L/usr/X11R6/lib -lX11 -lm -lcairo -lpng -lcups `imlib2-config --libs` -lXi -lXft -lcrypto -L$(LAXDIR) -L$(LAXIDIR)
DEBUGFLAGS= -g
CPPFLAGS= $(DEBUGFLAGS) -I. -I.. -I$(LAXDIR)/ -I../include -I../src -I/usr/include/freetype2



	

examples= \
	laxhello \
	interfs


all: laxhello interfs


interfs: lax laxinterface interfs.o
	$(LD) $(LDFLAGS) interfs.o -llaxinterfaces -llaxkit -o $@

cairotest: lax laxinterface cairotest.o
	$(LD) $(LDFLAGS) $@.o  -llaxinterfaces -llaxkit -o $@

test: lax laxinterface test.o
	$(LD) $(LDFLAGS) $@.o  -llaxinterfaces -llaxkit -o $@


laxhello: lax laxhello.o
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
