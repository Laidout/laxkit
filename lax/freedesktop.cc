//
//	
//    The Laxkit, a windowing toolkit
//    Please consult https://github.com/Laidout/laxkit about where to send any
//    correspondence about this software.
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 3 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; If not, see <http://www.gnu.org/licenses/>.
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
#include <lax/language.h>

#include <lax/lists.cc>


#define DBG
#include <iostream>
using namespace std;


namespace LaxFiles {

/*! If visited or modified are not null, then update those in the recent file to the current time.
 *
 * file must be an absolute path.
 *
 * If recent_file==NULL, then use "~/.local/share/recently-used.xbel".
 *
 * See the xbel spec here: http://www.freedesktop.org/wiki/Specifications/desktop-bookmark-spec
 */
int touch_recently_used_xbel(const char *file, const char *mime,
		const char *application,
		const char *app_exec,
		const char *group,
		bool visited,
		bool modified,
		const char *recent_file)
{
	if (!file) return 1;

	 //make sure to use uri rather than file
	char *uri=newstr(file);
	if (strncmp(uri,"file://",7)) prependstr(uri,"file://");
	if (uri[7]!='/') {
		delete[] uri; //was not an absolute path
		return 2;
	}

	 //find visited
	char visited_time[50];
	visited_time[0]='\0';
	if (visited) {
		time_t t;
		time(&t);
		strftime(visited_time, 50, "%FT%TZ", gmtime(&t));
	}

	 //find modified
	char mod_time[50];
	mod_time[0]='\0';
	if (modified) {
		time_t t;
		time(&t);
		strftime(mod_time, 50, "%FT%TZ", gmtime(&t));
	}

	 //read in recently-used
	char *recentfile=expand_home("~/.local/share/recently-used.xbel");
	Attribute *recent=XMLFileToAttributeLocked(NULL,recentfile,NULL);
	Attribute *content;

	if (!recent) {
		 //problem reading recentfile, or file does not exist
		recent=new Attribute;
		recent->push("?xml",(char*)NULL);
		recent->attributes.e[0]->push("version","1.0");
		recent->attributes.e[0]->push("encoding","UTF-8");
		content = recent->pushSubAtt("xbel",NULL);
		content = content->pushSubAtt("content:",NULL);

	} else {
		 //find RecentFiles->content: 
		content=recent->find("xbel");
		if (content) content=content->find("content:");
		if (!content) {
			 //error, couldn't find bookmarks!
			delete[] uri;
			delete recent;
			delete[] recentfile;
			return 3;
		}

	}
	
	 //search for file
	Attribute *item=NULL,*att=NULL;
	int c;
	int oldindex=-1;

	for (c=0; c<content->attributes.n; c++) {
		if (strcmp(content->attributes.e[c]->name,"bookmark")) continue;
		item=content->attributes.e[c];
		att=item->find("href");
		if (att && !strcmp(att->value,uri)) break;
		item=NULL;
	}

	if (item) {
		oldindex=c;

		 //file found, so update it.
		 //  Each bookmark has 3 time fields: added, modified, and visited
		 //  they all have format "2015-11-11T15:01:32Z" (iso 8601)

		 //with newer(?) timestamp
		if (visited) {
			att=item->find("visited");
			if (att && strcmp(att->value, visited_time)<0)
				makestr(att->value, visited_time);
		}

		if (modified) {
			att=item->find("modified");
			if (att && strcmp(att->value, mod_time)<0)
				makestr(att->value, mod_time);
		}

		 //now update meta section...
		Attribute *meta=item->find("content:");
		if (!meta) meta=item->pushSubAtt("content:");
		att = meta->find("info");
		if (!att) att=meta->pushSubAtt("info");
		meta = att;
		att  = meta->find("content:");
		if (!att) att=meta->pushSubAtt("content:");
		meta = att;
		att  = meta->find("metadata");
		if (!att) att = meta->pushSubAtt("metadata");
		meta = att;
		att = meta->find("content:");
		if (!att) att=meta->pushSubAtt("content:");
		meta = att;

		 //hopefully gratuitous mime type check
		att=meta->find("mime:mime-type");
		if (att) att=att->find("type");
		if (att && strcmp(att->value,mime)) {
			DBG cerr <<"***warning: mime type not same as before on "<<file<<endl;
			makestr(att->value,mime);
		}

		 //make sure group is in there
		if (group) {
			Attribute *gatt=meta->find("bookmark:groups");
			if (!gatt) {
				 //need to add a groups sections
				gatt = meta->pushSubAtt("bookmark:groups");
			}
			
			att=gatt;
			gatt=att->find("content:");
			if (!gatt) gatt = att = att->pushSubAtt("content:");

			for (c=0; c<gatt->attributes.n; c++) {
				att = gatt->attributes.e[c]->find("content:"); //should be <bookmark:group>CONTENT</bookmark:group>
				if (!att) att=gatt->attributes.e[c];
				if (att && att->value && !strcmp(att->value,group)) break; //found it!
			}
			if (c == gatt->attributes.n) att->push("bookmark:group",group); //wasn't found, so add it!
		}

		 //application
		if (application) {
			Attribute *gatt=meta->find("bookmark:applications");
			if (!gatt) {
				 //need to add an applications sections
				gatt = meta->pushSubAtt("bookmark:applications");
			}
			
			att=gatt;
			gatt=att->find("content:");
			if (!gatt) gatt = att = att->pushSubAtt("content:");

			for (c=0; c<gatt->attributes.n; c++) {
				att=gatt->attributes.e[c];
				Attribute *att2=att->find("name");
				if (att2 && !strcmp(att2->value,application)) {
					//found it!  update count and mod time
					if (modified) {
						att2=att->find("modified");
						if (att2) makestr(att2->value, mod_time);
						else att2=att->pushSubAtt("modified", mod_time);
					}
					att2=att->find("count");
					if (att2) {
						int count=strtol(att2->value,NULL,10);
						count++;
						char str[10];
						sprintf(str,"%d",count);
						makestr(att2->value,str);
					} else makestr(att2->value, "1");
					break;
				}
			}

			if (c==att->attributes.n) {
				 //app not found, need to add a fresh application section
				att = gatt->pushSubAtt("bookmark:application");
				att->push("name", application);
				att->push("exec", app_exec);
				if (!modified) {
					time_t t;
					time(&t);
					strftime(mod_time, 50, "%FT%TZ", gmtime(&t));
				}
				att->push("modified", mod_time);
				att->push("count", "1");
			}
		}

		if (oldindex>0) {
			//move to the end of file
			Attribute *it=content->attributes.pop(oldindex);
			content->push(it,-1);
		}


	} else {
		 //file not found, add to list 
		item = content->pushSubAtt("bookmark",NULL);
		item->push("href",uri);

		if (!visited) {
			time_t t;
			time(&t);
			strftime(visited_time, 50, "%FT%TZ", gmtime(&t));
		}

		item->push("added",   visited_time);
		item->push("modified",mod_time);
		item->push("visited", visited_time);

		item = item->pushSubAtt("content:");
		item = item->pushSubAtt("info");
		item = item->pushSubAtt("content:");
		item = item->pushSubAtt("metadata");
		item->push("owner","http://freedesktop.org");
		item = item->pushSubAtt("content:");
		att = item->pushSubAtt("mime:mime-type",NULL);
		att->push("type", mime);

		if (group) {
			att=item->pushSubAtt("bookmark:groups");
			att=att->pushSubAtt("content:");
			att->push("bookmark:group",group);
		}

		if (application) {
			att=item->pushSubAtt("bookmark:applications");
			att=att->pushSubAtt("content:");
			att=att->pushSubAtt("bookmark:application");
			if (app_exec) {
				att->push("name",application);
				char str[strlen(app_exec) + 20];
				sprintf(str,"&apos;%s %%u&apos;", app_exec);
				att->push("exec", str);
			}
			att->push("count", "1"); 
		}
	}

	 //make sure < 500 items
	while (content->attributes.n>500) {
		 //find oldest one (assuming the first one)
//		-------------
		c=0;
		while (c<content->attributes.n && strcmp(content->attributes.e[c]->name, "bookmark")) c++;

		if (c>=0) content->attributes.remove(c); //the oldest one hopefully

//		-------------if out of order: (*** needs updating)
//		Attribute *att2;
//		c=content->attributes.n-1;
//		att=content->attributes.e[c]->attributes.e[0]->find("visited");
//		int c2;
//		long l=strtol(att->value,NULL,0),l2;
//		for (c2=0; c2<content->attributes.n; c2++) {
//			att2=content->attributes.e[c2]->attributes.e[0]->find("Timestamp");
//			l2=strtol(att2->value,NULL,10);
//			if (l2<l) { 
//				c=c2;
//				l=l2;
//			}
//		}
//
//		if (c>=0) content->attributes.remove(c); //the oldest one
	}

	
	 //write back out recently-used
	setlocale(LC_ALL,"C");
	int fd=open(recentfile,O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
	delete[] recentfile;
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

/*! If timestamp==NULL, then use the current time. Otherwise it must be a string 
 * representing the number of seconds since the epoch, which is the beginning of
 * Jan 1, 1970 UTC. See time gettimeofday for more info about that.
 *
 * file must be an absolute path.
 *
 * \todo there could be more error checking in here. If recent file spec changes, could well break this.
 * \todo support the more commonly used recent file: ~/.local/share/recently-used.xbel
 */
int touch_recently_used_old(const char *file, const char *mime, const char *group, const char *timestamp)
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
	char *recentfile=expand_home("~/.recently-used");
	//char *recentfile=expand_home("~/.local/share/recently-used.xbel");
	Attribute *recent=XMLFileToAttributeLocked(NULL,recentfile,NULL);
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
			delete[] recentfile;
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
	int fd=open(recentfile,O_CREAT|O_WRONLY|O_TRUNC,S_IREAD|S_IWRITE);
	delete[] recentfile;
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
 * which puts info into ~/.local/share/recently-used.xbel instead. Apparently, this one
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
 * NOT IMPLEMENTED HERE, use the other recently_used:
 * If you instead to read from ~/.local/share/recently-used.xbel, then an attribute of the following
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
 * \todo support the other commonly used recent file: ~/.local/share/recently-used.xbel
 * \todo not a whole lot of error checking in here yet.
 */
char *recently_used(const char *mimetype,const char *group, int includewhat)
{
	char *name=expand_home("~/.recently-used");
	//char *name=expand_home("~/.local/share/recently-used.xbel");
	Attribute *mainatt=XMLFileToAttributeLocked(NULL,name,NULL);
	delete[] name;
	if (!mainatt) return NULL;

	char *recent=NULL;

	int priv,ingroup=0;
	char *value;
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

			 //convert "%20" to " "
			char *ss=file;
			int i=0;
			while (file[i]) {
				if (file[i]=='%' && file[i+1]=='2' && file[i+2]=='0') {
					*ss=' ';
					ss++;
					i+=3;
				} else {
					*ss=file[i];
					ss++;
					i++;
				}
			}
			*ss='\0';

			appendstr(recent,file); appendstr(recent,"\n"); 
		}
	}

	delete mainatt;
	return recent;
}

//! Return a MenuInfo of recent files.
/*! If mimetype!=NULL, then return only items of that type.
 * If group!=NULL, then return items in only that group. If there are private
 * items in the raw list, then return them only if its group matches the group
 * you are looking for.
 *
 * If includewhat==0, then only return the file name. 
 * If includewhat&1, next detail is mime type.
 * If includewhat&2, next detail is timestamp.
 *
 * If recentfile is NULL, then try ~/.local/share/recently-used.xbel.
 * If an <xbel> is found, it is assumed the xbel format is used.
 * If a <RecentFiles> is found, then assume the file conforms to 
 * the Recently Used Specification at Freedesktop.org:
 * http://www.freedesktop.org/wiki/Specifications/recent-file-spec
 * This would be in ~/.recently-used, for instance.
 *
 * Please note that many programs are not using the RecentFiles spec.
 * Many are using the xbel, Freedesktop bookmark spec:
 * http://www.freedesktop.org/wiki/Specifications/desktop-bookmark-spec,
 * which puts info into ~/.local/share/recently-used.xbel instead.
 *
 * If you choose to read from ~/.recently-used, then it expects that file to return
 * at Attribute structured basically like this: *
 *
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
 * If you instead to read from ~/.local/share/recently-used.xbel, then an attribute of the following
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
 * \todo support the other commonly used recent file: ~/.local/share/recently-used.xbel
 * \todo not a whole lot of error checking in here yet.
 */
Laxkit::MenuInfo *recently_used(const char *recentfile, const char *mimetype,const char *group, int includewhat, Laxkit::MenuInfo *existingmenu)
{
	Laxkit::MenuInfo *menu=existingmenu;
	if (!menu) menu=new Laxkit::MenuInfo(_("Recent"));

	char *rfile=NULL;
	//if (!recentfile) recentfile = expand_home("~/.recently-used");
	if (!recentfile) rfile = expand_home("~/.local/share/recently-used.xbel");

	Attribute *mainatt=XMLFileToAttributeLocked(NULL,rfile,NULL);
	delete[] rfile;

	if (!mainatt) { 
		if (!existingmenu) delete menu;
		return NULL;
	}


	int priv=0,ingroup=0;
	char *name, *value;
	char *mime,*file,*timestamp;
	Attribute *att2;
	Attribute *att=mainatt->find("RecentFiles");

	if (att) {
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

			if (file) { 
				if (!strncmp(file,"file://",7)) file+=7;

				 //convert "%20" to " "
				char *ss=file;
				int i=0;
				while (file[i]) {
					if (file[i]=='%' && file[i+1]=='2' && file[i+2]=='0') {
						*ss=' ';
						ss++;
						i+=3;
					} else {
						*ss=file[i];
						ss++;
						i++;
					}
				}
				*ss='\0';

				menu->AddItem(file);
				if (mime && includewhat&1)      { menu->AddDetail(mime,NULL); }
				if (timestamp && includewhat&2) { menu->AddDetail(timestamp,NULL); }
			}
		}

	} else if (att=mainatt->find("xbel"), att!=NULL) {
		att=att->find("content:");

		for (int c=att->attributes.n-1; c>=0; c--) { //foreach RecentItem
			if (strcmp(att->attributes.e[c]->name,"bookmark")) continue;

			mime=file=timestamp=NULL;
			ingroup=0;

			//att2=att->attributes.e[c]->find("content:");//the RecentItem->content: attribute
			att2=att->attributes.e[c];

			for (int c2=0; c2<att2->attributes.n; c2++) {
				name =att2->attributes.e[c2]->name;
				value=att2->attributes.e[c2]->value;

				if (!strcmp(name,"href")) {
					file=value;

				} else if (!strcmp(name,"modified")) {
					timestamp=value;

				} else if (!strcmp(name,"content:") && ((includewhat&1)!=0 || mimetype || group)) {
					try {
						Attribute *att3=att2->attributes.e[c2];
						att3=att3->find("info");
						if (att3) att3=att3->find("content:");
						if (att3) att3=att3->find("metadata");
						if (att3) att3=att3->find("content:");
						if (!att3) throw(1);

						 //find mime
						if ((includewhat&1)!=0) {
							Attribute *att4=att3->find("mime:mime-type");
							if (att4) att4=att4->find("type");
							if (att4) mime=att4->value;
						}

						 //find group
						if (group) {
							att3=att3->find("bookmark:groups");
							if (att3) att3=att3->find("content:");
							if (att3) { 
								for (int c3=0; c3<att3->attributes.n; c3++) {
									name =att3->attributes.e[c3]->name;
									value=att3->attributes.e[c3]->value;

									if (value && !strcmp(name,"bookmark:group")) {
										if (!strcmp(value,group)) { ingroup=1; break; }
									} 
								}
							}
						}
					} catch (int err) {
						DBG if (err>0) cerr <<"Missing component in recent file! ("<<err<<")"<<endl;
					}

				} else if (!strcmp(name,"Mime-Type")) {
					if (mimetype && strcmp(mimetype,value)) continue; //ignore unwanted mimes
					mime=value; 
				}
			}

			if (mimetype && (!mime || strcmp(mimetype,mime))) continue; //doesn't match specified mime type
			if (group && !ingroup) continue; //record only a particular group

			if (file) { 
				if (!strncmp(file,"file://",7)) file+=7;

				 //convert "%20" to " "
				char *ss=file;
				int i=0;
				while (file[i]) {
					if (file[i]=='%' && file[i+1]=='2' && file[i+2]=='0') {
						*ss=' ';
						ss++;
						i+=3;
					} else {
						*ss=file[i];
						ss++;
						i++;
					}
				}
				*ss='\0';

				menu->AddItem(file);
				if (mime && includewhat&1)      { menu->AddDetail(mime,NULL); }
				if (timestamp && includewhat&2) { menu->AddDetail(timestamp,NULL); }
			}
		}
	}


	delete mainatt;
	return menu;
}


/*! Read in file bookmarks, add to menu. Each item has 2 parts.
 * Main part is basename. Has one detail node, containing full path.
 *
 * filetype will be "gtk", "gtk3", "kde", "rox", "lax".
 *
 * If file==NULL and filetype==NULL, then each of those filetypes (but only "gtk3", NOT "gtk")
 * is used, and put in subitems.
 * If file==NULL, then read in the default locations for filetype, which are:
 *
 * If file!=NULL and filetype==NULL, then NULL is returned.
 *
 * If file!=NULL, then read in that file. Needs filetype!=NULL.
 * If no bookmarks are added, then return NULL.
 *
 * gtk:  ~/.gtk-bookmarks\n
 * gtk3: ~/.config/gtk-3.0/bookmarks\n
 * kde:  ~/.kde/share/apps/kfileplaces/bookmarks.xml\n
 * rox:  ~/.config/rox.sourceforge.net/ROX-Filer/Bookmarks.xml\n
 * lax: (no default file at this time)\n
 *
 * possibly fd desktop bookmark spec says folder bookmarks in ~/.shortcuts.xbel, but this is not
 * implemented here.
 *
 * If flat, then for filetype==NULL, then add to menu without adding submenus. Each section
 * is delineated with a separator.
 *
 * \todo need to lock files on opening
 */
Laxkit::MenuInfo *get_categorized_bookmarks(const char *file,const char *filetype, Laxkit::MenuInfo *menu, bool flat)
{
	if (filetype==NULL && file!=NULL) return NULL;
	if (filetype==NULL) {
		 //scan all common locations
		int created=0;
		int n=0;
		if (!menu) { created=1; menu=new Laxkit::MenuInfo; }
		Laxkit::MenuInfo *mm;


		if (flat) menu->AddSep(_("Gtk"));
		else {
			menu->AddItem(_("Gtk"));
			menu->SubMenu();
		}
		mm=get_categorized_bookmarks(NULL,"gtk3", menu, false);
		if (!flat) menu->EndSubMenu();
		if (!mm) menu->Remove(-1);
		else n++;


		if (flat) menu->AddSep(_("Kde"));
		else {
			menu->AddItem(_("Kde"));
			menu->SubMenu();
		}
		mm=get_categorized_bookmarks(NULL,"kde",  menu, false);
		if (!flat) menu->EndSubMenu();
		if (!mm) menu->Remove(-1);
		else n++;


		if (flat) menu->AddSep(_("Rox"));
		else {
			menu->AddItem(_("Rox"));
			menu->SubMenu();
		}
		mm=get_categorized_bookmarks(NULL,"rox",  menu, false);
		if (!flat) menu->EndSubMenu();
		if (!mm) menu->Remove(-1);
		else n++;


		//if (flat) menu->AddItem(_("Lax"));
		//else {
		//	menu->AddItem(_("Lax"));
		//	menu->SubMenu();
		//}
		//mm=get_categorized_bookmarks(NULL,"lax",  menu, false);
		//if (!flat) menu->EndSubMenu();
		//if (!mm) menu->Remove(-1);
		//else n++;


		if (!n && created) { delete menu; menu=NULL; } //nothing added
		return menu;
	}


	if (file==NULL) {
		 //load from standard bookmarks location
		if (!strcmp(filetype,"gtk")) file="~/.gtk-bookmarks";
		else if (!strcmp(filetype,"gtk3")) file="~/.config/gtk-3.0/bookmarks";
		//else if (!strcmp(filetype,"kde")) file="~/.kde/share/apps/konqueror/bookmarks.xml";
		else if (!strcmp(filetype,"kde")) file="~/.kde/share/apps/kfileplaces/bookmarks.xml";
		else if (!strcmp(filetype,"rox")) file="~/.config/rox.sourceforge.net/ROX-Filer/Bookmarks.xml";
	}
	if (!file) return NULL;


	char *ff=newstr(file);
	expand_home_inplace(ff);
	if (file_exists(ff,1,NULL)!=S_IFREG) return NULL;
	FILE *f=fopen(ff,"r");
	delete[] ff;
	if (!f) return NULL;


	int created=0;
	int n=0;
	if (!menu) { menu=new Laxkit::MenuInfo(); created=1; }


	if (!strcmp(filetype,"kde") || !strcmp(filetype,"rox")) {
		Attribute att;
		Attribute *attt, *satt;
		XMLChunkToAttribute(&att,f,NULL);

		attt=att.find("xbel");
		if (attt) attt=attt->find("content:");

		if (attt) {
			const char *value, *name;

			for (int c=0; c<attt->attributes.n; c++) {
				name =attt->attributes.e[c]->name;
				value=attt->attributes.e[c]->value;

				if (!strcmp(name,"bookmark")) {
					satt=attt->attributes.e[c]->find("href");
					if (satt && !strncmp(satt->value,"file://",7)) {
						value=satt->value+7;

						if (!isblank(value)) {
							n++;
							menu->AddItem(isblank(lax_basename(value)) ? value : lax_basename(value));
							menu->AddDetail(value,NULL);
						}
					} 
				}
			}
		}

	} else if (!strcmp(filetype,"gtk") || !strcmp(filetype,"gtk3")) {
		 //gtk uses (as far as I know) a simple list of lines like:   
		 //  file:///blah/blah optional-name
		char *line=NULL;
		char *name;
		size_t nn=0;
		int c;

		while (1) {
			c=getline(&line,&nn,f);
			if (c<=0) break;
			if (line[c-1]=='\n') line[c-1]='\0';
			if (!isblank(line)) {
				 //parse optional name
				name=strchr(line,' ');
				if (name && !isblank(name+1)) {
					*name='\0';
					name++;
				} else {
					name=(char*)lax_basename(line+7);
				}

				 //spaces in paths are coded as "%20" so...
				char *ss=line;
				int i=0;
				while (line[i]) {
					if (line[i]=='%' && line[i+1]=='2' && line[i+2]=='0') {
						*ss=' ';
						ss++;
						i+=3;
					} else {
						*ss=line[i];
						ss++;
						i++;
					}
				}
				*ss='\0';

				n++;
				menu->AddItem(name);
				menu->AddDetail(line+7,NULL);
			}
		}

	} else if (!strcmp(filetype,"lax")) {
		 //is lax indented format
		//Attribute *att=anXApp::app->Resource("Bookmarks");
	}

	if (created && !n) { delete menu; menu=NULL; }

	fclose(f);
	return menu;
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
	cerr << " *** freedesktop.cc/add_bookmark() either needs a ton of work or is deprecated!!"<<endl;

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

