/**************** laxhello.cc *****************/

// After installing the Laxkit, compile this program like this:
//
// g++ laxhello.cc -I/usr/include/freetype2 -L/usr/X11R6/lib -lX11 -lXft -lm  -lpng \
//         -lcups -llaxkit -o laxhello


#include <lax/messagebar.h>

#include <iostream>
using namespace std;
using namespace Laxkit;

int main(int argc,char **argv)
{
	anXApp app;
	app.SetTheme("Dark"); //built in are Dark, or Light (default)
	app.init(argc,argv);

	app.addwindow(new MessageBar(NULL,"mesbar","Read me!",ANXWIN_ESCAPABLE|MB_MOVE, 100,100,200,100,0, "Blah!"));
	

	cout <<"------Done adding initial windows in main() -------\n";
	app.run();

	cout <<"------ App Close:  -------\n";
	app.close();
	
	cout <<"------ Bye! -------\n";
	return 0;
}

