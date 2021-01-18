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
//    Copyright (C) 2004-2017 by Tom Lechner
//
#ifndef _LAX_STRMANIP_H
#define _LAX_STRMANIP_H


#include <cstring>
#include <cstdarg>

int isblank(const char *str);
int strcmp_safe(const char *s1, const char *s2);
int strncmp_safe(const char *s1, const char *s2, int n);
int strcasecmp_safe(const char *s1, const char *s2);
bool strEquals(const char *s1, const char *s2, bool caseless=false);
char *itoa(int a,char *dest,int base=10); /* does not null terminate */
char *newprintfstr(const char *fmt, ...);
char *numtostr(int num, int par=0);
char *numtostr(double num, int par=0); /* par=1 => (5) not 5 */
char *numtostr(char *dest,int buflen,double num,int par=0);
char *newstr(const char *str);
char *newnstr(const char *str,int n);
char *makestr(char *&dest,const char *src);
char *makenstr(char *&dest,const char *src,unsigned int n);
char *makestrmore(char *&dest, int slen, int num_bytes, bool isdiff);
char *insertstr(char *&dest,const char *data,int atpos);
char *prependnstr(char *&dest,const char *src,int n);
char *prependstr(char *&dest,const char *src);
char *appendnstr(char *&dest,const char *src,int n);
char *appendstr(char *&dest,const char *src);
char *appendintstr(char *&dest,int srci);
char *appendline(char *&dest,const char *src);
char *appendescaped(char *&dest, const char *src, char quote);
char *insertstr(char *&dest,const char *src,long pos);
char *insertnstr(char *&dest,const char *src,long len, long pos);
char *extendstr(char *&dest,int n);
char *extendstr(char *&dest,int &curmax,int n); /* assumes curmax>strlen */
char *stripws(char *dest,char where=3);
int has_outer_whitespace(const char *str);
int squish(char *exprs,int p1,int p2); /* remove p1-p2, including p2 */
						    /* doesn't create new str */
							/* returns #chars removed */
char *replace(char *&dest,const char *data,int s,int e,int *newe);
                                     /* put in [p1,p2], p2=new p2 */
char *replacefirst(const char *str, const char *old, const char *newstr);
char *replaceall(const char *dest,const char *old,const char *newn,int s=0,int e=-1);
char *replaceallname(const char *dest,const char *old,const char *newn);
char *getnamestring(const char *buf);
bool IsName(const char *name, const char *vlenstr, int len);
void deletestrs(char **&strs,int n);
char **splitspace(const char *stro,int *n_ret);
char **splitonspace(char *stro,int *n_ret);
char **split(const char *str,char delim,int *n_ret);
char **split(const char *str,const char *delim,int *n_ret);
char **spliton(char *str,char delim,int *n_ret);
int findInList(const char *name, const char **names);
int findInList(const char *name, const char **names, int numnames);

int is_absolute_path(const char *file);
const char *lax_basename(const char *path);
const char *lax_extension(const char *path);
char *lax_dirname(const char *path,char appendslash);
char *increment_file(const char *file);
char *chop_extension(char *file);

char *htmlchars_encode(const char *str, char *buffer, int len, int *len_ret);
char *htmlchars_decode(const char *str, char *buffer);


#endif

