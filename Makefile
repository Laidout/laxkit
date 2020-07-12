################################################
##############                   ###############
#############   Laxkit Makefile   ##############
##############                   ###############
################################################
#
# When installing include headers, make sure to install into
# something like: lax-version/lax/*.h. This makes it easy for users to
# do g++ -I/usr/local/include/lax-version for including lax/whatever.
# Allows multiple versions of the kit to be installed without serious
# conflicts.
#
#
# to make shared library,
# objects must be compiled without -g and:
# 	g++ -fPIC -c blah.cc -o blah.o
# and linked thus:
# 	g++ -shared -fPIC -o blah.so blah.o ...


 # Where to install stuff, currently:
 #   prefix/include
 #   prefix/lib
#PREFIX=/usr/local
#PREFIX=testinstall
include Makefile-toinclude

## Comment out the next line to disable anything to do with Imlib2.
EXTRAS= -DLAX_USES_IMLIB

## Comment out the next line to not compile in debugging info
DEBUGFLAGS= -g




 # the libs get installed here
LIBDIR=$(PREFIX)/lib
BINDIR=$(PREFIX)/bin

 # header files get installed in this directory like this: includedir/laxkit/version/lax/*.h
INCLUDEDIR=$(PREFIX)/include
LAX_INCLUDE_DIR=$(INCLUDEDIR)/laxkit/$(LAXKITVERSION)/lax

 # documentation goes here, like: /usr/share/doc/laxkit/lax-0.0.4
DOCDIR=$(PREFIX)/share/laxkit/$(LAXKITVERSION)/doc


### If you want to be sure that an install does not clobber anything that exists
### already, then uncomment the line with the '--backup=t' and comment out the other.
#INSTALL=install --backup=t 
INSTALL=install





LAXDIR= lax
LAXOBJDIR=objs



#this compiles the core laxkit only, not laxinput
almostall: touchdepends lax interfaces
	@echo "  -----------Done!-------------" 


all: touchdepends lax interfaces laxinput
	@echo "  -----------Done!-------------" 


install-docs:
	echo "---- installing docs `date` -----" >> install.log
	$(INSTALL) -d $(DOCDIR)
	$(INSTALL) -d $(DOCDIR)/html
	$(INSTALL) -D -m644 docs/style.css $(DOCDIR)
	$(INSTALL) -D -m644 docs/*.html $(DOCDIR)
	$(INSTALL) -D -m644 docs/*.png $(DOCDIR)
	$(INSTALL) -D -m644 docs/*.jpg $(DOCDIR)
	$(INSTALL) -D -m644 docs/html/* $(DOCDIR)/html
	ls $(DOCDIR)/*  >> install.log
	ls $(DOCDIR)/html/* >> install.log

icons:
	cd lax/icons && make

install: lax interfaces laxinput
	echo "---- installing `date` -----" >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/liblaxkit.a $(LIBDIR)/liblaxkit.a && \
		echo '$(LIBDIR)/liblaxkit.a' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/liblaxkit.so.$(LAXKITVERSION) $(LIBDIR)/liblaxkit.so.$(LAXKITVERSION) && \
		echo '$(LIBDIR)/liblaxkit.so.$(LAXKITVERSION)' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/interfaces/liblaxinterfaces.a $(LIBDIR)/liblaxinterfaces.a && \
		echo '$(LIBDIR)/liblaxinterfaces.a' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/interfaces/liblaxinterfaces.so.$(LAXKITVERSION) $(LIBDIR)/liblaxinterfaces.so.$(LAXKITVERSION) && \
		echo '$(LIBDIR)/liblaxinterfaces.so.$(LAXKITVERSION)' >> install.log
	$(INSTALL) -d $(LAX_INCLUDE_DIR)/lax/interfaces
	$(INSTALL) -D -m644 $(LAXDIR)/lists.cc $(LAX_INCLUDE_DIR)/lax/lists.cc && \
		echo '$(LAX_INCLUDE_DIR)/lax/lists.cc' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/refptrstack.cc $(LAX_INCLUDE_DIR)/lax/refptrstack.cc && \
		echo '$(LAX_INCLUDE_DIR)/lax/refptrstack.cc' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/*.h $(LAX_INCLUDE_DIR)/lax
	$(INSTALL) -D -m644 $(LAXDIR)/interfaces/*.h $(LAX_INCLUDE_DIR)/lax/interfaces
	ls $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/*.h $(LAX_INCLUDE_DIR)/lax/interfaces/*.h >> install.log
	$(INSTALL) -D -m711 laxinput/laxinput $(BINDIR)/laxinput && \
		echo '$(BINDIR)/laxinput' >> install.log


uninstall: 
	rm -vf $(LIBDIR)/liblaxkit.a
	rm -vf $(LIBDIR)/liblaxinterfaces.a
	rm -v -rf $(INCLUDEDIR)/lax-$(LAXKITVERSION)

docs:
	cd docs && doxygen

lax:
	cd lax && $(MAKE) all

interfaces:
	cd lax/interfaces && $(MAKE) all

laxinput: lax interfaces
	cd laxinput/ && $(MAKE)

touchdepends:
	touch lax/makedepend
	touch lax/interfaces/makedepend

depends:
	touch lax/makedepend
	touch lax/interfaces/makedepend
	cd lax && $(MAKE) depends
	cd lax/interfaces && $(MAKE) depends


#link debian to deb if not there.. Debian guidelines say don't put 
#a "debian" directory in upstream sources by default.
deb: touchdepends
	if [ ! -e debian ] ; then ln -s deb debian; fi
	dpkg-buildpackage -rfakeroot


#---------- garbage be-gone -----------------
# This basically changes in *.cc all occurrences of 
# '(whitespace)DBG' to '(same ws)//DBG'.
hidegarbage:
	cd lax && $(MAKE) hidegarbage

# This changes in *.cc all occurrences of 
# '(whitespace)//DBG' to '(same ws)DBG'.
unhidegarbage:
	cd lax && $(MAKE) unhidegarbage


#---------- clean up and go home -----------------

# Like clean, but also remove logs of any previous install
dist-clean: clean 
	rm -f install.log
	rm -rf docs/html
	rm -rf Makefile-toinclude config.log lax/configured.h lax/version.h

.PHONY: clean docs lax interfaces depends unhidegarbage hidegarbage laxinput icons deb
clean:
	cd lax && rm -f *.o *.a *.so *.so.*
	cd lax/interfaces && rm -f *.o *.a *.so *.so.*

