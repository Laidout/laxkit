#include <iostream>


#include <sys/time.h>
#include <sys/resource.h>
#include <cstdlib>
#include <unistd.h>
#include <lax/freedesktop.h>
#include <lax/utf8string.h>

#include <lax/attributes.h>

#include <lax/lists.h>

using namespace std;
using namespace Laxkit;


int main(int argc,char **argv)
{
	Attribute att;
//	const char *blah[]= {
//			"meta",
//			"META",
//			"link",
//			"LINK",
//			"br",
//			"BR",
//			"hr",
//			"HR",
//			"basefont",
//			"BASEFONT",
//			"img",
//			"IMG",
//			"center",
//			"CENTER"
//		};
	if (argc==1) { cout <<"attxml infile"<<endl; exit(1); }

	Utf8String infile = argv[1];
	//Utf8String outfile = nullptr;
	//if (argc > 2) outfile = argv[2];

	//-----------fromxml:
	if (infile.EndsWith(".xml") || infile.EndsWith(".XML")) {
		Attribute att;
		XMLFileToAttribute(&att, infile.c_str(), nullptr);
		att.dump_out(stdout, 0);
		
		exit(0);	
	}
		
	//---------from att
	att.dump_in(infile.c_str());
	//cout <<"--------------attribute in:-----------------"<<endl;
	att.dump_out(stdout,0);

	return 0;
}
