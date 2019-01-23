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

#include <lax/iobuffer.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>


#define DBG
#include <iostream>
using namespace std;


namespace LaxFiles {


/*! \class IOBuffer
 * Class that lets you output or input to a disk file or a string, using
 * the same somewhat memory efficient interface.
 *
 * If only reading, you can use a char[] or const char[].
 */


#define WHAT_None    0
#define WHAT_File    1
#define WHAT_String  2
#define WHAT_CString 3


IOBuffer::IOBuffer()
{
	blocksize = 512; //chunks to allocate

	what = WHAT_None;
	filemode = '\0';

	astr = NULL;
	cstr = NULL;
	slen = 0;   //num of defined bytes in str or cstr
	max  = 0; //maximum space allocated

	fd   = 0; //file descriptor used in OpenFileLocked() and CloseAndUnlock()
	f    = NULL;
	filename = NULL;

	errormessage = NULL;
	errorstate = 0;
	curpos = 0;
}

IOBuffer::~IOBuffer()
{
	delete[] astr;
	delete[] filename;
	if (f) {
		DBG cerr <<"IOBuffer destructor closing f."<<endl;
		fclose(f);
	}
}

/*! Set the buffer blocksize used when allocating new string space.
 * Default is 512.
 */
int IOBuffer::Blocksize(int newblocksize)
{
	if (newblocksize > 0) {
		blocksize = newblocksize;
	}

	return blocksize;
}

/*! Reallocate astr to be able to contain size bytes, and copy over old data.
 */
int IOBuffer::Reallocate(long size)
{
	if (size == 0) {
		 //just truncate, without deallocating
		slen = 0;
		curpos = 0;

	} else {
		 //reallocate, and copy over
		max = size;
		char *nstr = new char[max];
		if (slen) {
			if (slen > max) slen = max-1;
			memcpy(nstr, astr, slen+1);
			nstr[slen] = '\0';
		}
		delete[] astr;
		astr = nstr;
	}

	return 0;
}

int IOBuffer::Printf(const char *fmt, ...)
{
	va_list arg;

	if (f) {
		va_start(arg, fmt);
		int c = vfprintf(f, fmt, arg);
		va_end(arg);
		return c;
	}
	
	if (what != WHAT_CString) return 0;

	 //write to string, expanding if necessary
	va_start(arg, fmt);
	int c = vsnprintf(NULL, 0, fmt, arg);
	va_end(arg);

	if (curpos + c > max) Reallocate(curpos+c + blocksize + 1);

	va_start(arg, fmt);
	vsnprintf(astr+curpos,max-curpos, fmt, arg);
	va_end(arg);

	curpos += c;
	if (curpos > slen) slen = curpos;

	return c;
}

/*! Return number of bytes written.
 */
size_t IOBuffer::Write(const void *ptr, size_t size, size_t nmemb)
{
	if (f) return fwrite(ptr,size,nmemb, f);

	if (what != WHAT_CString) return 0;

	 //else append to string
	if (curpos + (long)(nmemb * size) > max) {
		Reallocate(curpos + nmemb * size + blocksize + 1);
	}

	memcpy(astr+curpos, ptr, size*nmemb);
	curpos += size*nmemb;
	if (curpos > slen) {
		slen = curpos;
		astr[curpos] = '\0';
	}

	return nmemb*size;
}

/*! Return number of bytes read, and advance curpos.
 */
size_t IOBuffer::Read(void *ptr, size_t size, size_t nmemb)
{
	if (f) return fread(ptr,size,nmemb, f);

	 //else read from string and advance curpos
	int nbytes = size*nmemb;

	if (curpos + nbytes > slen) nbytes = slen - curpos;
	if (nbytes <= 0) {
		((char*)ptr)[0] = 0;
		return 0;
	}

	if (what == WHAT_CString) memcpy(ptr, cstr+curpos, nbytes);
	else memcpy(ptr, astr+curpos, nbytes);

	curpos += nbytes;

	return nbytes;
}

/*! Return in lineptr the characters from curpos up to and including
 * the next newline character. curpos will be updated to just after the newline.
 *
 * realloc lineptr if necessary. 
 *
 * Normal usage is to start with *lineptr being NULL, then it is reallocated
 * here as needed.
 * Currently, lineptr should be deleted with free(), not
 * delete[], to preserve compatibility with default system getline.
 * You can use FreeGetLinePtr() instead to be agnostic about it. Since getline
 * is GNU only, it may be phased out at some point, then this will switch
 * to c++ style delete[].
 *
 * Return value is 0 for no bytes read, not quite at eof, or positive number
 * for number of bytes read (including the newline). Returns negative for
 * in error state or eof, and nothing read.
 *
 * So usage will be like this:
 * <pre>
 *     IOBuffer buffer;
 *     //buffer.OpenFile("somefile.txt", "r");
 *     buffer.OpenString(testatt);
 * 
 *     char *line = NULL;
 *     size_t n = 0;
 *     int c;
 * 
 *     while (!buffer.IsEOF()) {
 *         c = buffer.GetLine(&line, &n);
 *         if (c<=0) break;
 *
 *         // ... do stuff ...
 *     }
 * 
 *     buffer.FreeGetLinePtr(line);
 *     buffer.Close();
 * </pre>
 */
int IOBuffer::GetLine(char **lineptr, size_t *n)
{
	if (f) {
//		//cache file io, then pull lines from string
//		***
//
//		return ***;

		return getline(lineptr, n, f);
	}

	const char *s = astr;
	if (what == WHAT_CString) s = cstr;

	const char *nextnl = strchr(s+curpos, '\n');
	if (!nextnl) nextnl = s+slen;
	else nextnl++;
	size_t linel = nextnl - (s+curpos);

	if (linel > *n) {
		 //reallocate line
		//delete[] *lineptr;
		//*lineptr = new char[linel+20];
		if (*lineptr) free(*lineptr);
		*lineptr = (char*)malloc(linel+20);

		*n = linel+20;
	}

	memcpy(*lineptr, s+curpos, linel);
	(*lineptr)[linel] = '\0';

	curpos += linel;
	return linel;
}

/*! kludge to that calling code can be agnostic about how GetLine() is
 * allocating new lines.
 *
 * Note that lineptr is totally independent of IOBuffer. It is not stored
 * locally anywhere in *this.
 */
void IOBuffer::FreeGetLinePtr(char *lineptr)
{
	if (lineptr) free(lineptr);
	//delete[] lineptr;
}


/*! Return 0 for success or nonzero error.
 */
int IOBuffer::SetPos(long offset) //fseek-whence: SEEK_SET, SEEK_CUR, or SEEK_END
{
	if (f) {
		if (offset < 0) return fseek(f, 0, SEEK_END);
		return fseek(f, offset, SEEK_SET);
	}

	if (offset < 0) curpos = slen;
	else curpos = offset;
	if (curpos > slen) curpos = slen;

	return 0;
}

long IOBuffer::Curpos()
{
	if (f) return ftell(f);
	return curpos;
}

void IOBuffer::Rewind()
{
	if (f) rewind(f);
	else curpos = 0;
}

const char *IOBuffer::GetError()
{
	return errormessage;
}

void IOBuffer::Clearerr()
{
	if (f) clearerr(f);
	errorstate = 0;
}

int IOBuffer::IsEOF()
{
	if (f) return feof(f);
	return curpos == slen;
}

const char *IOBuffer::Filename()
{
	return filename;
}

int IOBuffer::IsOpen()
{
	return f != NULL;
}

/*! This will set up to use ff instead of existing f.
 * Please note that the previous f WILL NOT BE CLOSED! It will just be forgotten by *this.
 * Also note that the new ff WILL BE CLOSED in destructor if you don't do UseThis(NULL) beforehand! Important!!
 */
int IOBuffer::UseThis(FILE *ff)
{
	f = ff;
	what = WHAT_File;
	return 0;
}

/*! Like OpenFile(), but also flock() the file and also setlocale to "C" before doing input.
 * If you use this, you MUST close with CloseAndUnlock().
 * Return 0 for success, or nonzero error.
 */
int IOBuffer::OpenFileLocked(const char *file_name, const char *mode, bool shield_locale)
{
	cerr << " *** need to implement OpenFileLocked()"<<endl;

	what = WHAT_File;
	if (f) fclose(f);
	makestr(filename, file_name);

	int modei = (!strcmp(mode, "r") ? O_RDONLY : (!strcmp(mode,"w") ? O_WRONLY : (!strcmp(mode,"wr") || !strcmp(mode, "rw") ? O_RDWR : 0)));
	if (modei == 0) return 1;

    fd = open(filename, modei);
	if (fd <= 0) {
		return -1;
	}
	flock(fd,LOCK_EX);
	f = fdopen(fd, mode);
	if (!f) {
		close(fd);
		return -2;
	}

	setlocale(LC_ALL,"C");
	return 0;
}

/*! Un-flocks, does setlocale(LC_ALL,""), and closes f.
 * Return 0 for success.
 */
int IOBuffer::CloseAndUnlock()
{
	if (f) {
		flock(fd,LOCK_UN);
		int status = fclose(f);// this closes fd too
		setlocale(LC_ALL,"");
		f = NULL;
		fd = 0;
		return status;
	}
	return 0;
}

/*! Open file_name as a file.
 * return 0 for success, or nonzero error.
 */
int IOBuffer::OpenFile(const char *file_name, const char *mode)
{
	what = WHAT_File;
	if (f) fclose(f);
	makestr(filename, file_name);
	f = fopen(filename, mode);

	curpos = 0;
	filemode = 0;

	if (f) {
		if (mode[0] == 'r') filemode ='r';
		else if (mode[0] == 'w') filemode ='w';
		else if (mode[0] == 'a') filemode ='w';
		if (mode[1] == '+') filemode = '%';
		return 0;
	}
	return 1;
}

int IOBuffer::Close()
{
	if (f) {
		int status = fclose(f);
		f = NULL;
		return status;
	}
	return 0;
}

/*! Save current string contents to file_name.
 * If file_name == NULL, then use current filename, if any.
 * Fails if file_name and this->filename are both NULL.
 * Return 0 for success or nonzero for error.
 */
int IOBuffer::SaveStrToFile(const char *file_name)
{
	if (!file_name) file_name = filename;
	else makestr(filename, file_name);
	if (!file_name) return 1;

	if      (what == WHAT_String)  return LaxFiles::save_string_to_file(astr, slen, file_name);
	else if (what == WHAT_CString) return LaxFiles::save_string_to_file(cstr, slen, file_name);

	return 0;
}

/*!If file_name == NULL, then use current filename, if any.
 * use read_in_whole_file to set this->str to contents of file_name.
 * Closes f if it exists.
 *
 * Read at most maxchars. If maxchars <0, then read the whole thing.
 *
 * End result is if you read in the whole file from disk to a string,
 * then called OpenString(on_that_string).
 */
int IOBuffer::GetStrFromFile(const char *file_name, int maxchars)
{
	if (f) { fclose(f); f = NULL; }

	if (!file_name) file_name = filename;
	else makestr(filename, file_name);

	what = WHAT_String;
	curpos = 0;

	delete[] astr;
	astr = LaxFiles::read_in_whole_file(file_name, NULL, maxchars);
	if (astr) {
		slen = strlen(astr);
		return 0;
	}
	return 1;
}

/*! Reads only, does not allocate a new string. Beware if your str goes out of scope before *this!
 */
int IOBuffer::OpenCString(const char *str)
{
	if (f) { fclose(f); f = NULL; }

	what = WHAT_CString;

	cstr = str;
	curpos = 0;
	slen = strlen(cstr);

	return 0;
}


//int IOBuffer::OpenInString(char *str, long nn, long nmax); //can read and write within the string, does not allocate new. cannot shrink or grow string past allocation

/*! copies to a new string, can read, write, and grow the string
 */
int IOBuffer::OpenString(const char *str)
{
	if (f) { fclose(f); f = NULL; }

	what = WHAT_String;
	curpos = 0;

	if (str) {
		int len = strlen(str);
		if (len >= max) Reallocate(len + blocksize);
		slen = len;
		memcpy(astr, str, len+1);
	} else {
		slen = 0;
	}

	return 0;
}



} //namespace LaxFiles;

