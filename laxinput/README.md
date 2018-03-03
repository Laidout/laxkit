Laxinput
--------
A simple gui based XInput2 configurator

Written by Tom Lechner, 2011  
Currently released under the GPL V3.


USE AT YOUR OWN RISK!!!!

Simply click and drag devices to reposition them.  
Drop on the "Floating Devices" block to float.  
Drag on the "Add new..." block to create a new master device pair from that device.  
Drag a master device to another device area to merge.  

Doing control-s will save whatever is the current configuration.
Also try `laxinput -h` for a list of various command line options.

This software is not idiot proof, i.e. it does not prevent you from floating all your mice,
for instance, which would prevent you from using any mice at all.

Please note that this program is not even close to a complete wrapper around xinput.
It is currently only for quickly remapping the X device hierarchy.


Compiling
---------
You will need the Laxkit (https://github.com/Laidout/laxkit). Set the path to the Laxkit headers
if necessary in the Makefile. You will also need makedepend, which on Debian systems is in xutils-dev.

If you need to download the Laxkit fresh, then do this:
First ensure that all Laxkit dependecies are present. On debian systems, you can do this as root:

    apt-get install g++ pkg-config libpng12-dev libreadline-dev libx11-dev libxext-dev libxi-dev libxft-dev libcups2-dev libimlib2-dev libfontconfig-dev libfreetype6-dev libssl-dev xutils-dev libcairo2-dev libharfbuzz-dev libsqlite3-dev



Now download and compile the Laxkit (don't have to be root):

    git clone http://github.com/Laidout/laxkit  
    cd laxkit
    ./configure
    make depends
    make
    cd laxinput

Now do:

    touch makedepend
    make depends
    make

And that's it! If it actually worked, you should be able to simply run ./laxinput.

