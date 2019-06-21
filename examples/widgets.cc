/**************** widgets.cc *****************/



#include <lax/messagebar.h>
#include <lax/scrolledwindow.h>
#include <lax/language.h>


#include <iostream>
using namespace std;
using namespace Laxkit;


int main(int argc,char **argv)
{
	anXApp app;
	app.SetTheme("Dark"); //built in are Dark, or Light (default)
	app.init(argc,argv);


	ScrolledWindow *scrolling = new ScrolledWindow(nullptr, "ScrollTest",_("ScrollTest"),
									ANXWIN_ESCAPABLE | SW_RIGHT | SW_BOTTOM | SW_MOVE_WINDOW,
									0,0,400,600,0, nullptr);

	//RowFrame *rowframe = new RowFrame();
	MessageBar *messagebar = new MessageBar(NULL,"mesbar","Read me!",MB_MOVE, 0,0,0,0,1,
			"111111111\n"
			"222222222\n"
			"333333333\n"
			"444444444\n"
			"555555555\n"
			"666666666\n"
			"777777777\n"
			"888888888\n"
			"999999999\n"
			"000000000\n"
			"111111111\n"
			"222222222\n"
			"333333333\n"
			"444444444\n"
			"555555555\n"
			"666666666\n"
			"777777777\n"
			"888888888\n"
			"999999999\n"
			"000000000\n"
			);

	scrolling->UseThisWindow(messagebar);

	app.addwindow(scrolling);
	

	cout <<"------Done adding initial windows in main() -------\n";
	app.run();

	cout <<"------ App Close:  -------\n";
	app.close();
	
	cout <<"------ Bye! -------\n";
	return 0;
}

