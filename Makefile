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

 # header files get installed in this directory like this: includedir/lax-version/lax/*.h
INCLUDEDIR=$(PREFIX)/include

 # documentation goes here, like: /usr/share/doc/laxkit/lax-0.0.4
DOCDIR=$(PREFIX)/share/doc/laxkit/laxkit-$(LAXKITVERSION)


### If you want to be sure that an install does not clobber anything that exists
### already, then uncomment the line with the '--backup=t' and comment out the other.
#INSTALL=install --backup=t 
INSTALL=install





LAXDIR= lax
LAXOBJDIR=objs





all: lax interfaces laxinput
	@echo "  -----------Done!-------------" 


#this compiles the core laxkit only, not interfaces
almostall: lax
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
	$(INSTALL) -d $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/interfaces
	$(INSTALL) -D -m644 $(LAXDIR)/lists.cc $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/lists.cc && \
		echo '$(LIBDIR)/lists.cc' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/refptrstack.cc $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/refptrstack.cc && \
		echo '$(LIBDIR)/refptrstack.cc' >> install.log
	$(INSTALL) -D -m644 $(LAXDIR)/*.h $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax
	$(INSTALL) -D -m644 $(LAXDIR)/interfaces/*.h $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/interfaces
	ls $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/*.h $(INCLUDEDIR)/lax-$(LAXKITVERSION)/lax/interfaces/*.h >> install.log
	$(INSTALL) -D -m711 laxinput/laxinput $(BINDIR)/laxinput && \
		echo '$(BINDIR)/laxinput' >> install.log

#	$(INSTALL) -D -m644 $(LAXDIR)/liblaxatts.a $(LIBDIR)/liblaxatts.a && \
#		echo '$(LIBDIR)/liblaxatts.a' >> install.log

uninstall: 
	rm -vf $(LIBDIR)/liblaxkit.a
	rm -vf $(LIBDIR)/liblaxatts.a
	rm -vf $(LIBDIR)/liblaxinterfaces.a
	rm -v -rf $(INCLUDEDIR)/lax-$(LAXKITVERSION)

#this mkdir stuff is kind of an ugly hack so that when I do 'svn status' the html dir, which
#has a million files will not show up
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

.PHONY: clean docs lax interfaces depends unhidegarbage hidegarbage laxinput
clean:
	cd lax && rm -f *.o *.a *.so
	cd lax/interfaces && rm -f *.o *.a *.so

