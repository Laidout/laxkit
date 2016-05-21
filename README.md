
THE LAXKIT
---------------
Version 0.0.8.1  
Released under the LGPL  
http://github.com/tomlechner/laxkit  
http://laxkit.sourceforge.net  


WHAT IS IT
----------
The Laxkit is a C++ gui toolkit currently in the form of an Xlib wrapper.
It has a number of user interfaces for manipulating various common two 
dimensional vector art objects like bezier curves with variable widths,
gradients, and meshes.

The main driving force behind Laxkit development is as a windowing
backend for the desktop publishing program Laidout (http://www.laidout.org).

The goal of the kit is to make the creation of art related programs
easy, by providing basic, but adaptible and efficient interfaces that can
be used as building blocks for more complex interfaces.
Examples of efficiency would be having a lot of configurability in shortcuts,
using combinations of key modifiers during mouse actions to change scroll 
speeds, and providing draggable visual elements to change drawable
object properties instead of low level input boxes off to the side.

There is copius documentation of the source accessible through doxygen by 
running 'make docs'.

Another partially implemented goal of the Laxkit is to provide an interface
kit supporting multi-pointer, multi-keyboard, and multi-touch surfaces.


COMPILING RELEASES (NOT GIT)
----------------------------
To compile development git source, see the next section.

You will need the development files for:  
   Imlib2, harfbuzz, freetype2, fontconfig, cairo, x11, ssl, cups, and optionally sqlite3
 
On Debian systems, you can get them with this command:

    apt-get install g++ pkg-config libpng12-dev libreadline-dev libx11-dev libxext-dev libxi-dev libxft-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype6-dev libssl-dev xutils-dev libcairo2-dev libharfbuzz-dev libsqlite3-dev

On Fedora, it looks more like this:

    sudo dnf install -y cairo-devel cups-devel fontconfig-devel ftgl-devel glibc-headers harfbuzz-devel imlib2-devel lcms-devel libpng-devel libX11-devel libXext-devel libXft-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel openssl-devel readline-devel sqlite-devel xorg-x11-proto-devel zlib-devel GraphicsMagick-c++-devel libstdc++-devel freetype-devel imake


Now simply do:

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
you will need git installed, as well as all the extra packages listed above.
Then do:

    git clone http://github.com/tomlechner/laxkit.git laxkit-git
    
    cd laxkit-git
    ./configure
    make depends
    make
    make icons  ##<- this requires you have Inkscape installed
    make docs   ##<- optional

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

