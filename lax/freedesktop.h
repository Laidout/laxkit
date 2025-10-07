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
//    Copyright (C) 2007 by Tom Lechner
//
#ifndef _LAX_FREEDESKTOP_H
#define _LAX_FREEDESKTOP_H

#include <lax/attributes.h>
#include <lax/menuinfo.h>

namespace Laxkit {


int touch_recently_used_xbel(const char *file, const char *mime,
		const char *application,
		const char *app_exec,
		const char *group,
		bool visited,
		bool modified,
		const char *recent_file);
int touch_recently_used_old(const char *file, const char *mime, const char *group, const char *timestamp);
char *recently_used(const char *mimetype,const char *group, int includewhat);
Laxkit::MenuInfo *recently_used(const char *recentfile, const char *mimetype,const char *group, int includewhat, Laxkit::MenuInfo *existingmenu);

Laxkit::MenuInfo *get_categorized_bookmarks(const char *file,const char *filetype, Laxkit::MenuInfo *menu, bool flat);
int add_bookmark(const char *directory, int where);

char *freedesktop_get_existing_thumbnail(const char *file);
char *freedesktop_thumbnail_filename(const char *file, char which='n');
const char **freedesktop_thumbnail_dirs();
void freedesktop_md5(const unsigned char *data, int len, unsigned char *md5_ret);
int freedesktop_guess_thumb_size(const char *filename);


//-------------- XDG Basedir functions ------------------------

char *xdg_data_home();
char *xdg_config_home();
char *xdg_state_home();
const char *xdg_cache_home();
char *xdg_runtime_dir();
char **xdg_data_dirs(int *n_ret);
char **xdg_config_dirs(int *n_ret);


} //namespace

#endif


