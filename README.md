
THE LAXKIT
---------------
Version 0.0.7.1  
Released under the LGPL  
http://github.com/tomlechner/laxkit  
http://laxkit.sourceforge.net  


WHAT IS IT
----------
The Laxkit is a C++ gui toolkit currently in the form of an Xlib wrapper.
It also has a number of user interfaces for manipulating various common two 
dimensional artsy types of objects like bezier curves and gradients.

The purpose of the kit is to make the creation of artsy types of programs
easier, and that makes creation of user interfaces very efficient by having
a lot of configurability in shortcuts (something not yet implemented!),
and various other usability enhancements.
For instance, when you have a large file list, pressing shift while 
twirling the wheel makes the list scroll faster and shift-control while
twirling makes scrolling go extra fast.

The main driving force behind Laxkit development is as a windowing
backend for the desktop publishing program Laidout (http://www.laidout.org).

There is copius documentation of the source accessible through doxygen by 
running 'make docs'.

Another partially implemented goal of the Laxkit is to provide an interface
kit supporting multi-pointer, multi-keyboard, and multi-touch surfaces.


COMPILING
---------
You will need the development files for (these are the debian packages):  
    libcupsys2-dev
    libimlib2-dev
    libx11-dev
    libxft-dev
    libxi-dev
    libpng12-dev
    libfontconfig-dev 
    libxext-dev
    libssl-dev
    xutils-dev
 
You can get them with this command (on debian systems):

    apt-get install g++ pkg-config libpng12-dev libx11-dev libxft-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype6-dev libssl-dev xutils-dev

Simply do:

    ./configure
    make
    make install

If you type `./configure --prefix=/your/own/install/path`, then the laxkit will get
installed in prefix/include, prefix/share/doc, prefix/lib, etc.
What files were installed are put into the file install.log.

By default, `make install` will plop down everything in /usr/local/include and 
/usr/local/lib. The end result is two libraries:

    liblaxkit.a          The laxkit core      
    liblaxinterfaces.a   The laxkit interfaces


COMPILING FROM GIT
------------------
The development version of the Laxkit is currently kept on github. To access,
you will need git installed. Then do:

    git clone http://github.com/tomlechner/laxkit.git laxkit-git
    
    cd laxkit-git
    ./configure
    make depends
    make
    make docs

By default, running the Laxkit directly from git code will pump out lots of debugging information 
to the terminal. To prevent this, do `make hidegarbage` before `make`.


CONTRIBUTING
------------
If you are interested in translating, please look in lax/po/README, which gives
a rough overview of how to go about that. The Laxkit uses gettext for translations.

Bug reports and reviews are welcome!
There is currently no dedicated mailing list, but since the Laxkit is very much tied
to Laidout, free to post any questions to the Laidout mailing list:
 http://lists.sourceforge.net/lists/listinfo/laidout-general


DOCUMENTATION
-------------
The Laxkit source code contains a lot of doxygen style documentation.
'make install' will not generate or install this documentation. 
You must do that yourself with:

    make docs

This uses doxygen to generate lots of docs in the docs/html directory. This assumes you
have the dot tool, a part of Graphviz, which generates collaboration diagrams of the classes,
which can be very handy. Also, there are some writeups in the documentation, particularly math 
related to bezier curves and patches, that need latex installed in order to be generated 
correctly. On Debian systems, as of Feb. 2010, you'll need the texlive-latex-base package.


AUTHORS
-------
Tom Lechner, http://tomlechner.com

