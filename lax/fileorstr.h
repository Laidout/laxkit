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
//    Copyright (C) 2013 by Tom Lechner
//
#ifndef _LAX_FILEORSTR_H
#define _LAX_FILEORSTR_H



***** /* is this even useful? */ ******

// Idea is to have a single interface for various streams.
// This might simplify various dump_out and att/css/json/xml conversion systems
// These could be:
//   char[] buffers
//   files on disk



namespace Laxkit {

class FileOrStr
{
  protected:
	char *str;
	const char *cstr;
	long n;   //num of defined bytes in str or cstr
	long max; //maximum space allocated

	char *filename;
	FILE *f;

	int errorstate;
	long curpos;

  public:
	FileOrStr();
	virtual ~FileOrStr();

	int printf(const char *fmt,...);
	size_t write(const void *ptr, size_t size, size_t nmemb);
	size_t fread(void *ptr, size_t size, size_t nmemb);
	int getline(char **lineptr, size_t *n);

	int setpos(long offset, int whence=0); //fseek-whence: SEEK_SET, SEEK_CUR, or SEEK_END
	long curpos();
	void rewind();

	void clearerr();
	int feof();

	const char *Filename();
	int Open(const char *filename, const char *mode);
	int Close();
	int OpenTempFile();

	int OpenCString(const char *str); //reads only, does not allocate a new string
	int OpenInString(char *str, long nn, long nmax); //can read and write within the string, does not allocate new. cannot shrink or grow string past allocation
	int OpenNewString(const char *str); //copies to a new string, can read, write, and grow the string

	int SaveStrToFile(const char *filename);
	int GetStrFromFile(const char *filename);
};

} //namespace Laxkit;

#endif

