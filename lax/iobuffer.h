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
	int what; // see WHAT_*
	char filemode; //'r' or 'w'

	char *astr;
	const char *cstr;
	long slen;   //length in astr or cstr
	long max; //maximum space allocated

	char *filename;
	FILE *f;
	int fd;

	const char *errormessage;
	int errorstate;
	long curpos;

	enum What {
		WHAT_None    =0,
		WHAT_File    ,
		WHAT_String  ,
		WHAT_CString
	};

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
	virtual int OpenFile(const char *file_name, const char *mode);
	virtual int OpenFileLocked(const char *file_name, const char *mode, bool shield_locale);
	virtual int Close();
	virtual int CloseAndUnlock();
	virtual int SaveStrToFile(const char *file_name);
	virtual int GetStrFromFile(const char *file_name, int maxchars=-1);
	virtual int UseThis(FILE *ff);
	virtual int IsOpen();
	//int OpenTempFile();


	 //----string specific
	virtual int OpenCString(const char *str); //reads only, does not allocate a new string
	virtual int OpenString(const char *str); //copies to a new string, can read, write, and grow the string
	virtual const char *GetStringBuffer();
	virtual long GetStringBufferLength();
	//int OpenInString(char *str, long nn, long nmax); //can read and write within the string, does not allocate new. cannot shrink or grow string past allocation
};

} //namespace

#endif

