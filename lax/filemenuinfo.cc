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
//    Copyright (C) 2004-2010 by Tom Lechner
//

#include <lax/filemenuinfo.h>

#include <cstdio>
#include <dirent.h>


#include <iostream>
using namespace std;
#define DBG 

namespace Laxkit {

/*! \class FileMenuItem
 * \ingroup menuthings
 * \brief Sub class of MenuItem to read in directories only when needed.
 *
 * Uses info to say whether to followlinks or not (1 is yes).
 *
 * *** TODO: sync up all the options with QuickFileOpen/FileDialog/MenuSelector/PopupMenu
 */


//! Constructor, passes NULL as the submenu to MenuItem constructor, and stats newitem.
/*! newitem is the filename to stat.
 * If followlinks!=0 then follow links, otherwise, statbuf will refer to the link itself.
 */
FileMenuItem::FileMenuItem(const char *newitem,unsigned int nstate,int followlinks)
	: MenuItem(newitem,0,nstate,0,NULL,0)
{
	int c;
	info=followlinks;
	if (newitem==NULL) newitem="./";
	if (followlinks) c=stat(newitem,&statbuf);
	else c=lstat(newitem,&statbuf);
	if (c) { //*** error
		perror("filemenuitem constructor stat error");
	} else {
		if (S_ISDIR(statbuf.st_mode)) {
//			DBG cerr <<name<<" has dir!!!!!!!!!"<<endl;
			state|=LAX_HAS_SUBMENU;
		} else {
//			DBG cerr <<name<<" has no dir!!!!!!!!!"<<endl;
		}
	}
	if ((state&LAX_STATE_MASK)==0) state|=LAX_OFF;
}

//! Grab out the submenu, defining if necessary. submenu undefined afterwards.
MenuInfo *FileMenuItem::ExtractMenu()
{
	MenuInfo *sm=GetSubmenu(1);
	subislocal=0;
	submenu=NULL;
	return sm;
}

//! Redifines GetSubmenu to call CreateSubmenu(name) rather than CreateSubmenu(NULL)
MenuInfo *FileMenuItem::GetSubmenu(int create) //create=0
{
	if (state&LAX_HAS_SUBMENU && create) CreateSubmenu(name);
	return submenu;
}

//! Scan in a directory whose path is ntitle, adding new FileMenuItems.
/*! pathtodir must not be NULL to mean ".". You must pass in either "." or "".
 */
MenuInfo *FileMenuItem::CreateSubmenu(const char *pathtodir)
{
	if (!pathtodir) return submenu;
	if (!submenu) {
		 // Read in directory, do first to make sure it exists
		if (pathtodir[0]=='\0') pathtodir=".";
		DIR *dir=opendir(pathtodir);
		if (!dir) {
			DBG perror(pathtodir);
			return NULL;
		}
		submenu=new MenuInfo(pathtodir);
		subislocal=1;

		int errorcode;
		char *str=NULL;
		struct dirent entry,*result;
		do {
			errorcode=readdir_r(dir,&entry,&result);
			if (errorcode>0) break; //error reading directory
			//if (!strcmp(entry.d_name,".") || !strcmp(entry.d_name,"..")) continue;
			if (!strcmp(entry.d_name,".")) continue;
			if (result) {
				makestr(str,pathtodir);
				if (pathtodir[strlen(pathtodir)]!='/') appendstr(str,"/");
				appendstr(str,entry.d_name);
				FileMenuItem *nmi=new FileMenuItem(str,0,info);
				submenu->AddItem(nmi,1);
			}
		} while (result);
		delete[] str;
		closedir(dir);
	}
	state|=LAX_HAS_SUBMENU;
	return submenu;
}


} // namespace Laxkit

