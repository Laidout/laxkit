/******** examples/simplepathtool.cc ***********/

//
// Initializers are below for different tools. Comment or uncomment to experiment
// with specific ones!
//
//


//#include <lax/interfaces/freehandinterface.h>
//#include <lax/interfaces/ellipseinterface.h>
//#include <lax/interfaces/colorpatchinterface.h>
//#include <lax/interfaces/gradientinterface.h>
//#include <lax/interfaces/imageinterface.h>
//#include <lax/interfaces/linestyle.h>
//#include <lax/interfaces/pathinterface.h>
//#include <lax/interfaces/rectinterface.h>
//#include <lax/interfaces/objectinterface.h>
//#include <lax/interfaces/captioninterface.h>
//#include <lax/interfaces/engraverfillinterface.h>
//#include <lax/interfaces/pressuremapinterface.h>
#include <lax/units.h>

#include <lax/interfaces/viewerwindow.h>
#include <lax/interfaces/viewportwithstack.h>
#include <lax/interfaces/simplepathinterface.h>

using namespace Laxkit;
using namespace LaxInterfaces;


#include <iostream>
using namespace std;



//----------------------------- main() ------------------------------
int main(int argc,char **argv)
{
	ColorManager *colorManager = ColorManager::GetDefault();
	colorManager->AddSystem(Create_sRGB_System(true), true);

	anXApp app;
	app.init(argc,argv);

	 // owner and dp get assigned in PathInterface constructor
	ViewportWithStack vp(NULL,"vw viewportwithstack","viewportwithstack",
								  ANXWIN_HOVER_FOCUS|VIEWPORT_BACK_BUFFER|VIEWPORT_ROTATABLE,
								  0,0,0,0,0
								 );
	vp.draw_axes = false;
	ViewerWindow *viewer = new ViewerWindow(nullptr, "Viewer","Viewer",
			ANXWIN_CENTER | ANXWIN_ESCAPABLE,
			100,100, 600,500, 5,
			&vp
		);


	 //add to selection of tools
	int i=100, current=100;
	viewer->AddTool(new    SimplePathInterface(nullptr, i++, nullptr), 1,0);
	//viewer->AddTool(new    CaptionInterface(i++,NULL,"New text\nline 2\n  spaced line 3"), 1,0);
	//viewer->AddTool(new     ObjectInterface(i++,NULL), 1,0);
	//viewer->AddTool(new   GradientInterface(i++,NULL), 1,0); current = i;
	//viewer->AddTool(new      ImageInterface(i++,NULL), 1,0);
	//viewer->AddTool(new      PatchInterface(i++,NULL), 1,0);
	//viewer->AddTool(new ColorPatchInterface(i++,NULL), 1,0);
	//viewer->AddTool(new       RectInterface(i++,NULL), 1,0);
	//viewer->AddTool(new       PathInterface(i++,NULL), 1,0);
	//viewer->AddTool(new   FreehandInterface(NULL, i++,NULL), 1,0); current=i-1;
	//viewer->AddTool(new   PressureMapInterface(NULL, i++,NULL), 1,0);
	//viewer->AddTool(new EngraverFillInterface(i++,NULL), 1,0);


	viewer->SelectTool(current);
	app.addwindow(viewer);

	
	app.run();
	cout <<"---------App Close--------------"<<endl;
	app.close();
	cout <<"---------Bye!--------------"<<endl;
	return 0;
}


