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
//    Copyright (C) 2004-2007 by Tom Lechner
//
#ifndef _LAX_FILEUTILS_H
#define _LAX_FILEUTILS_H

#include <lax/iobuffer.h>

#include <cstdio>
#include <sys/stat.h>



namespace LaxFiles {

#ifdef _LAX_PLATFORM_MAC
int getline(char **line, size_t *n,FILE *f);
#endif  //_LAX_PLATFORM_MAC

int how_indented(char *str,char **strt=NULL);
int getline_indent_nonblank(char **line, size_t *n, IOBuffer &f, int indent, 
					const char *comment,char quote='"',char skiplines=1, int *lineindent=NULL);

int cut_comment(char *str,const char *cm="#",char quote='"');
int is_good_filename(const char *filename);
int check_dirs(const char *dirs, bool make_too, int permissions = 0700);
int CheckDirs(const char *dirstr, int depth, int permissions = 0700);
int lax_stat(const char *file, int followlink, struct stat *buf);
int file_exists(const char *filename, int followlink, int *error);
long file_size(const char *filename, int followlink, int *error);
char *sanitize_filename(char *filename,int makedup);
int readable_file(const char *filename,FILE **ff=NULL);
char *full_path_for_file(const char *file, const char *path);
char *convert_to_full_path(char *&file,const char *path);
char *relative_file(const char *file,const char *relativeto,char isdir);
bool is_relative_path(const char *file);
int is_in_subdir(const char *file,const char *dir);
char *file_to_uri(const char *file);
char *expand_home(const char *file);
char *expand_home_inplace(char *&file);
char *simplify_path(const char *file);
char *simplify_path(char *file, int modorig);
char *make_filename_base(const char *f);
//void get_path_parts(const char *f,const char **dir,const char **file);
char *read_in_whole_file(const char *file, int *chars_ret, int maxchars=0);
char *pipe_in_whole_file(FILE *f, int *chars_read_ret);
int save_string_to_file(const char *str,int n, const char *file);
char *current_directory();
char *ExecutablePath();

} //namespace LaxFiles

#endif

