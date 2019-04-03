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


// NOTE this is c++ code, using new and delete, not malloc and free


#include <cstdio>
#include <cstring>
#include <cctype>

//#include <iostream>
//using namespace std;

#include <lax/strmanip.h>

//*** what the hell with SAFE? relic from a forgotten age?
#define SAFE	5


/*! \defgroup Strman String Manipulation Functions
 *	This file defines various utilities for manipulating the old character strings 
 *	in C++ using new and delete operators. The C++ Standard Template Library (STL)
 *	does not handle utf8 and unicode, and these functions provide convenient low level
 *	string manipulation that is easily adapted to utf8, without the large 
 *	overhead of the STL.
 * 
 * \todo ***convert to using memcpy and memmove instead of stuff like s[c]=s[c+1]
 * \todo *** some functions that have a dest and a src must be careful not to
 *   delete[] dest if src==dest, must go through and check for that
 * \todo make a strprintf(&str,"format",args), would have to step through format, and
 *   figure out how big a string is needed, then allocate it for str.
 * \todo perhaps wrap this in LaxStr namespace?
 * @{
 */


//! Nonzero when str is NULL (returns 1), "" (2), or a string of whitespace (3). Else 0.
///*! Checks up to n chars. If n<=0 then use strlen(str).
// */
int isblank(const char *str) //,int n)
{
	if (!str) return 1;
	if (*str=='\0') return 2;
	//if (n<=0) n=strlen(str);
	while (isspace(*str)) str++;
	if (*str=='\0') return 3;
	return 0;
}

/*! Return a new char[] from printf style arguments.
 */
char *newprintfstr(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    int c = vsnprintf(NULL, 0, fmt, arg);
    va_end(arg);

    char *message = new char[c+1+10];
    va_start(arg, fmt);
    vsnprintf(message, c+1, fmt, arg);
    va_end(arg);
    return message;
}


//! This turns an integer into a string with any base.
/*! base can be any number greater than 1 and less than 37.
 * For bases greater than 10, lowercase letters are used.
 * Does not null terminate, and assumes that dest is big
 * enough to receive the digits.
 * 
 * \return Returns a pointer to the character after the final
 *    character written into dest.
 */
// *** make dest=buf, use buflen
// returns pointer to char after last digit written
char *itoa(int a,char *dest,int base) // does not null terminate
{
	if (dest==NULL || base<2 || base>36) return NULL;
	int len=0,c,n=0;
	if (a<0) { n=-1; a=-a; }
	do {
		c=a%base;
		a/=base;
		if (c<10) dest[len++]='0'+c;
		else dest[len++]='a'+c-10;
	} while (a!=0);	
	if (n<0) dest[len++]='-';
	if (len>1) {
		char ch;
		for (int cc=0; cc<len/2; cc++)
		{ 	ch=dest[cc]; 
			dest[cc]=dest[len-cc-1]; 
			dest[len-cc-1]=ch;
		}
	}
	//dest[len]='\0';
	return dest+len;
}

//! Basic conversion of integer to string, created with new.
/*! If par!=0, parentheses are put around the number.
 *
 * \return Returns the new'd char array.
 */
char *numtostr(int num, int par)//par=0
{
	char *dest=new char[10];
	if (par) sprintf(dest,"(%d)",num); // sprintf puts the trailing 0
	  else sprintf(dest,"%d",num);
	return dest;	   
}

 //! Removes characters p1 to p2 inclusive: [p1,p2].
 /*! Squish doesn't create new array, it just copies the later
  *  characters into previous positions.
  *
  * \return Returns the number of characters removed.
  */
int squish(char *exprs,int p1,int p2)
{		
	if (p2<=p1 || p1<0 || p2+1>(int)strlen(exprs)) return 0;
	int sl=strlen(exprs);
	for (int c=0; c<sl-p2; c++)
		exprs[p1+c]=exprs[p2+c+1];
	return p2-p1+1;	
}

//! Converts a double into a char string, removing trailing zeroes.
/*! if par!=0, parentheses are put around the number.
 *  A precision of 13 decimal places is used. The resulting
 *  string is put into the supplied dest, not exceeding buflen
 *  characters (including null termination).  
 *  and no new string is created.
 *
 *  ***** ignores buflen, what about printf("%g")??
 */
char *numtostr(char *dest,int buflen,double num,int par)//par=0
{
	if (par) sprintf(dest,"(%.13lf)",num); // sprintf puts trailing \0
	  else sprintf(dest,"%.13lf",num);	   
	int p1,d=0,c;
	for (c=0; c<(int)strlen(dest); c++) {
		if (dest[c]=='e' || dest[c]=='E') break;
		if (dest[c]=='.') d=1;
	}
	c--; if (par) c--;
	p1=c;
	if (d) { // remove trailing zeros
		while (dest[p1]=='0') p1--;
		if (dest[p1]=='.') p1--;
		if (c-p1>0) squish(dest,p1+1,c);
	}
	return dest;
}


//! Converts a double into a char string, removing trailing zeroes.
/*! if par!=0, parentheses are put around the number.
 *  A precision of 13 decimal places is used. The resulting
 *  null terminated string is put into a new char[].
 */
char *numtostr(double num, int par) // par=0,   par=1 => (5) not 5 
{
	char *dest=new char[80];			 
	numtostr(dest,80,num,par);
	return dest;
}

//! Return a new'd, null terminated duplicate of the first n characters of str.
/*! If str==NULL and n>0, then return a new char[n] with the first byte 0.
 *
 * If n==0, then return an empty string (""), not NULL.
 * If n<0, and str!=NULL, then copy the whole string.
 * If n<0 and str==NULL, then return NULL.
 */
char *newnstr(const char *str,int n)
{
	if (!str) {
		if (n<=0) return NULL;
		else {
			char *s=new char[n];
			s[0]='\0';
			return s;
		}
	}
	if (n==0) return newstr("");
	if (n>(int)strlen(str)) n=strlen(str);
	if (n<0) n=strlen(str);
	char *dup=new char[n+1];
	strncpy(dup,str,n);
	dup[n]='\0';
	return dup;
}

//! Return a new'd duplicate of str.
/*! If str==NULL, then return NULL.
 */
char *newstr(const char *str)
{
	if (!str) return NULL;
	char *dup = new char[strlen(str)+1];
	strcpy(dup,str);
	return dup;
}

 //! Make dest a new copy of src. 
 /*! If dest is not NULL, dest is deleted first.
  *  If src is NULL, then dest is also made NULL.
  *
  *  \return Returns a pointer to what dest now points to.
  */
char *makestr(char *&dest,const char *src)
{		    
	if (dest==src) return dest;
	if (dest) delete[] dest;
	if (!src) { dest=NULL; return NULL; }
	dest=new char[strlen(src)+1+SAFE];
	return strcpy(dest,src);
}

/*! Basically realloc(). If num_bytes would result in a smaller allocation, nothing is done.
 * If isdiff, then add num_bytes to strlen. Else allocate that num_bytes in new string.
 */
char *makestrmore(char *&dest, int slen, int num_bytes, bool isdiff)
{
	if (num_bytes <= 0) return dest;
	if (slen < 0) slen = (dest ? strlen(dest) : 0);
	if (!isdiff && num_bytes < slen+1) return dest;
	if (isdiff) slen = slen+1 + num_bytes; else slen = num_bytes;
	char *newdest = new char[num_bytes];
	if (dest) strcpy(newdest, dest); else newdest[0]='\0';
	delete[] dest;
	dest = newdest;
	return dest;
}

 //! Like makestr, but only grabs the first n characters of src.
 /*! \return Returns a pointer to what dest now points to.
  *
  * If src==NULL and n>0, then make dest point to a new char[n], with
  * str[0]=0.
  *
  * If n<0, then use strlen(src) instead.
  */
char *makenstr(char *&dest,const char *src,unsigned int n)
{		     
	if (n<0) n=strlen(src);
	if (dest==src) {
		if (dest && n<strlen(dest)) dest[n]='\0';
		return dest;
	}
	delete[] dest;
	if (!src) { 
		if (n) { dest=new char[n]; dest[0]='\0'; }
		else dest=NULL; 
		return dest; 
	}
	if (strlen(src)<n) n=strlen(src);
	dest=new char[n+1+SAFE];
	strncpy(dest,src,n);
	dest[n]='\0';
	return dest;
}

//! Append the first n characters of src to dest.
/*! If dest is not NULL, delete[] dest is called. The new'd string
 *  is assigned to dest, which is returned also.
 *
 *  If n<=0 nothing is done. If n>strlen(src) then strlen(src)
 *  is used instead.
 *
 *  \todo *** question: is *&dest allowed to be NULL?
 */
char *appendnstr(char *&dest,const char *src,int n)
{
	if (!src || n<=0) return dest;
	char *newdest;
	if (n>(int)strlen(src)) n=(int)strlen(src);
	if (!dest) {
		dest=new char[n+1+SAFE];
		strncpy(dest,src,n);
		dest[n]='\0';
	} else {
		newdest=new char[strlen(dest)+n+1+SAFE];
		strcpy(newdest,dest);
		strncat(newdest,src,n);
		delete[] dest;
		dest=newdest;
	}
	return dest;
}

//! Prepend the first n characters of src to dest.
/*! If dest is not NULL, delete[] dest is called. The new'd string
 *  is assigned to dest, which is returned also.
 *
 *  If n<=0 nothing is done. If n>strlen(src) then strlen(src)
 *  is used instead.
 */
char *prependnstr(char *&dest,const char *src,int n)
{
	if (!src || n<=0) return dest;
	char *newdest;
	if (n>(int)strlen(src)) n=(int)strlen(src);
	if (!dest) {
		dest=new char[n+1+SAFE];
		strncpy(dest,src,n);
		dest[n]='\0';
	} else {
		newdest=new char[strlen(dest)+n+1+SAFE];
		strncpy(newdest,src,n);
		strcpy(newdest+n,dest);
		delete[] dest;
		dest=newdest;
	}
	return dest;
}

 //! Append src to dest, with an extra '\\n' between if dest!=NULL to start.
 /*! If dest is not NULL, delete[] dest is called. The new'd string
  *  is assigned to dest, which is returned also.
  *
  *  This is useful, for instance, if you want to concatenate many error
  *  messages, and don't want to worry about trailing newlines.
  */
char *appendline(char *&dest,const char *src)
{
	if (!src) return dest;
	char *newdest;
	if (!dest) {
		dest=new char[strlen(src)+1+SAFE];
		strcpy(dest,src);
	} else {
		newdest=new char[strlen(dest)+strlen(src)+2];
		sprintf(newdest,"%s%s%s",
				dest,
				(dest[strlen(dest)-1]!='\n'?"\n":""),
				src);
		delete[] dest;
		dest=newdest;
	}
	return dest;
}
	
 //! Append src to dest.
 /*! If dest is not NULL, delete[] dest is called. The new'd string
  *  is assigned to dest, which is returned also.
  */
char *appendstr(char *&dest,const char *src)
{
	if (!src) return dest;
	char *newdest;
	if (!dest) {
		dest=new char[strlen(src)+1+SAFE];
		strcpy(dest,src);
	} else {
		newdest=new char[strlen(dest)+strlen(src)+1+SAFE];
		strcpy(newdest,dest);
		strcat(newdest,src);
		delete[] dest;
		dest=newdest;
	}
	return dest;
}

 //! Append src to dest.
 /*! If dest is not NULL, delete[] dest is called. The new'd string
  *  is assigned to dest, which is returned also.
  */
char *appendintstr(char *&dest,int srci)
{
	char src[20];
	sprintf(src,"%d",srci);
	char *newdest;
	if (!dest) {
		dest=new char[strlen(src)+1+SAFE];
		strcpy(dest,src);
	} else {
		newdest=new char[strlen(dest)+strlen(src)+1+SAFE];
		strcpy(newdest,dest);
		strcat(newdest,src);
		delete[] dest;
		dest=newdest;
	}
	return dest;
}

 //! Prepend src to dest (returning srcdest).
 /*! If dest is not NULL, delete[] dest is called. The new'd string
  *  is assigned to dest, which is returned also.
  */
char *prependstr(char *&dest,const char *src)
{
	if (!src) return dest;
	char *newdest;
	if (!dest) {
		dest=new char[strlen(src)+1+SAFE];
		strcpy(dest,src);
	} else { 
		newdest=new char[strlen(dest)+strlen(src)+1+SAFE];
		strcpy(newdest,src);
		strcat(newdest,dest);
		delete[] dest;
		dest=newdest;
	}
	return dest;
}

char *insertstr(char *&dest,const char *src,long pos)
{
	if (!src) return dest;
	return insertnstr(dest,src,strlen(src),pos);
}

char *insertnstr(char *&dest,const char *src,long len, long pos)
{
	if (!src || len==0) return dest;

	if (len<0) len=strlen(src);

	long destlen=strlen(dest);
	if (pos<0 || pos>destlen) pos=destlen;

	char *ndest=new char[destlen+len+1];
	*ndest='\0';
	if (pos) { strncpy(ndest,dest, pos); ndest[pos]='\0'; }

	strncat(ndest,src,len);
	strcat(ndest,dest+pos);

	delete[] dest;
	dest=ndest;
	return dest;
}

 //! Expand how much memory dest takes up, and leave its contents the same.
 /*! Reassigns dest to a new char[] that takes up strlen(dest)+n bytes.
  * \return Returns the new string.
  */
char *extendstr(char *&dest,int n)
{
	if (n<=0) return NULL;
	char *temp;
 	temp=dest;
	dest=new char[strlen(temp)+n+SAFE];
	strcpy(dest,temp);
	delete[] temp;
	return dest;
}

 //! Expand how much memory dest takes up, and leave its contents the same.
 /*! Reassigns dest to a new char[] that takes up curmax+n bytes. Adjusts curmax
  *  to reflect the new curmax.
  * \return Returns the new string.
  */
char *extendstr(char *&dest,int &curmax,int n) // assumes curmax>strlen 
{
	if (n<=0) return NULL;
	char *temp;
 	temp=dest;
	dest=new char[curmax+n+SAFE];
	strcpy(dest,temp);
	delete[] temp;
	curmax+=n;
	return dest;
}

//! Strip whitespace. where&1 means in front, where&2 means trailing.
/*! This does not create a new string. It merely moves characters in
 * the string as appropriate, and repositions the final '\0'.
 *
 * Returns dest.
 */
char *stripws(char *dest,char where)
{
	int n=0;
	while (dest[n] && isspace(dest[n])) n++;
	if (n) memmove(dest,dest+n,strlen(dest+n));
	n=strlen(dest);
	while (n && isspace(dest[n-1])) { dest[n-1]='\0'; --n; }
	return dest;
}

/*! Return whether there is whitespace at the beginning or the end.
 */
int has_outer_whitespace(const char *str)
{
	if (!str) return 0;
	if (isspace(str[0])) return 1;
	if (isspace(str[strlen(str)])) return 1;
	return 0;
}

//! Insert data into dest.
/*! dest will be reassigned to a new char[]. If atpos==0, then this
 * function is the same as prependstr(dest,data). If atpos<0 or 
 * atpos>=strlen(dest) then it is the same as appendstr(dest,data).
 * Otherwise, for instance, insertstr(dest,data,3) will insert data
 * starting at the 4th byte of dest.
 *
 * If dest==NULL and data!=NULL, then make dest be a copy of data.
 * If data is NULL, then return dest unchanged.
 */
char *insertstr(char *&dest,const char *data,int atpos)
{
	if (!data) return dest;
	if (!dest) {
		dest=newstr(data);
		return dest;
	}
	char *newdata=new char[strlen(dest)+strlen(data)+1];
	if (atpos<0 || atpos>=(int)strlen(dest)) return appendstr(dest,data);
	if (atpos==0) return prependstr(dest,data);
	strncpy(newdata,data,atpos);
	strcpy(newdata+atpos,data);
	strcat(newdata,dest+atpos);
	delete[] dest;
	dest=newdata;
	return dest;
}

//! Replace the characters from s up to and including e with data.
/*! dest is reassigned to a new char[].
 *  If newe is not NULL, then put the new e into it.
 *
 * If dest==NULL and data!=NULL, then make dest be a copy of data.
 * If data is NULL, then return dest unchanged. These happen no matter what
 * s and e are.
 *
 * If s or e are out of bounds or e<s, then NULL is returned and dest
 * is not changed.
 *
 * Returns dest.
 */
char *replace(char *&dest,const char *data,int s,int e,int *newe)
{						 // put in [s,e], newe=new e 
	if (data==NULL) return dest;
	if (dest==NULL) {
		e=strlen(data);
		dest=new char[e+1];
		strcpy(dest,data);
		if (newe) *newe=e-1;
		return dest;
	}
	int l=strlen(data),l2=strlen(dest);
	if (s<0 || e>l2-1 || e<s) return NULL;

	char *newexp=new char[s + l + l2-e + 1];

	 //copy over initial segment from dest, if any
	if (s>0) strncpy(newexp,dest,s);

	 //copy over data
	strncpy(newexp+s,data,l);

	 //copy over final segment from dest, if any
	if (e<l2-1) strcpy(newexp+s+l,dest+e+1);

	newexp[s +l+ l2-e-1]='\0';
	e=s+l-1;
	delete[] dest;
	if (newe) *newe=e;
	return dest=newexp;    
}

//! Replace all occurences of old in dest with newn. Does not modify dest. Returns new'd result.
/*! If s and e are specified then replace only within the inclusive range [s,e].
 *
 * This function is not efficient for large arrays (or small for that matter) It reallocs on finding each occurence.
 * *** should probably rewrite to reallocate once.. means counting occurences of old, then substituting.
 *
 * \todo **** this needs work and testing
 *
 * *** maybe have option to replace dest rather than return new?
 */
char *replaceall(const char *dest,const char *old,const char *newn,int s,int e)//s=0,e=-1
{
	if (!dest) return NULL;
	if (s<0) s=0;
	if (e<s || e>=(int)strlen(dest)) e=strlen(dest)-1;
	
	char *ndest=NULL;
	const char *p=dest+s,*f=p;
	if (s) ndest=newnstr(dest,s);
	while (p-dest<e+1) {
		f=strstr(p,old);
		if (!f || f-dest+(int)strlen(old)>e+1) { break; }
		if (f-p) appendnstr(ndest,p,f-p);
		appendstr(ndest,newn);
		p=f+strlen(old);
	}
	appendstr(ndest,p);
	return ndest;
}

 //! Replace all name occurences in dest of old with newn.
 /*! Replace all of old names with newn in dest, without deleting dest. A name
  *  in this case means a continuous string of '_' or alphanumeric characters.
  *  Really it just searches for old in dest, then checks to make sure that
  *  the characters immediately before and after what it found are not '_' or
  *  a letter or a number. Then it puts newn where that old was. Like
  *  the vi command s/\\&lt;old\\&gt;/s//newn/g.
  *  \return Returns the new char string.
  */
char *replaceallname(const char *dest,const char *old,const char *newn)
{
	if (!dest) return NULL;
	int c=0, c2=0;
	char *temp,*newdest;
	newdest=new char[strlen(dest)+1];
	strcpy(newdest,dest);
	while (c<(int)strlen(newdest)) {
		temp=strstr(newdest+c,old);
		if (!temp) return newdest;
		c2=(temp-newdest);
		if (c>0 && c2==c) 
		   if (isalnum(newdest[c2-1]) || newdest[c2-1]=='_')
			{ c=c2+strlen(old); continue; }
		if (isalnum(newdest[c2+strlen(old)]) ||
					 newdest[c2+strlen(old)]=='_')
			{ c=c2+strlen(old); continue; }
		c=c2+strlen(old)-1;
		replace(newdest,newn,c2,c,&c);
		c++;
	}
	return newdest;
}

 //! Get a new char array of any '_' or alphanumeric characters. Assumes no whitespace.
 /*! \return Returns the new'd array, which the user must delete.
  */
char *getnamestring(const char *buf)
{
	if (!isalpha(*buf) && *buf!='_') return NULL;

	char *tname;
	int c=0;     
	while (isalnum(buf[c]) || buf[c]=='_') c++;
	tname=new char[c+1];
	strncpy(tname,buf,c);
	tname[c]='\0';
	return tname;
}

//! For char ** arrays, delete each element, then strs itself.
/*! If n==0, then delete entries until the first NULL entry.
 * Otherwise, delete any non-null entry from 0 to n-1.
 * strs is set to NULL.
 */
void deletestrs(char **&strs,int n)
{
	if (n<0) return;
	for (int c=0; (n>0 && c<n) || (n==0 && strs[c]!=NULL); c++) delete[] strs[c];
	delete[] strs;
	strs=NULL;
}

 //! Split str using delim as delimiter. 
 /*! The delimiter is removed. The number of fields is put into n_ret,
  * including the final NULL.
  *
  *  Returns a NULL terminated char** holding the fields which are 
  *  new'd character arrays that are copies from the original str.
  *  The user must delete these itself,
  * or call deltestrs(). Empty fields are created as string "".
  *
  * \todo investigate strtok strsep, make a split version that puts '\\0' on
  * delimiters, does not allocate new strings, returns new'd array of char* that
  * point to start of each field..
  */
char **split(const char *str,char delim,int *n_ret)
{
	if (!str) {
		if (n_ret) *n_ret=0;
		return NULL;
	}
	const char *t=NULL;
	int c,c2=0,l=strlen(str);
	int n=1; // n is the number of fields
	for (c=0; c<l; c++) if (str[c]==delim) n++;
	char **r=new char*[n+1];
	r[n]=NULL;
	if (n==1) { 
		r[0]=NULL; 
		makestr(r[0],str); 
		if (n_ret) *n_ret=n;
		return r; 
	}
	for (c=0,c2=0; c2<n; c2++) {
		t=strchr(str+c,delim);
		if (!t) {
			t=str+strlen(str);
//			if (t==str+c) { // is blank final entry before eol
//				r[c2]=new char[1];
//				r[c2][0]='\0';
//			}
		}
		r[c2]=new char[t-str-c+1];
		strncpy(r[c2],str+c,t-str-c);
		r[c2][t-str-c]='\0';
		c=t-str+1;
	}
	if (n_ret) *n_ret=n;
	return r;
}

//! Split str using delim as delimiter, modifying the original str. 
/*! The delimiter is replaced by '\\0' in the original str.
 * The number of fields is put into n_ret.
 *
 *  Returns a NULL terminated char** holding the fields which
 *  point to the start of each field within str.
 *  The user need only delete[] the returned array, not the individual elements,
 *  as the elements of the returned array will be destroyed when str itself is destroyed.
 */
char **spliton(char *str,char delim,int *n_ret)
{
	int c,l=strlen(str);
	int n=1,nn=0; // n is the number of fields
	for (c=0; c<l; c++) {
		if (str[c]==delim) n++;
	}
	char **r=new char*[n+1];
	r[nn++]=str;
	for (c=0; c<l; c++) {
		if (str[c]==delim) {
			str[c]='\0';
			r[nn++]=str+c+1;
		}
	}
	r[n]=NULL;
	if (n_ret) *n_ret=n;
	return r;
}

//! Split stro into a NULL terminated char** of subfields, where whitespace is the delimiter.
/*! This does not modify stro. Creates new char[] to hold the fields.
 * Returns the number of fields created, not including the final NULL.
 * These strs can be easily deleted by calling deletestrs().
 * 
 * See splitonspace() for a splitter that modifies the string.
 * 
 * Any length of whitespace is the delimiter.
 * Ignores initial and final whitespace
 */
char **splitspace(const char *stro,int *n_ret)
{
	char str[strlen(stro)+1];
	strcpy(str,stro);
//cout <<stro<<endl<<str<<endl;
	int l=strlen(str);
	int c,c2,c3;
	
	 // scrunch down so that any length of whitespace is converted into a single space.
	for (c=0; c<(int)strlen(str); c++) {
		while (str[c]!='\0' && !isspace(str[c])) c++;
		if (isspace(str[c])) {
			c2=c;
			while (str[c2]!='\0' && isspace(str[c2])) c2++;
			str[c++]=' ';
			c3=c2-c;
			while (str[c2]!='\0') {
				str[c2-c3]=str[c2];
				c2++;
			}
			str[c2-c3]='\0';
		}
	}
	 // remove initial whitespace
	for (c=0; str[c]!='\0' && isspace(str[c]); ) c++;
	
	 // remove trailing whitespace
	l=strlen(str)-1;
	while (isspace(str[l])) l--;
	str[l+1]='\0';
	
	
	char **r=split(str+c,' ',n_ret);
//cout <<*n_ret<<endl;
	return r;
}

//! Split str into a NULL terminated char** of subfields on whitespace, modifying str.
/*! Returns the number of fields created, not including the final NULL.
 * 
 * If str is only whitespace, then NULL is returned.
 * 
 * Any length of whitespace is the delimiter.
 * Ignores initial and final whitespace.
 *
 * Remember that you should not delete a sub-part of the returned array. That is,
 * say you do 'char **t=splitonspace(str,&n)', then you can do 'delete[] t', but
 * do NOT do 'delete[] t[2]', because t[2] points to the inside of the original str.
 */
char **splitonspace(char *str,int *n_ret)
{
	int l=strlen(str);
	int c, n=0,nn=0;
	
	for (c=0; c<l; ) {
		while (isspace(str[c])) c++;
		if (str[c]) n++;
		while (str[c] && !isspace(str[c])) c++;
	}
	if (!n) {
		if (n_ret) *n_ret=0;
		return NULL; 
	}
	char **r=new char*[n+1];
	for (c=0; c<l; c++) {
		while (isspace(str[c])) c++;
		if (str[c]) r[nn++]=str+c;
		while (str[c] && !isspace(str[c])) c++;
		if (str[c]) { str[c]='\0'; }
	}
	r[n]=NULL;
	if (n_ret) *n_ret=n;
	return r;
}

/*! Return index in null terminated list if str is in there.
 * else return -1.
 */
int findInList(const char *name, const char **names)
{
	for (int c=0; names[c]; c++) {
		if (!strcmp(name, names[c])) return c;
	}

	return -1;
}

/*! Return index in list if str is in there.
 * else return -1.
 */
int findInList(const char *name, const char **names, int numnames)
{
	for (int c=0; c<numnames; c++) {
		if (!strcmp(name, names[c])) return c;
	}

	return -1;
}

//! Return 1 if file starts with '/' or '~/'
int is_absolute_path(const char *file)
{
	if (*file!='/') return 1;
	if (*file=='~' && (file[1]=='/' || file[1]=='\0')) return 1;
	return 0;
}

/*! \todo maybe should move lax_basename(), lax_dirname(), and increment_file() to fileutils? they
 *    only in strmanip because they don't really need the filesystem to operate.
 */

//! Return a pointer to the part of path that starts the file name, or NULL.
/*! "1/2/" returns NULL. "" returns NULL. "1/2" returns "2".
 * "1" returns "1".
 *
 * Thus if n is the returned pointer, the dirname is just the first n-path 
 * characters of path.
 *
 * Note that this differs from similar basename functions in that "1/" usually
 * returns pointer to the '\\0' after the "/", but lax_basename returns NULL.
 */
const char *lax_basename(const char *path)
{
	if (!path || strlen(path)==0) return NULL;
	const char *n=strrchr(path,'/');
	if (!n) return path;
	n++;
	if (*n=='\0') return NULL;
	return n;
}

/*! Like lax_basename, but return a pointer to the extension, if any.
 * If no extension, return NULL.
 */
const char *lax_extension(const char *path)
{
	const char *period = strrchr(path, '.');
	const char *slash  = strrchr(path, '/');
	if (slash && period && slash>period) return NULL;
	if (!period) return NULL;
	if (period[1]=='\0') return NULL;
	return period+1;
}

//! Returns a new char[] with the dir part of the path, or NULL.
/*! The calling code must delete[] what is returned.
 *
 * So something like "blah.h" will return NULL.
 * "yack/hack" will return "yack" or "yack/" if (appendslash).
 *
 * "/" will return "" (not NULL), or "/" if appendslash.
 */
char *lax_dirname(const char *path,char appendslash)
{
	if (!path) return NULL;
	const char *base=strrchr(path,'/');
	if (!base) return NULL;
	char *dir=new char[base-path+2];
	strncpy(dir,path,base-path+(appendslash?1:0));
	dir[base-path+(appendslash?1:0)]='\0';
	return dir;
}

/*! If there is a final ".ext" then replace the '.' with a '\\0'.
 *
 * Changes file, returns file.
 */
char *chop_extension(char *file)
{
	char *period = strrchr(file, '.');
	char *slash  = strrchr(file, '/');
	if (slash && period && slash>period) return file;
	if (!period) return file;
	*period='\0';
	return file;
}

//! Return a new name based on the old file plus one, so "file.jpg" will return "file2.jpg"
/*! "file3.jpg" -> "file4.jpg",
 *  "blah" -> "blah2" -> "blah3",
 *  "blah001.jpg" -> "blah002.jpg",
 *  "too.many.extensions" -> "too.many2.extensions"
 *
 *  Note that currently, only the final extension is saved, meaning "blah.tar.gz" -> "blah.tar2.gz".
 *
 *  \todo perhaps optionally allow "blah.tar.gz" -> "blah2.tar.gz", like increment_file(file, numextensions=1)
 */
char *increment_file(const char *file)
{
	if (!file) return NULL;
	
	 //find extension, if any
	const char *extension=strrchr(file,'.');
	const char *slash    =strrchr(file,'/');
	if (slash && extension<slash) extension=NULL;
	if (!extension) extension=file+strlen(file);
	
	 //find old number
	unsigned int num=2;
	const char *cnum;
	cnum=extension;

	if (cnum>file && isdigit(*(cnum-1))) {
		int base=1;
		num=0;
		cnum--;
		do {
			num  += base*(cnum[0]-'0');
			base *= 10;
			cnum--;
		} while (cnum>file && isdigit(*cnum));
		cnum++;
		num++;
	}

	 //build new name
	char *newfile=new char[cnum-file + strlen(file)-(extension-file) + 12]; //12 is safe for integer
	if (cnum-file) strncpy(newfile,file,cnum-file);
	char format[15];
	sprintf(format, "%%0%dd%%s", (int)(extension-cnum));
	sprintf(newfile+(cnum-file),format,num,extension);
	return newfile;
}

/*! Reverse of htmlchars_decode().
 *
 * Encode single quote (to &apos;), double quote (&quot;),
 * ampersand (&amp;), less than (&lt;) and greater than (&gt;).
 *
 * If buffer==NULL, then return a new char[] with the string.
 * If buffer!=NULL, then attempt to put in buffer, which has room for len chars, and return buffer.
 * If buffer does not have enough room, then put how many chars are needed in len_ret, and return NULL.
 */
char *htmlchars_encode(const char *str, char *buffer, int len, int *len_ret)
{
	const char *s = str;

	int n=0;
	while (s) {
		s = strpbrk(s, "\"'&<>");
		if (s) {
			if (*s == '&') n += 5;
			else if (*s == '\'') n += 6;
			else if (*s == '"' ) n += 6;
			else if (*s == '<' ) n += 4;
			else if (*s == '>' ) n += 4;
			s++;
		}
	}

	int slen = strlen(str);
	if (buffer && (len < slen+n+1)) {
		if (len_ret) *len_ret = slen+n+1;
		return NULL;
	}

	char *ret = buffer;
	if (!ret) {
		len = n + slen;
		ret = new char[len + 1];
	}

	*ret = '\0';

	const char *s1;
	s = s1 = str;
	while (s && *s) {
		s = strpbrk(s1, "\"'&<>");

		if (s) {
			if (s!=s1) strncat(ret, s1, s-s1); //not translated bit

			if (*s == '&') strcat(ret, "&amp;");
			else if (*s == '\'') strcat(ret, "&apos;");
			else if (*s == '"' ) strcat(ret, "&quot;");
			else if (*s == '<' ) strcat(ret, "&lt;");
			else if (*s == '>' ) strcat(ret, "&gt;");

			s1 = s+1;

		} else {
			strcat(ret, s1);
		}
	}

	if (len_ret) *len_ret = slen+n+1;
	return ret;
}

/* Reverse of htmlchars_encode(). Convert a few &amp; things to: '"&<>
 *
 * If buffer==NULL, then return a new char[].
 * If buffer!=NULL, then it should be at least as long as strlen(str)+1, since
 * the returned string can only be the same length or shorter.
 */
char *htmlchars_decode(const char *str, char *buffer)
{
	if (!buffer) buffer = new char[strlen(str)+1];

	const char *s = str;
	const char *f;
	*buffer = '\0';

	while (s && *s) {
		f = strchr(s,'&');

		if (!f) {
			strcat(buffer, s);
			s = NULL;

		} else {
			if (f>s) strncat(buffer, s, f-s);
			s=f;

			if (strcasestr(s,"&amp;") == s) { strcat(buffer, "&"); s += 5; }
			else if (strcasestr(s,"&apos;") == s) { strcat(buffer, "'");  s += 6; }
			else if (strcasestr(s,"&quot;") == s) { strcat(buffer, "\""); s += 6; }
			else if (strcasestr(s,"&gt;")   == s) { strcat(buffer, ">");  s += 4; }
			else if (strcasestr(s,"&lt;")   == s) { strcat(buffer, "<");  s += 4; }
			else { //uncaught ampersand!
				strcat(buffer,"&");
				s++;
			}
		}
	}

	return buffer;
}


/*! @} */



