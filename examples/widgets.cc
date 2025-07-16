/**************** widgets.cc *****************/




#include <lax/laxutils.h>
#include <lax/transformmath.h>
#include <lax/language.h>

#include <lax/printdialog.h>
#include <lax/quickfileopen.h>
#include <lax/lineinput.h>
#include <lax/multilineedit.h>
#include <lax/fontdialog.h>
#include <lax/filedialog.h>
#include <lax/iconselector.h>
#include <lax/numslider.h>
#include <lax/scrolledwindow.h>
#include <lax/popupmenu.h>

#include <lax/interfaces/characterinterface.h>
#include <lax/interfaces/sliderinterface.h>
#include <lax/interfaces/gridselectinterface.h>
#include <lax/interfaces/interfacewindow.h>


#include <iostream>
using namespace std;


using namespace Laxkit;
using namespace LaxInterfaces;


int main(int argc,char **argv)
{
	anXApp app;
	app.SetTheme("Dark"); //built in are Dark, or Light (default)
	app.init(argc,argv);


	// If you get crashes trying to make new colors, probably ColorManager not initialized with any
	// color systems, so uncomment the following:
//	ColorManager *colorManager = new ColorManager();
//	colorManager->AddSystem(Create_sRGB_System(true), true);
//	ColorManager::SetDefault(colorManager);
//	colorManager->dec_count();


	//------------------- ScrolledWindow --------------------------

//	ScrolledWindow *scrolling = new ScrolledWindow(nullptr, "ScrollTest",_("ScrollTest"),
//									ANXWIN_ESCAPABLE | SW_RIGHT | SW_BOTTOM | SW_MOVE_WINDOW,
//									0,0,400,600,0, nullptr);
//
//	//RowFrame *rowframe = new RowFrame();
//	MessageBar *messagebar = new MessageBar(NULL,"mesbar","Read me!",MB_MOVE, 0,0,0,0,1,
//			"111111111\n"
//			"222222222\n"
//			"333333333\n"
//			"444444444\n"
//			"555555555\n"
//			"666666666\n"
//			"777777777\n"
//			"888888888\n"
//			"999999999\n"
//			"000000000\n"
//			"111111111\n"
//			"222222222\n"
//			"333333333\n"
//			"444444444\n"
//			"555555555\n"
//			"666666666\n"
//			"777777777\n"
//			"888888888\n"
//			"999999999\n"
//			"000000000\n"
//			);
//
//	scrolling->UseThisWindow(messagebar);
//
//	app.addwindow(scrolling);
	

	//------------------- LineInput --------------------------
	//app.addwindow(new LineInput(nullptr, "File", "File", ANXWIN_ESCAPABLE | ANXWIN_CENTER | LINP_FILE,  0,0,400,50, 1, nullptr, 0, nullptr, "Label", "text"));


	//------------------- MultiLineEdit --------------------------
	//app.addwindow(new MultiLineEdit(nullptr, "File", "File", ANXWIN_ESCAPABLE | ANXWIN_CENTER,  0,0,400,50, 1, nullptr, 0, nullptr,
	//			0, "One\nTwo\nThree"));


	//------------------- NumSlider --------------------------
	//app.addwindow(new NumSlider(nullptr, "NumSlider", nullptr, ANXWIN_ESCAPABLE | ANXWIN_CENTER | ItemSlider::EDITABLE,  0,0,400,50, 1, nullptr, 0, nullptr, "Slide", 0.0, 100.0, 0.0, .5));


	//------------------- PrintDialog --------------------------
	//app.addwindow(new PrintDialog(nullptr,"Print","Print",ANXWIN_ESCAPABLE|ANXWIN_CENTER, 0,0,1000,700,0, nullptr,0,nullptr, 0));

//	 	PrintDialog(anXWindow *parnt,const char *nname,const char *ntitle,unsigned long nstyle,
//			int xx,int yy,int ww,int hh,int brder,
//			anXWindow *prev,unsigned long nowner,const char *nsend,
//			const char *nfiletoprint=NULL, PrintContext *pt=NULL);

	//------------------- FileInput --------------------------
	//app.addwindow(new FileDialog(nullptr, "Files", nullptr, 0, -1,-1, 800,600, 0, 0,nullptr,FILES_PREVIEW) );

//	app.addwindow(new FilePreviewer(nullptr,"previewer",nullptr,ANXWIN_ESCAPABLE|FILEPREV_SHOW_DIMS,
//				-1,-1, 600,400,0,
//				"/path/to/something.jpg"));

	//------------------- FontInput --------------------------
//	app.addwindow(new FontDialog(nullptr, "FontDialog", "Font Dialog", ANXWIN_CENTER,
//				0,0,700,900,0,
//				0,nullptr,
//				0, //unsigned long ndstyle,
//				nullptr, //const char *fam,
//				nullptr, //const char *style,
//				30, //size
//				nullptr, //const char *nsample,
//				nullptr, //LaxFont *nfont,
//				false // work_on_dup
//				));

	//------------------- IconSelector --------------------------
//	double th = app.theme->base_font_size;
//	double gap = .3 * app.theme->base_font_size * scale;
//	IconSelector *isel = new IconSelector(nullptr, "isel", nullptr,
//						ANXWIN_CENTER | BOXSEL_ROWS | BOXSEL_HCENTER | BOXSEL_VCENTER,
//						0,0,17*th,300, 0,
//						nullptr,0,nullptr,
//						gap, gap, gap);
//	isel->default_boxw = 5*th;
//	isel->default_boxh = 5*th;
//	isel->AddBox("One",  0);
//	isel->AddBox("Two",  0);
//	isel->AddBox("Three",0);
//	isel->AddBox("Four", 0);
//	app.addwindow(isel);


	//------------------- StackFrame --------------------------
	// vertical
//	StackFrame *stack = new StackFrame(nullptr, "StackFrame","StackFrame", STACKF_VERTICAL|ANXWIN_ESCAPABLE|ANXWIN_CENTER /* STACKF_VERTICAL for vertical */,
//		0,0, 200,600, 0,
//		nullptr,0,nullptr,
//		15 //int ngap
//		);
//	stack->AddWin(new MessageBar(nullptr, "mbar1",nullptr, 0, 0,0,100,100,1, "One"),1,   100,0,0,50,0, 100,0,0,50,0);
//	
//	//stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   100,0,0,50,0, 10000,9900,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   10000,9900,0,50,0,  100,0,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   100,0,10000,50,0,  100,0,0,50,0);
//	stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   100,0,0,50,0,   100,0,10000,50,0);
//	
//	stack->AddWin(new MessageBar(nullptr, "mbar3",nullptr, 0, 0,0,100,100,1, "Three"),1, 100,0,0,50,0, 100,0,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar4",nullptr, 0, 0,0,100,100,1, "Four"),1, 100,0,0,50,0, 100,0,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar5",nullptr, 0, 0,0,100,100,1, "Five"),1, 100,0,0,50,0, 100,0,0,50,0);
//	//stack->HideSubBox(2);
//	//stack->HideSubBox(0);
//	app.addwindow(stack);


	// horizontal
//	StackFrame *stack = new StackFrame(nullptr, "StackFrame","StackFrame", ANXWIN_ESCAPABLE|ANXWIN_CENTER /* STACKF_VERTICAL for vertical */,
//		0,0, 600,200, 0,
//		nullptr,0,nullptr,
//		15 //int ngap
//		);
//	stack->AddWin(new MessageBar(nullptr, "mbar1",nullptr, 0, 0,0,100,100,1, "One"),1,   100,0,0,50,0, 100,0,0,50,0);
//
//
//	//stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   10000,9900,0,50,0,  100,0,0,50,0);
//	stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   100,0,10000,50,0,  100,0,0,50,0);
//
//	//stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   100,0,0,50,0, 10000,9900,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar2",nullptr, 0, 0,0,100,100,1, "Two"),1,   100,0,0,50,0,   100,0,10000,50,0);
//
//
//	//stack->AddWin(new MessageBar(nullptr, "mbar3",nullptr, 0, 0,0,100,100,1, "Three"),1, 100,0,0,50,0, 100,0,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar4",nullptr, 0, 0,0,100,100,1, "Four"),1, 100,0,0,50,0, 100,0,0,50,0);
//	//stack->AddWin(new MessageBar(nullptr, "mbar5",nullptr, 0, 0,0,100,100,1, "Five"),1, 100,0,0,50,0, 100,0,0,50,0);
//	//stack->HideSubBox(2);
//	//stack->HideSubBox(0);
//	app.addwindow(stack);


	//------------------- InterfaceWindow --------------------------
//	LaxFont *font = GetDefaultFontManager()->MakeFont("sans", nullptr, 15, -1);
//	CharacterInterface *character_interface = new CharacterInterface(nullptr, -1, nullptr, font);
//	font->dec_count();
//	InterfaceWindow *iwindow = new InterfaceWindow(nullptr, "InterfaceWindow","Interface Window", ANXWIN_CENTER | ANXWIN_ESCAPABLE,
//		0,0, 600,200, 0,
//		nullptr,0,nullptr,
//		character_interface, 1
//		);
//	app.addwindow(iwindow);

	//------------------- GridSelect InterfaceWindow --------------------------
	GridSelectInterface *grid_interface = new GridSelectInterface(nullptr, -1, nullptr);
	MenuInfo items;
	char str[30];
	for (int c = 0; c < 20; c++) {
		sprintf(str, "%d", c);
		items.AddItem(str);
	}
	grid_interface->UseThisMenu(&items);
	InterfaceWindow *iwindow = new InterfaceWindow(nullptr, "InterfaceWindow","Interface Window", ANXWIN_CENTER | ANXWIN_ESCAPABLE,
		0,0, 600,200, 0,
		nullptr,0,nullptr,
		grid_interface, 1
		);
	app.addwindow(iwindow);

	//------------------- Slider InterfaceWindow --------------------------
//	SliderInterface *sinterface = new SliderInterface();
//	SliderInfo *info = sinterface->newData();
//	//info->from.set(0,0);
//	//info->to.set(100,0);
//	info->min = 0;
//	info->max = 100;
//	info->graphic_size = 40;
//	info->graphic_fill_type = 2;
//	info->line_width = 20;
//	info->outline_width = 5;
//	sinterface->UseThis(info);
//	info->dec_count();
//
//	InterfaceWindow *swindow = new InterfaceWindow(nullptr, "SliderInterfaceWindow",nullptr, ANXWIN_CENTER | ANXWIN_ESCAPABLE,
//		0,0, 600,200, 0,
//		nullptr,0,nullptr,
//		sinterface, 1
//		);
//	app.addwindow(swindow);

	
	//------------------- PopupWindow --------------------------
//	MenuInfo *menu = new MenuInfo();
//	menu->AddItem("one");
//	menu->AddItem("two");
//	menu->AddItem("3");
//	menu->AddItem("4");
//	menu->AddItem("5");
//	menu->AddItem("6");
//	menu->AddItem("7");
//	PopupMenu *popup = new PopupMenu(NULL,_("Move"), 0,
//						0,0,0,0, 1,
//						0,"moveto",
//						0, //mouse to position near?
//						menu,1, nullptr,
//						TREESEL_LEFT);
//	popup->pad = 5;
//	popup->WrapToMouse(0);
//	app.rundialog(popup);

	//----------------- Done setting up. Now run! ----------------------------

	cout <<"------Done adding initial windows in main() -------\n";
	app.run();

	cout <<"------ App Close:  -------\n";
	app.close();
	
	cout <<"------ Bye! -------\n";
	return 0;
}

