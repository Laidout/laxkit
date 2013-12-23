//
//	
//    The Laxkit, a windowing toolkit
//    Please consult http://laxkit.sourceforge.net about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    Copyright (C) 2011 by Tom Lechner
//
#ifndef _LAX_LAXOPTIONS_H
#define _LAX_LAXOPTIONS_H

#include <cstdio>
#include <lax/lists.h>

namespace Laxkit {


//-------------------------------------- LaxOption -------------------------------------

#define LAXOPT_Hidden        (1<<0)
#define LAXOPT_HasParam      (1<<1)
#define LAXOPT_Group         (1<<2)
#define LAXOPT_Remaining     (1<<3)
#define LAXOPT_Unknown       (1<<4)
#define LAXOPT_MissingParam  (1<<5)

class LaxOption
{
  public:
	char *help_text;
	char *long_option;
	int short_option;
	int flags;
	int option_id;
	int groupnumber;
	char *short_example;
	char *option_example; // like '--list, -l "all"' instead of just "--list, -l"

	int parsed_present;
	char *parsed_arg;

	LaxOption(const char *l_option, 
		      int s_option,
			  int nflags,
			  const char *nhelp_text,
			  int nid,
			  const char *opt_example, // like '--list, -l "all"' instead of just "--list, -l"
			  int ex_is_full=0
			);
	~LaxOption();
	int unknown() { return flags&LAXOPT_Unknown; }
	int missing_param() { return flags&LAXOPT_MissingParam; }
	int has_arg() { return flags&LAXOPT_HasParam; }
	int chr() { return short_option; }
	const char *str() { return long_option; }
	int id() { return option_id; }
	int present() { return parsed_present; }
	const char *helptext() { return help_text; }
	const char *arg();

};


//-------------------------------------- LaxOptions -------------------------------------
class LaxOptions : protected PtrStack<LaxOption>
{
  protected:
	char *helpheader;
	char *optionsheader;
	char *usageline;

	int curgroup, curitem;
	int loop_which;
	int first_remaining;
	int erred;
  public:
	LaxOptions();
	~LaxOptions();

	int Verify();
	void HelpHeader(const char *header_text);
	const char *HelpHeader() { return (helpheader?helpheader:"Help!"); }
	void UsageLine(const char *usage_line);
	void OptionsHeader(const char *usage_line);
	int NewGroup(const char *group_header_text);
	int Add(const char *long_option, 
		    int short_option,
			int has_param,
			const char *nhelp_text,
			int nid=0,
			const char *opt_example=NULL // like '--list, -l "all"' instead of just "--list, -l"
		   );

	void Help(FILE *file=NULL, int columns=-1);
	void HelpHtml(FILE *file=NULL);
	void HelpMan(FILE *file=NULL);
	int Parse(int argc, char **argv, int *argerr);
	int Review(int which); //1 for remaining, 0 for options
	LaxOption *error();
	LaxOption *start();
	LaxOption *remaining();
	LaxOption *next();
	LaxOption *find(const char *long_option, int short_option);
	int more(); //return number of remaining options

	//void man_page_text(); //print to stdout the options formatted for a man page
};

} //namespace Laxkit



#endif 

