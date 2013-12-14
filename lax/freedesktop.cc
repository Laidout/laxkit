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
//    Copyright (C) 2007,2010 by Tom Lechner
//

#include <locale.h>
#include <sys/file.h>
#include <openssl/md5.h>
#include <cstdlib>
#include <unistd.h>

#include <lax/freedesktop.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>
#include <lax/attributes.h>

#include <lax/lists.cc>

#include <cstdlib>

#define DBG
#include <iostream>
using namespace std;

namespace LaxFiles {

/*! If timestamp==NULL, then use the current time. Otherwise it must be a string 
 * representing the number of seconds since the epoch, which is the beginning of
 * Jan 1, 1970 UTC. time gettimeofday
 *
 * file must be an absolute path.
 *
 * \todo there could be more error checking in here. If recent file spec changes, could well break this.
 * \todo support the other commonly used recent file: ~/.recently-used.xbel
 */
int touch_recently_used(const char *file, const char *mime, const char *group, const char *timestamp)
{
	if (!file) return 1;

	 //make sure to use uri rather than file
	char *uri=newstr(file);
	if (strncmp(uri,"file://",7)) prependstr(uri,"file://");
	if (uri[7]!='/') {
		delete[] uri; //was not an absolute path
		return 2;
	}

	 //find timestamp
	char ts[20];
	if (!timestamp) {
		time_t t=time(NULL);
		sprintf(ts,"%lu",t);
		timestamp=ts;
	}

	 //read in recently-used
	Attribute *recent=recently_used_att();
	Attribute *content;
	if (!recent) {
		 //problem reading ~/.recently-used, or file does not exist
		recent=new Attribute;
		recent->push("?xml",(char*)NULL);
		recent->attributes.e[0]->push("version","1.0");
		recent->push("RecentFiles",(char*)NULL);
		recent->attributes.e[1]->push("content:",(char*)NULL);
		content=recent->attributes.e[1]->attributes.e[0];
	} else {
		 //find RecentFiles->content: 
		content=recent->find("RecentFiles");
		if (content) content=content->find("content:");
		if (!content) {
			 //error;
			delete[] uri;
			delete recent;
			return 3;
		}

	}
	
	 //search for file
	Attribute *item=NULL,*att=NULL;
	int c;
	for (c=0; c<content->attributes.n; c++) {
		if (strcmp(content->attributes.e[c]->name,"RecentItem")) continue;
		item=content->attributes.e[c]->find("content:");
		if (!item) continue;
		att=item->find("URI");
		if (att && !strcmp(att->value,uri)) break;
		item=NULL;
	}
	if (item) {
		 //file found, so update it

		 //with newer(?) timestamp
		att=item->find("Timestamp");
		if (strtol(att->value,NULL,10)<strtol(timestamp,NULL,0))
			makestr(att->value,timestamp);

		 //hopefully gratuitous mime type check
		att=item->find("Mime-Type");
		if (strcmp(att->value,mime)) {
			DBG cerr <<"***warning: mime type not same as before"<<endl;
			makestr(att->value,mime);
		}

		 //make sure group is in there
		if (group) {
			att=item->find("Groups");
			att=att->attributes.e[0]; //this wouldbe "content:" for Groups
			for (c=0; c<att->attributes.n; c++) {
				if (!strcmp(att->attributes.e[c]->value,group)) break;
			}
			if (c==att->attributes.n) att->push("Group",group);
		}

		makestr(att->value,timestamp);
	} else {
		 //file not found, add to list {
		item=new Attribute("RecentItem",NULL);
		item->push("content:",(char*)NULL);
		item->attributes.e[0]->push("URI",uri);
		item->attributes.e[0]->push("Mime-Type",mime);
		item->attributes.e[0]->push("Timestamp",ts);
		if (group) {
			item->attributes.e[0]->push("Groups");
			item->attributes.e[0]->attributes.e[3]->push("content:");
			item->attributes.e[0]->attributes.e[3]->attributes.e[0]->push("Group",group);
		}
		
		content->push(item,0);
	}

	 //make sure < 500 items
	Attribute *att2;
	while (content->attributes.n>500) {
		 //find oldest one
		c=content->attributes.n-1;
		att=content->attributes.e[c]->attributes.e[0]->find("Timestamp");
		int c2;
		long l=strtol(att->value,NULL,0),l2;
		for (c2=0; c2<content->attributes.n; c2++) {
			att2=content->attributes.e[c2]->attributes.e[0]->find("Timestamp");
			l2=strtol(att2->value,NULL,10);
			if (l2<l) { 
				c=c2;
				l=l2;
			}
		}

		content->attributes.remove(c); //the oldest one
	}

	
	 //write back out recently-used
	setlocale(LC_ALL,"C");
	char *rf=expand_home("~/.recently-used");
	int fd=open(rf,O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
	delete[] rf;
	if (fd<0) { 
		setlocale(LC_ALL,"");
		delete[] uri;
		delete recent;
		return 1; 
	}
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"w");
	if (!f) { 
		setlocale(LC_ALL,"");
		delete[] uri;
		delete recent;
		close(fd);
		return 1; 
	}

	AttributeToXMLFile(f,recent,0);

	flock(fd,LOCK_UN);
	fclose(f);
	setlocale(LC_ALL,"");
	delete[] uri;
	delete recent;

	return 0;
}

//! Return an Attribute created from XML.
/*! Default is to read from ~/.recently-used.
 * See the Recently Used Specification at Freedesktop.org:
 * http://www.freedesktop.org/wiki/Specifications/recent-file-spec
 *
 * This locks the file, reads it in to an Attribute with XMLChunkToAttribute(),
 * then unlocks the file.
 *
 * Please note that many programs are not using ~/.recently-used, but
 * are instead using ~/.recently-used.xbel.
 *
 */
Attribute *recently_used_att(const char *file)
{
	if (isblank(file)) file="~/.recently-used";

	setlocale(LC_ALL,"C");
	char *rf=expand_home(file);
	int fd=open(rf,O_RDONLY);
	delete[] rf;
	if (fd<0) { setlocale(LC_ALL,""); return NULL; }
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"r");
	if (!f) { setlocale(LC_ALL,""); close(fd); return NULL; }

	Attribute *mainatt=XMLChunkToAttribute(NULL,f,NULL);

	flock(fd,LOCK_UN);
	fclose(f);// this closes fd too
	setlocale(LC_ALL,"");

	return mainatt;
}


//! Return a newline separated list of recent files.
/*! If mimetype!=NULL, then return only items of that type.
 * If group!=NULL, then return items in only that group. If there are private
 * items in the raw list, then return them only if its group matches the group
 * you are looking for.
 *
 * If includewhat==0, then only return the file name. If it is 1, then each line contains
 * the mime type and file, separated by a tab. If it is 2, then each line is timestamp, then
 * file. If it is 3, then each line is mime-tab-timestamp-tab-file.
 *
 * By default, this reads in from ~/.recently-used. See the Recently Used Specification at Freedesktop.org:
 * http://www.freedesktop.org/wiki/Specifications/recent-file-spec
 *
 * Please note that many programs are not using this spec, but are instead using
 * the Freedesktop bookmark spec:
 * http://www.freedesktop.org/wiki/Specifications/desktop-bookmark-spec,
 * which puts info into ~/.recently-used.xbel instead. Apparently, this one
 * will supercede the Recently Used spec.
 *
 * If you choose to read from ~/.recently-used, then it expects that file to return
 * at Attribute structured basically like this: *
 * <pre>
 *  ?xml
 *    version "1.0"
 *  RecentFiles
 *    content:
 *      RecentItem
 *        content:
 *          URI "file:///blah/blah"
 *          Mime-Type "text/plain"
 *          Timestamp "1192504650"
 *          Groups
 *            content:
 *              Group "gedit"
 *      RecentItem
 *        content:
 *          URI "file:///blah/blah2"
 *          Mime-Type "image/jpeg"
 *          Timestamp "1192420896"
 *          Groups
 *            content:
 *              Group "gimp"
 * </pre>
 *
 * If you instead to read from ~/.recently-used.xbel, then an attribute of the following
 * form is expected.
 * <pre>
 *  ?xml
 *    version "1.0"
 *    encoding "UTF-8"
 *  xbel
 *    version "1.0"
 *    xmlns:bookmark "http://www.freedesktop.org/standards/desktop-bookmarks"
 *    xmlns:mime "http://www.freedesktop.org/standards/shared-mime-info"
 *    content:
 *      bookmark
 *        href "file:///home/tom/images/strobist/pdxstrobist/2010-01-10-BlueMonk/arms/arms.xcf"
 *        added "2010-01-22T14:35:40Z"
 *        modified "2010-02-21T05:35:45Z"
 *        visited "2010-01-22T14:35:40Z"
 *        content:
 *          info
 *            content:
 *              metadata
 *                owner "http://freedesktop.org"
 *                content:
 *                  mime:mime-type
 *                    type "image/xcf"
 *                  bookmark:groups
 *                    content:
 *                      bookmark:group "Graphics"
 *                  bookmark:applications
 *                    content:
 *                      bookmark:application
 *                        name "GNU Image Manipulation Program"
 *                        exec "&apos;gimp-2.6 %u&apos;"
 *                        modified "2010-02-21T05:35:45Z"
 *                        count "32"
 *  
 * </pre>
 *
 * \todo support the other commonly used recent file: ~/.recently-used.xbel
 * \todo not a whole lot of error checking in here yet.
 */
char *recently_used(const char *mimetype,const char *group, int includewhat)
{
	Attribute *mainatt=recently_used_att();
	if (!mainatt) return NULL;

	char *recent=NULL;

	int priv,ingroup=0;
	char *name,*value;
	char *mime,*file,*timestamp;
	Attribute *att2,*att=mainatt->find("RecentFiles");
	if (!att) return NULL;//protection against malformed files

	if (att->attributes.n==1 && !strcmp(att->attributes.e[0]->name,"content:"))
		att=att->attributes.e[0];

	for (int c=att->attributes.n-1; c>=0; c--) { //foreach RecentItem
		if (strcmp(att->attributes.e[c]->name,"RecentItem")) continue;
		priv=0;
		mime=file=timestamp=NULL;
		ingroup=0;
		if (!att->attributes.e[c]->attributes.n) continue; //protection against malformed files
		att2=att->attributes.e[c]->attributes.e[0];//the RecentItem->content: attribute
		for (int c2=0; c2<att2->attributes.n; c2++) {
			name =att2->attributes.e[c2]->name;
			value=att2->attributes.e[c2]->value;
			if (!strcmp(name,"URI")) {
				file=value;
			} else if (!strcmp(name,"Mime-Type")) {
				if (mimetype && strcmp(mimetype,value)) continue; //ignore unwanted mimes
				mime=value;
			} else if (!strcmp(name,"Timestamp")) {
				timestamp=value;
			} else if (!strcmp(name,"Private")) {
				priv=1;
			} else if (!strcmp(name,"Groups") && group) {
				if (!att2->attributes.e[c2]->attributes.n) continue; //protection against malformed files
				Attribute *att3=att2->attributes.e[c2]->attributes.e[0];//the Groups->content: attribute
				if (strcmp(att3->name,"content:")) continue;

				for (int c3=0; c3<att3->attributes.n; c3++) {
					name =att3->attributes.e[c3]->name;
					value=att3->attributes.e[c3]->value;
					if (value && !strcmp(name,"Group")) {
						if (!strcmp(value,group)) { ingroup=1; break; }
					} 
				}
			}
		}
		if (group && !ingroup) continue; //record only a particular group
		if (!group && priv) continue;   //ignore private elements if you are not looking for that group
		if (mime && includewhat&1)      { appendstr(recent,mime);      appendstr(recent,"\t"); }
		if (timestamp && includewhat&2) { appendstr(recent,timestamp); appendstr(recent,"\t"); }
		if (file) { 
			if (!strncmp(file,"file://",7)) file+=7;
			appendstr(recent,file); appendstr(recent,"\n"); 
		}
	}
	delete mainatt;

	return recent;
}

//! Read in file bookmarks.
/*! filetype will be "gtk", "kde", "rox", "lax". If file==NULL and filetype==NULL,
 * then each of the previous filetypes is used. If file!=NULL and filetype==NULL, then NULL
 * is returned. Otherwise, a new char[] with the file or directories separated by a newline is returned.
 *
 * If file==NULL, then read in the default locations for filetype, which are:
 *
 * gtk: ~/.gtk-bookmarks\n
 * kde: ~/.kde/share/apps/konqueror/bookmarks.xml\n
 * rox: ~/.config/rox.sourceforge.net/ROX-Filer/Bookmarks.xml\n
 * lax: (no default file at this time)\n
 * possibly fd desktop bookmark spec says folder bookmarks in ~/.shortcuts.xbel
 *
 * \todo need to lock files on opening
 */
char *get_bookmarks(const char *file,const char *filetype)
{
	if (filetype==NULL && file!=NULL) return NULL;
	if (filetype==NULL) {
		char *str,*str2;
		str=get_bookmarks(NULL,"gtk");
		str2=get_bookmarks(NULL,"kde");
		if (str2) { appendstr(str,str2); delete[] str2; }
		str2=get_bookmarks(NULL,"rox");
		if (str2) { appendstr(str,str2); delete[] str2; }
		str2=get_bookmarks(NULL,"lax");
		if (str2) { appendstr(str,str2); delete[] str2; }
		return str;
	}
	if (file==NULL) {
		if (!strcmp(filetype,"gtk")) file="~/.gtk-bookmarks";
		else if (!strcmp(filetype,"kde")) file="~/.kde/share/apps/konqueror/bookmarks.xml";
		else if (!strcmp(filetype,"rox")) file="~/.config/rox.sourceforge.net/ROX-Filer/Bookmarks.xml";
	}
	if (!file) return NULL;
	char *ff=newstr(file);
	expand_home_inplace(ff);
	if (file_exists(ff,1,NULL)!=S_IFREG) return NULL;
	FILE *f=fopen(ff,"r");
	delete[] ff;
	if (!f) return NULL;
	char *files=NULL;

	if (!strcmp(filetype,"kde") || !strcmp(filetype,"rox")) {
		//Attribute *att=XMLChunkToAttribute(NULL,f,NULL);
		//***
	} else if (!strcmp(filetype,"gtk")) {
		 //gtk uses (as far as I know) a simple list of lines like:   
		 //  file:///blah/blah
		char *line=NULL;
		size_t n=0;
		int c;
		while (1) {
			c=getline(&line,&n,f);
			if (c<=0) break;
			if (!isblank(line)) appendstr(files,line+7);
		}
	} else if (!strcmp(filetype,"lax")) {
		 //is lax indented format
		//Attribute *att=anXApp::app->Resource("Bookmarks");
	}

	fclose(f);
	return files;
}

//! Add a directory bookmark in some standard way.
/*! Currently, where is ignored. This will always default to adding
 * the directory to ~/.gtk-bookmarks.
 *
 * Return 0 for added. Return -1 for already there.
 * Return 1 for directory was not a directory.
 * Return other positive number for other error, not added.
 */
int add_bookmark(const char *directory, int where)
{
	if (!directory) return 3;
	char *dir=file_to_uri(directory);
	if (file_exists(dir+7,1,NULL)!=S_IFDIR) { //skip over the "file://"
		delete[] dir;
		return 1;
	}
	char *bookmarkfile=expand_home("~/.gtk-bookmarks");

	int fd=open(bookmarkfile, O_CREAT|O_RDWR, S_IREAD|S_IWRITE);
	delete[] bookmarkfile;
	if (fd<0) { 
		delete[] dir;
		return 2; 
	}
	flock(fd,LOCK_EX);
	FILE *f=fdopen(fd,"w+");//open for read and write

	char *line=NULL;
	size_t n=0;
	ssize_t s=0;
	int lasthadnl=0;
	while (1) {
		s=getline(&line, &n, f);
		if (s>0) {
			if (line[s-1]=='\n') { line[s-1]='\0'; lasthadnl=1; }
			else lasthadnl=0;
		}
		if (s<=0) { //eof
			fseek(f,0,SEEK_END);
			if (!lasthadnl) fwrite("\n",1,1, f); //add newline
			fwrite(dir,1,strlen(dir), f);
			break; 
		}
	
		if (!strcmp(line,dir)) break; //line already exists
	}
	if (line) free(line);

	flock(fd,LOCK_UN);
	fclose(f);

	delete[] dir;
	return 0;
}

//! Return the freedesktop thumbnail name corresponding to file.
/*! If which=='n' (default), then use the "normal" thumbnail, else use the "large" thumbnail.
 *
 * Returns a new'd char[] with the path to the presumed preview. Does no existence check
 * or actual thumbnail generation.
 */
char *freedesktop_thumbnail(const char *file, char which)
{
	char *pname;
	char *str,*h;
	unsigned char md[17];

	 //provide space for 32 characters of the MD5 hex value
	pname=newstr(which=='n'?
					 "~/.thumbnails/normal/                                .png"
					 :"~/.thumbnails/large/                                .png");
	expand_home_inplace(pname);

	h=full_path_for_file(file,NULL);
	str=file_to_uri(h);

	delete[] h;
	if (!str) cerr <<"**** ERROR!! null str here but there shouldn't be!!"<<endl;

	MD5((unsigned char *)str, strlen(str), md);
	h=strrchr(pname,'/')+1;
	for (int c2=0; c2<16; c2++) {
		sprintf(h,"%02x",(int)md[c2]);
		h+=2;
	}
	*h='.';
	delete[] str;
	return pname;
}


} //namespace LaxFiles

