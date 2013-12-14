--------------Laxinput-----------------
A simple gui based XInput2 configurator

Written by Tom Lechner, 2011
Currently released under the GPL V3.


USE AT YOUR OWN RISK!!!!

Simply click and drag devices to reposition them.
Drop on the "Floating Devices" block to float.
Drag on the "Add new..." block to create a new master device pair from that device.
Drag a master device to another device area to merge.

Doing control-s will save whatever is the current configuration.
Also try "laxinput -h" for a list of various command line options.

This software is not idiot proof, i.e. it does not prevent you from floating all your mice,
for instance, which would prevent you from using any mice at all.

Please note that this program is not even close to a complete wrapper around xinput.
It is currently only for quickly remapping the X device hierarchy.


COMPILING
---------
You will need the Laxkit (http://laxkit.sourceforge.net). Set the path to the Laxkit headers
if necessary in the Makefile. You will also need makedepend, which on Debian systems is in xutils-dev.

If you need to download the Laxkit fresh, then do this:
First ensure that all Laxkit dependecies are present. On debian systems, you can do this as root:

apt-get install libpng12-dev libx11-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype-dev libssl-dev


Now download and compile the Laxkit (don't have to be root):

 > svn co https://laxkit.svn.sourceforge.net/svnroot/laxkit/laxkit/trunk laxkit-svn
 > cd laxkit-svn
 > ./configure
 > make depends
 > make
 > cd laxinput

Now do:

 > touch makedepend
 > make depends
 > make

And that's it! If it actually worked, you should be able to simply run laxinput.

