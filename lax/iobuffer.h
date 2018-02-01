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
//    Copyright (C) 2018 by Tom Lechner
//
#ifndef _LAX_IOBUFFER_H
#define _LAX_IOBUFFER_H


#include <cstdio>


namespace Laxkit {


class IOBuffer
{
  protected:
	int blocksize;
	int what; //0 nothing yet. 1 = FILE, 2 = astr, 3 = cstr
	char filemode; //'r' or 'w'

	char *astr;
	const char *cstr;
	long slen;   //length in str or cstr
	long max; //maximum space allocated

	char *filename;
	FILE *f;

	const char *errormessage;
	int errorstate;
	long curpos;

  public:
	IOBuffer();
	virtual ~IOBuffer();
	virtual int Blocksize(int newblocksize);
	virtual int Reallocate(long size);

	virtual int    Printf(const char *fmt,...);
	virtual size_t Write(const void *ptr, size_t size, size_t nmemb);
	virtual size_t Read(void *ptr, size_t size, size_t nmemb);
	virtual int    GetLine(char **lineptr, size_t *n);
	virtual void   FreeGetLinePtr(char *lineptr);

	virtual int  SetPos(long offset); //fseek-whence: SEEK_SET, SEEK_CUR, or SEEK_END
	virtual long Curpos();
	virtual void Rewind();

	virtual const char *GetError();
	virtual void Clearerr();
	virtual int  IsEOF();


	 //----file specific
	virtual const char *Filename();
	virtual int OpenFile(const char *filename, const char *mode);
	virtual int Close();
	virtual int SaveStrToFile(const char *filename);
	virtual int GetStrFromFile(const char *filename, int maxchars=-1);
	virtual int UseThis(FILE *ff);
	//int OpenTempFile();


	 //----string specific
	virtual int OpenCString(const char *str); //reads only, does not allocate a new string
	virtual int OpenString(const char *str); //copies to a new string, can read, write, and grow the string
	//int OpenInString(char *str, long nn, long nmax); //can read and write within the string, does not allocate new. cannot shrink or grow string past allocation
};

} //namespace Laxkit;

#endif

