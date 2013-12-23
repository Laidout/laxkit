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

#include <lax/laxoptions.h>
#include <lax/strmanip.h>
#include <lax/language.h>
#include <sys/ioctl.h>

#include <lax/lists.cc>

#include <cstdlib>
#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {


//-------------------------------------- LaxOption -------------------------------------


/*! \class LaxOption
 * \brief Used internally by LaxOptions.
 */


/*! If ex_is_full, then use all of opt_example as the option example, instead of building text
 * from the long and short options. Otherwise opt_example is just the name of the argument, to be
 * used when building that text.
 */
LaxOption::LaxOption(const char *l_option, 
		      int s_option,
			  int nflags,
			  const char *helptext,
			  int nid,
			  const char *opt_example,
			  int ex_is_full
			)
{
	parsed_present=0;

	long_option=newstr(l_option);
	short_option=s_option;
	flags=nflags;
	help_text=newstr(helptext);
	option_id=nid;

	option_example=NULL;
	parsed_arg=NULL;

	short_example=newstr(opt_example);

	if (ex_is_full) option_example=newstr(opt_example);
	if (!option_example && (long_option || short_option)) {
		option_example=new char[4+(long_option?strlen(long_option):0) + 4+(short_option?4:0)];
		if (long_option && short_option) sprintf(option_example,"-%c, --%s",(char)short_option,long_option);
		else if (long_option) sprintf(option_example,"--%s",long_option);
		else sprintf(option_example,"-%c",(char)short_option);

		if ((nflags&LAXOPT_HasParam) && !ex_is_full) {
			appendstr(option_example, " ");
			appendstr(option_example, opt_example?opt_example:"arg");
		}
	}
}

LaxOption::~LaxOption()
{
	if (parsed_arg)     delete[] parsed_arg;
	if (long_option)    delete[] long_option;
	if (option_example) delete[] option_example;
	if (short_example)  delete[] short_example;
	if (help_text)      delete[] help_text;
}

/*! Return parsed_arg.
 *
 * if (flags&LAXOPT_Remaining), return this->long_option, which is basically the parsed arg, but
 * there was no actual long option. This is a coding hack.
 */
const char *LaxOption::arg()
{
	if (flags&LAXOPT_Remaining) return long_option;
	return parsed_arg;
}


//-------------------------------------- LaxOptions -------------------------------------
/*! \class LaxOptions
 * \brief Class to simplify parsing command line options, and outputing help text.
 *
 * This class currently assumes that each option can appear once. They can have
 * long and short options with descriptions, and simple grouping of options.
 *
 * In outputting, it is currently assumed that the option example is short, and contained on one line.
 * The help text per option is wrapped to fit within a certain number of columns, and is assumed
 * to be able to be read as a single line (or a wrapped single line).
 *
 * You would Add() new options to a LaxOptions object, then when done, call Parse().
 * Afterwards you can loop over parsed options with start(), next(), and remaining().
 *
 * \todo perhaps allow duplicate options, like "-i some/dir -i some/other/dir"
 */


LaxOptions::LaxOptions()
{
	helpheader=NULL;
	optionsheader=NULL;
	usageline=NULL;
	curgroup=0;
	loop_which=0;
	first_remaining=-1;
	curitem=-1;
	erred=-1;
}

LaxOptions::~LaxOptions()
{
	if (helpheader) delete[] helpheader;
	if (optionsheader) delete[] optionsheader;
	if (usageline) delete[] usageline;
}

//! Output a man page formatted template, listing the options.
/*! The idea here is to simplify creation of man pages, to automatically
 * get a formatted option list, which is otherwise very tedious to produce.
 *
 *  If file==NULL, then use stdout.
 */
void LaxOptions::HelpHtml(FILE *file)
{
	if (!file) file=stdout;

	fprintf(file,"<html>\n<head>\n</head>\n<body>\n\n");

	if (helpheader) fprintf(file,"<h1>%s</h1>\n\n",helpheader);
	if (usageline) fprintf(file, " %s<br/><br/>\n\n", usageline);
	fprintf(file, "%s<br/><br/>\n\n", optionsheader?optionsheader:_("Options:"));


	LaxOption *o;


	fprintf(file,"<table cellpadding=\"5\">\n");

	for (int c=0; c<n; c++) {
		o=e[c];
		if (o->flags&LAXOPT_Hidden) continue;
		if (o->flags&LAXOPT_Group) {
			fprintf(file,"  <tr>\n");
			fprintf(file,"    <td colspan=\"4\">\n");
			if (o->str()) fprintf(file,"      %s\n",o->str());
			else fprintf(file,"      group\n");
			fprintf(file,"    </td>\n");
			fprintf(file,"  </tr>\n");
			continue;
		}

		fprintf(file,"  <tr>\n");
		fprintf(file,"    <td>\n");
		if (o->long_option) {
			fprintf(file,"      --%s\n",o->long_option);
		}
		fprintf(file,"    </td>\n");

		fprintf(file,"    <td>\n");
		if (o->short_option) {
			fprintf(file,"      -%c\n",o->short_option);
		}
		fprintf(file,"    </td>\n");

		fprintf(file,"    <td>\n");
		if (o->short_example) {
			fprintf(file,"      %s\n",o->short_example);
		}
		fprintf(file,"    </td>\n");

		fprintf(file,"    <td>\n");
		if (o->helptext()) {
			fprintf(file,"      %s\n",o->helptext());
		}
		fprintf(file,"    </td>\n");
		fprintf(file,"  </tr>\n");
	}

	fprintf(file,"</table>\n\n");


	fprintf(file,"</body>\n</html>");
}

//! Output an html snippet formatted template, listing the options.
/*! Output a table with 4 columns:
 *   long form, short form, parameter example, description
 *
 *   If file==NULL, then use stdout.
 */
void LaxOptions::HelpMan(FILE *file)
{
	if (!file) file=stdout;

	fprintf(file,".TH REPLACEWITHPROGRAMNAME SECTION \"November 13, 2012\"\n"
				 ".SH NAME\n");
	if (helpheader) fprintf(file,"%s\n",helpheader);
	else fprintf(file,
				"Program \\- brief summary\n"
				".SH SYNOPSIS\n"
				".B program\n"
				".RI [ options ] \" files\" ...\n"
				".br\n"
				".SH DESCRIPTION\n"
				"\\fBProgram\\fP ...\n");
	if (usageline) fprintf(file, " %s\n\n", usageline);
	fprintf(file, "%s\n", optionsheader?optionsheader:_("Options:"));


	fprintf(file,".SH OPTIONS\n\n");

	LaxOption *o;
	int group=0;
	const char *s;

	for (int c=0; c<n; c++) {
		o=e[c];
		if (o->flags&LAXOPT_Hidden) continue;
		if (o->flags&LAXOPT_Group) {
			fprintf(file,"\n.SH");
			if (o->str()) fprintf(file,"%s\n",o->str());
			else fprintf(file," group\n");
			group++;
			continue;
		}

		fprintf(file,"\n.TP\n");
		if (o->option_example) {
			fprintf(file,".B ");
			s=o->option_example;
			for (unsigned int i=0; i<strlen(s); i++) {
				if (s[i]=='-') fprintf(file,"\\");
				if (s[i]=='"') fprintf(file,"'");
				else fprintf(file,"%c",s[i]);
			}
		}
		fprintf(file,"\n%s\n",o->helptext());
	}
}

//! Output the help to the given file as plain text. Defaults to stderr if NULL.
/*! If columns<=0, then use ioctl(0, TIOCGWINSZ, &w), w.ws_col for the number of columns.
 */
void LaxOptions::Help(FILE *file, int columns)
{
	if (!file) file=stderr;

	if (columns<=0) {
		//char *str=getenv("COLUMNS");
		//if (str) columns=strtol(str,NULL,10);
		struct winsize w;
		ioctl(0, TIOCGWINSZ, &w);
		columns=w.ws_col;
		if (columns<=0) columns=100;
	}

	// Todo: *** wrap various headers to fit columns
	if (helpheader) fprintf(file, "%s\n\n", helpheader);
	if (usageline) fprintf(file, " %s\n\n", usageline);
	fprintf(file, "%s\n", optionsheader?optionsheader:_("Options:"));

	char fmt[20];
	char *odesc, *odt, *odtt;
	char odescchar;
	LaxOption *o;
	int w;
	int ow=0; //width of option column
	int group=0;
	unsigned int descwidth; //width available for description

	 // compute column width
	if (first_remaining<0) first_remaining=n;
	for (int c=0; c<first_remaining; c++) {
		o=e[c];
		if (!o->option_example) continue;
		w=strlen(o->option_example);
		if (w>ow) ow=w;
	}
	descwidth=columns-ow-4;
	if (descwidth<0) descwidth=60;

	sprintf(fmt,"  %%-%ds  ",ow);
	for (int c=0; c<n; c++) {
		o=e[c];
		if (o->flags&LAXOPT_Hidden) continue;
		if (o->flags&LAXOPT_Group) {
			fprintf(file,"\n");
			if (o->str()) fprintf(file,"%s\n",o->str());
			group++;
			continue;
		}
		w=2;
		if (o->option_example) {
			fprintf(file,fmt,o->option_example);
		}

		odesc=newstr(o->helptext());
		odt=odesc;
		if (odt) {
		  do {
			if (strlen(odt)<descwidth) {
				 //the line fits
				fprintf(file,"%s\n",odt);
				break;
			}
			 //else line has to be broken
			odtt=odt+descwidth;
			while (odtt!=odt && !isspace(*odtt)) odtt--;
			if (odtt==odt) {
				 //no whitespace at all, so print that line anyway
				odtt=odt+descwidth;
			}
			odescchar=*odtt;
			*odtt='\0';
			fprintf(file,"%s\n",odt);
			fprintf(file,fmt,""); //<- prepare next line

			if (!isspace(odescchar)) {
				*odtt=odescchar;
				odt=odtt;
			} else odt=odtt+1;
		  } while (odt && *odt);
		  if (odesc) delete[] odesc;
		}
	}
}

int LaxOptions::Verify()
{ cerr <<" *** TODO: LaxOptions::Verify()"<<endl; return -1; }

int LaxOptions::NewGroup(const char *group_header_text)
{
	curgroup++;
	push(new LaxOption(NULL, 0, LAXOPT_Group, group_header_text, -1, NULL));
	return curgroup;
}

//! Create the top portion of help text, not option specific. Multiple calls will append.
void LaxOptions::HelpHeader(const char *header_text)
{
	if (!helpheader) makestr(helpheader,header_text);
	else appendstr(helpheader,header_text);
}

void LaxOptions::UsageLine(const char *usage_line)
{
	makestr(usageline,usage_line);
}

void LaxOptions::OptionsHeader(const char *options_line)
{
	makestr(optionsheader,options_line);
}

/*! If has_param==1, then opt_example is used as the text to indicate the argument in the help text.
 * If opt_example is NULL, then use "arg" instead. 
 * If has_param==2, then opt_example is used as the whole option example.
 */
int LaxOptions::Add(const char *long_option, 
		    int short_option,
			int has_param, //!< nonzero if the option takes a parameter, else it doesn't
			const char *nhelp_text,
			int nid,
			const char *opt_example //!< like '--list, -l "all"' instead of just "--list, -l"
		   )
{
	int nflags=0;
	if (has_param) nflags=LAXOPT_HasParam;
	push(new LaxOption(long_option, short_option, nflags, nhelp_text, nid, opt_example, has_param==2));
	return 0;
}

//! Parse options.
/*! This follows basic conventions that long options are preceeded with "--". Any number of short options 
 * can follow a "-" unless it expects a parameter. An argument of only "--" is ignored and signifies that all
 * following arguments are not options.
 * Parameters can be specified with "--long-option arg", '-s arg' for short options, or -abcd for multiple
 * short options.
 *
 * The original argv is not modified.
 *
 * On success, returns the number of options found and parsed, which will be 0 or more.
 * If there is a an unknown option, then parsing stops, argerr is set to the index of argv for the offending
 * item and -1 is returned.
 * If there is a missing parameter, then parsing stops, argerr is set to the index of argv for the offending
 * item and -2 is returned.
 *
 * Note that argv[0] is ignored.
 */
int LaxOptions::Parse(int argc, char **argv, int *argerr)
{
	int nopts=0;
	int s;
	int i, c2;
	LaxOption *o=NULL;

	first_remaining=n;

	for (int c=1; c<argc; c++) {
		if (argv[c][0]=='-') {
			if (argv[c][1]=='-') {
				//we have "--(something)", long option probably

				if (argv[c][2]=='\0') {
					 //all remaining are not options
					for (int c2=c+1; c2<argc; c2++) {
						push(new LaxOption(argv[c2], 0, LAXOPT_Remaining, NULL, 0, NULL));
					}
					break;
				} else {
					i=1;
					for (c2=0; c2<first_remaining; c2++) {
						o=e[c2];
						if (!strcmp(o->str(),argv[c]+2)) break;
					}
					if (c2!=first_remaining) {
						 //option found
						o->parsed_present=1;
						nopts++;
						if (o->flags&LAXOPT_HasParam) {
							if (c+1==argc) {
								 //missing parameter!
								erred=c2;
								if (argerr) *argerr=c;
								return -2;
							}
							makestr(o->parsed_arg, argv[c+1]);
							c++;
						}
					} else {
						 //unknown option!
						if (argerr) *argerr=c;
						return -1;
					}
				}

			} else {
				//is short option

				i=1;
				while (i>0 && argv[c][i]!='\0') {
					s=argv[c][i++];
					for (c2=0; c2<first_remaining; c2++) {
						o=e[c2];
						if (s==o->chr()) break;
					}
					if (c2!=first_remaining) {
						 //option found
						o->parsed_present=1;
						nopts++;
						if (o->flags&LAXOPT_HasParam) {
							if (argv[c][i]!='\0' || c+1==argc) {
								 //missing parameter!
								erred=c2;
								if (argerr) *argerr=c;
								return -2;
							}
							makestr(o->parsed_arg, argv[c+1]);
							c++;
							i=-1;
							break; //stop looking for more short options
						}
					} else {
						 //unknown option!
						if (argerr) *argerr=c;
						return -1;
					}
				}
			}
		} else {
			 //is not on option
			push(new LaxOption(argv[c], 0, LAXOPT_Remaining, NULL, 0, NULL));
		}
	}
	return nopts;
}

//! When a missing parameter of unknown option is encountered in Parse(), use this to return the object for more info.
/*! Particularly, for missing parameters, this gives you easy access to the help text corresponding to that option.
 */
LaxOption *LaxOptions::error()
{
	if (erred>=0 && erred<n) return e[erred];
	return NULL;
}

//! Set which set of arguments to review, 1 for remaining non-options, 0 for options.
/*! Sets loop counter to 0.
 */
int LaxOptions::Review(int which)
{
	loop_which=which?1:0;
	curitem=loop_which?first_remaining:0;
	curitem--;
	return loop_which;
}

//! Start looping with any option arguments, and return the first of those, if any.
LaxOption *LaxOptions::start()
{
	if (erred>=0) return NULL;

	loop_which=0;
	curitem=0;
	while (n && curitem<first_remaining && !e[curitem]->parsed_present) curitem++;
	if (curitem>=0 && curitem<first_remaining) return e[curitem];
	return NULL;
}

//! Start looping with any non-option arguments, and return the first of those, if any.
LaxOption *LaxOptions::remaining()
{
	if (erred>=0) return NULL;

	loop_which=1;
	curitem=first_remaining;
	if (n && curitem>=0 && first_remaining<n) return e[curitem];
	return NULL;
}

/*! Find an option corresponding to either long_option or short_option (when non-null).
 */
LaxOption *LaxOptions::find(const char *long_option, int short_option)
{
	for (int c=0; c<howmany(); c++) {
		if (long_option && e[c]->long_option && !strcmp(long_option,e[c]->long_option))
			return e[c];
		if (short_option!=0 && short_option==e[c]->short_option) return e[c];
	}
	return NULL;
}

LaxOption *LaxOptions::next()
{
	if (erred>=0) return NULL;

	if (loop_which==0)  {
		 //options
		while (1) {
			if (curitem<first_remaining && curitem<n) curitem++;
			if (curitem==first_remaining || curitem==n) return NULL;
			if (!e[curitem]->parsed_present) continue; //skip any options not present
			return curitem>=0?e[curitem]:NULL;
		}
	}

	 //remaining
	if (curitem<n) curitem++;
	if (curitem==n) return NULL;
	return curitem>=0?e[curitem]:NULL;
}

//! Return number of remaining options
int LaxOptions::more()
{
	if (loop_which==0) {
		int n=0;
		for (int c=curitem+1; c<first_remaining; c++) 
			if (e[c]->parsed_present) n++;
		return n;
	}
	return n-curitem-1;
}

} //namespace Laxkit

