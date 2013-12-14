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
//    Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef _LAX_FILEUTILS_H
#define _LAX_FILEUTILS_H

#include <cstdio>
#include <sys/stat.h>

namespace LaxFiles {

int how_indented(char *str,char **strt=NULL);
int getline_indent_nonblank(char **line, size_t *n,FILE *f, int indent, 
					const char *comment,char quote='"',char skiplines=1, int *lineindent=NULL);
int cut_comment(char *str,const char *cm="#",char quote='"');
int is_good_filename(const char *filename);
int check_dirs(const char *dirs,char make_too);
int lax_stat(const char *file, int followlink, struct stat *buf);
int file_exists(const char *filename, int followlink, int *error);
long file_size(const char *filename, int followlink, int *error);
char *sanitize_filename(char *filename,int makedup);
int readable_file(const char *filename,FILE **ff=NULL);
char *full_path_for_file(const char *file, const char *path);
char *convert_to_full_path(char *&file,const char *path);
char *relative_file(const char *file,const char *relativeto,char isdir);
int is_in_subdir(const char *file,const char *dir);
char *file_to_uri(const char *file);
char *expand_home(const char *file);
char *expand_home_inplace(char *&file);
char *simplify_path(char *file, int modorig=0);
char *make_filename_base(const char *f);
//void get_path_parts(const char *f,const char **dir,const char **file);
char *read_in_whole_file(const char *file, int *chars_ret, int maxchars=0);
	
char *get_bookmarks(const char *file,const char *filetype);

#ifdef _LAX_PLATFORM_MAC
int getline(char **line, size_t *n,FILE *f);
#endif  //_LAX_PLATFORM_MAC

} //namespace LaxFiles

#endif

