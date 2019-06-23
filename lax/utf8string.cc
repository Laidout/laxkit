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
//    Copyright (C) 2018 by Tom Lechner
//


#include <cstring>
#include <cstdio>
#include <cstdarg>

#include <iostream>

#include <lax/utf8string.h>
#include <lax/utf8utils.h>
#include <lax/strmanip.h>

#define DBG


using namespace std;

namespace Laxkit {


/*! \class Utf8String
 *
 * A string class built for utf8 use.
 * This class exists mainly due to lack of printf style formatting in std::string,
 * as well as poor utf8 support there also.
 *
 * \todo look into variadic templates to allow easier expansion of printf style formatting to non-primitive types.
 */



/*! Reallocate in blocks of this number of bytes.
 */
unsigned int Utf8String::CHARBLOCKSIZE = 20;


DBG static int num = 0;

Utf8String::Utf8String()
 : s(nullptr), bytes_allocated(0), num_chars(0), num_bytes(0)
{
	debug_which = num;
	num++;
	cerr <<"Utf8String constructor default "<<debug_which<<endl;

}

/*! n is bytes. If n<0, then use strlen(str).
 */
Utf8String::Utf8String(const char *str, int n)
  : Utf8String()
{
	if (n<0) n = (str ? strlen(str) : 0);
	bytes_allocated = (n/CHARBLOCKSIZE + 1)*CHARBLOCKSIZE + 1;
	s = new char[bytes_allocated];
	strncpy(s,str,n);
	s[n] = '\0';
	num_bytes = n;
	updateNumChars();

	cerr <<"Utf8String constructor string,len"<<endl;
}

/*! n is bytes. If n<0, then use strlen(str). If insert, then take
 * possession of str. Else str is copied.
 */
Utf8String::Utf8String(char *str, int n, bool insert)
  : Utf8String()
{
	if (n<0) n = (str ? strlen(str) : 0);
	bytes_allocated = (n/CHARBLOCKSIZE + 1)*CHARBLOCKSIZE + 1;
	if (insert) s = str;
	else {
		s = new char[bytes_allocated];
		strncpy(s,str,n);
		s[n] = '\0';
	}
	num_bytes = n;
	updateNumChars();

	cerr <<"Utf8String constructor string,len"<<endl;
}

Utf8String::Utf8String(const Utf8String &str)
  : Utf8String(str.c_str(), -1)
{
	cerr <<"Utf8String constructor &utf8string"<<endl;
}

Utf8String::Utf8String(const Utf8String *str)
  : Utf8String(str ? str->c_str() : nullptr)
{
}

Utf8String::Utf8String(int i)
  : Utf8String()
{
	s = new char[50];
	bytes_allocated = 50;
	sprintf(s, "%d", i);
	num_bytes = strlen(s);
	updateNumChars();

	cerr <<"Utf8String constructor int"<<endl;
}

Utf8String::Utf8String(long i)
  : Utf8String()
{
	s = new char[50];
	bytes_allocated = 50;
	sprintf(s, "%ld", i);
	num_bytes = strlen(s);
	updateNumChars();

	cerr <<"Utf8String constructor int"<<endl;
}

Utf8String::Utf8String(double d)
  : Utf8String()
{
	s = new char[50];
	bytes_allocated = 50;
	sprintf(s, "%f", d);
	num_bytes = strlen(s);
	updateNumChars();

	cerr <<"Utf8String constructor double"<<endl;
}

/*! Note that arguments need to be old school primitive types.
 * Variadic templates to expand this are not used at this time.
 */
Utf8String::Utf8String(const char *fmt, ...)
  : Utf8String()
{
    va_list arg;
    va_start(arg, fmt);
    int c = vsnprintf(nullptr, 0, fmt, arg);
    va_end(arg);

	bytes_allocated = c+1+10;
    s = new char[bytes_allocated];
    va_start(arg, fmt);
    vsnprintf(s, c+1, fmt, arg);
    va_end(arg);
	num_bytes = strlen(s);
	updateNumChars();

	cerr <<"Utf8String constructor fmt"<<endl;
}


Utf8String::~Utf8String()
{
	cerr <<"Utf8String destructor, which: "<<debug_which<<"  str: "<<(s ? s : "null")<<endl;
	delete[] s;
	s = nullptr;
}

Utf8String &Utf8String::operator=(const Utf8String &str)
{
	if (s == str.c_str()) return *this;
	Clear();
	Append(str);
	return *this;
}

Utf8String &Utf8String::operator=(const char *str)
{
	Clear();
	Append(str);
	return *this;
}



/*! Return the actual char[] that stores the string, and zero out ourselves.
 * This is basically the reverse of InsertBytes().
 */
char *Utf8String::ExtractBytes(int *chars, int *bytes, int *allocated)
{
	if (chars) *chars = num_chars;
	if (bytes) *bytes = num_bytes;
	if (allocated) *allocated = bytes_allocated;

	num_chars = num_bytes = bytes_allocated;
	char *s_ret = s;
	s = nullptr;
	return s_ret;
}

/*! Use the actual newstr to replace anything we have currently.
 * You must ensure that pointer transfer of newstr is valid.
 * This is basically the reverse of ExtractBytes().
 * If len<0, use strlen(newstr) as len.
 * We assume bytes_allocated == len+1.
 */
void Utf8String::InsertBytes(char *newstr, int len)
{
	if (s) delete[] s;
	if (len<0) len = strlen(newstr);
	num_bytes = len;
	bytes_allocated = len+1;
	s = newstr;
	updateNumChars();
}

void Utf8String::Clear()
{
	if (bytes_allocated>CHARBLOCKSIZE) {
		delete[] s;
		s = nullptr;
		bytes_allocated = 0;
	} else {
	}

	if (s) s[0]='\0';

	num_chars=0;
	num_bytes=0;
}

/*! Make num_chars be the number of unicode characters in the string
 * (not graphemes, but merely unicode chars).
 */
void Utf8String::updateNumChars()
{
	num_chars=0;
	if (!s) return;

	const char *ss;

	for (int c=0; c<num_bytes && s[c]!=0; ) {
		num_chars++;
		c++;
		ss = utf8fwd(s+c, s+c, s+num_bytes);
		if (ss == s+c) break;
		c = ss-s;
	}
}

///*! Return new Utf8String from substring [from..to]. 
// * from and to are char positions, not bytes.
// */
//Utf8String Utf8String::SubstrChars(long from, long to)
//{
//	from = charToByte(from);
//	if (from<0) from=0;
//	to = charToByte(to);
//	if (to<0) to = num_bytes-1;
//	return Utf8String(s+from,to-from+1);
//}

/*! Return new Utf8String from substring [from..to]. 
 * from and to are byte positions, not character positions.
 */
Utf8String Utf8String::Substr(long from_byte, long to_byte)
{
	if (from_byte < 0) from_byte = 0;
	if (to_byte < 0) to_byte = num_bytes-1;
	return Utf8String(s + from_byte, to_byte - from_byte + 1);
}

/*! Compare against another string. -1 for less than, 0 for equal, 1 for greater than.
 */
int Utf8String::Strcmp(const Utf8String &str) const
{
	if (!s) {
		if (!str.c_str()) return 0;
		return 1;
	}
	if (!str.c_str()) return -1;
	return strcmp(s, str.c_str());
}

/*! Caselessly compare against another string. -1 for less than, 0 for equal, 1 for greater than.
 */
int Utf8String::Strcasecmp(const Utf8String &str) const
{
	if (!s) {
		if (!str.c_str()) return 0;
		return 1;
	}
	if (!str.c_str()) return -1;
	return strcasecmp(s, str.c_str());
}

/*! Return the unicode value for the given character pos.
 */
unsigned int Utf8String::ch(int char_pos)
{
	int bpos = 0;
	for (int i=0; i<char_pos && bpos < num_bytes; i++) {
		bpos++;
		bpos = utf8fwd_index(s, bpos, num_bytes);
	}
	
	int len;
	return utf8decode(s+bpos, s+num_bytes, &len);
}

/*! Change the character at char_pos, reallocating if necessary.
 */
unsigned int Utf8String::ch(int char_pos, unsigned int newch)
{ // ***
	cerr << "AAA!! IMPLEMENT Utf8String::ch(int char_pos, unsigned int newch)!!!"<<endl;
	return 0;
}

/*! Return the unicode value for the character that byte_pos is part of.
 */
unsigned int Utf8String::ch_b(int byte_pos)
{
	byte_pos = utf8back_index(s, byte_pos, num_bytes);
	int len;
	return utf8decode(s+byte_pos, s+num_bytes, &len);
}

/*! Change the character that byte_pos is a part of, reallocating if necessary.
 */
unsigned int Utf8String::ch_b(int byte_pos, unsigned int newch)
{ //***
	cerr << "AAA!! IMPLEMENT Utf8String::ch_b(int byte_pos, unsigned int newch)!!!"<<endl;
	return 0;
}

/*! Set a byte. Does NOT do allocation bounds checking and does NOT do
 * utf8 character validation. It assumes you know what you are doing.
 */
unsigned int Utf8String::byte(int byte_index, unsigned int newbyte)
{
	s[byte_index] = newbyte;
	return newbyte;
}

/*! Return the byte position of the next utf8 character after curpos.
 */
unsigned long Utf8String::next(int byte_index)
{
	byte_index++;
	if (byte_index == num_bytes) return byte_index;
	return utf8fwd_index(s, byte_index, num_bytes);
}

unsigned long Utf8String::prev(int byte_index)
{
	if (byte_index == 0) return 0;
	byte_index--;
	return utf8back_index(s, byte_index, num_bytes);
}


void Utf8String::Sprintf(const char *fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    int c = vsnprintf(nullptr, 0, fmt, arg);
    va_end(arg);

	if (s && c+1 > bytes_allocated) {
		delete[] s;
		bytes_allocated = 0;
		num_bytes = num_chars = 0;
		s = nullptr;
	}

	if (s == nullptr) {
		bytes_allocated = c+1+10;
		s = new char[bytes_allocated];
	}

    va_start(arg, fmt);
    vsnprintf(s, c+1, fmt, arg);
    va_end(arg);
	num_bytes = strlen(s);
	updateNumChars();
}

void Utf8String::Append(const Utf8String &str)
{
	Append(str.c_str());
}

void Utf8String::Append(const char *str)
{
	if (!str) return;
	if (!s) {
		s = new char[strlen(str) + 1 + CHARBLOCKSIZE];
		strcpy(s,str);
	} else {
		char *newdest;
		newdest = new char[strlen(s) + strlen(str) + 1 + CHARBLOCKSIZE];
		strcpy(newdest, s);
		strcat(newdest, str);
		delete[] s;
		s = newdest;
	}
	num_bytes = strlen(s);
	updateNumChars();
}

void Utf8String::Prepend(const Utf8String &str)
{
	Prepend(str.c_str());
}

void Utf8String::Prepend(const char *str)
{
	if (!str) return;
	if (!s) {
		s = new char[strlen(str) + 1 + CHARBLOCKSIZE];
		strcpy(s,str);
	} else {
		char *newdest;
		newdest = new char[strlen(s) + strlen(str) + 1 + CHARBLOCKSIZE];
		strcpy(newdest, str);
		strcat(newdest, s);
		delete[] s;
		s = newdest;
	}
}

/*! Find first occurence of str from byte position startat.
 * Return -1 for not found, or >=0 for byte index of found.
 */
long Utf8String::Find(const char *str, int startat, int ignorecase)
{
	if (!s || !str) return -1;
	char *i = (ignorecase ? strcasestr(s + startat, str) : strstr(s + startat, str));
	if (!i) return -1;
	return i-s;
}

long Utf8String::Find(const Utf8String &str, int startat, int ignorecase)
{
	return Find(str.c_str(), startat, ignorecase);
}

/*! Find the first unicode character ch.
 */
long Utf8String::Find(unsigned int ch, int startat_byte_pos)
{
	char chstr[8];
	utf8encode(ch, chstr);
	return Find(chstr, startat_byte_pos, false);
}

/*! Find last occurence of str from byte position startat looking backwards.
 * Return -1 for not found, or >=0 for byte index of found.
 */
long Utf8String::Findr(const char *str, int startat, int ignorecase)
{
	if (!str) return -1;
	if (startat<0) startat = num_bytes;
	int len = strlen(str);
	while (startat-len > 0) {
		if ((ignorecase ? strncasecmp(s + startat - len, str, len) : strncmp(s + startat - len, str, len)) == 0) return startat - len;
		startat--;
	}
	return -1;
}

/*! Find the byte position of the last occurence of unicode ch, starting backward from startat.
 * If start<0, then start from the end.
 * Return -1 if not found.
 */
long Utf8String::Findr(unsigned int ch, int startat)
{
	if (!s) return -1;
	char chstr[8];
	utf8encode(ch, chstr);
	return Findr(chstr, startat, false);
}

/*! Replace occurences of oldstr with newstr.
 */
int Utf8String::Replace(const char *new_str, const char *oldstr, bool all)
{
	if (!s || isblank(oldstr)) return 0;

	char *news;
	if (all) news = replaceall(s, oldstr, new_str);
	else news = replacefirst(s, oldstr, new_str);
	delete[] s;
	s = news;
	num_bytes = strlen(s);
	updateNumChars();

	return 0;
}

/*! Remove the bytes from to to inclusive. Return number of bytes removed.
 */
int Utf8String::Erase(long from, long to)
{
	if (!s || from>to || from == num_bytes) return 0;
	if (to<0) to = num_bytes;
	if (from<0 || from > num_bytes || to < 0 || to > num_bytes) return 0;
	memmove(s+from, s+to, to-from+1);
	return to-from+1;
}

/*! Remove initial and final whitespace.
 * This is a convenience function that calls LTrim() and RTrim().
 */
void Utf8String::Trim()
{
	LTrim();
	RTrim();
}

/*! Trim initial whitespace.
 */
void Utf8String::LTrim()
{
	if (s) return;
	int p = 0;
	while (isspace(s[p])) p++;
	if (p>0) Erase(0,p-1);		
}

/*! Trim final whitespace.
 */
void Utf8String::RTrim()
{
	if (!s || num_bytes == 0) return;
	while (num_bytes > 0 && isspace(s[num_bytes-1])) num_bytes--;
}

/*! Return null terminated list of splits. Returns nullptr if none.
 * Optionally return the number of strings in num_ret, not including the terminating null.
 */
Utf8String *Utf8String::Split(const char *on_this, int *num_ret)
{ //***
	cerr << "AAAA!! IMPLEMENT Utf8String *Utf8String::Split(const char *on_this, int *num_ret)!!!"<<endl;
	return nullptr;
}

bool Utf8String::EndsWith(const char *str)
{
	if (!str) return false;
	int l = strlen(str);
	if (l > num_bytes) return false;
	return ::strcmp(s+num_bytes-l, str) == 0;
}

bool Utf8String::EndsWith(const Utf8String &str)
{
	return EndsWith(str.c_str());
}

bool Utf8String::StartsWith(const char *str)
{
	if (!str) return false;
	return strstr(s, str) == s;
}

bool Utf8String::StartsWith(const Utf8String &str)
{
	return StartsWith(str.c_str());
}

/*! Return if string has no characters.
 */
bool Utf8String::IsEmpty()
{
	return s == nullptr || *s == '\0';
}


Utf8String operator+(const Utf8String &a, const char *b)
{
	Utf8String str(a);
	str.Append(b);
	return str;
}

Utf8String operator+(const char *a, const Utf8String &b)
{
	Utf8String str(a);
	str.Append(b);
	return str;
}

Utf8String operator+(const Utf8String &a, const Utf8String &b)
{
	Utf8String str(a);
	str.Append(b);
	return str;
}

Utf8String operator+=(Utf8String &s, const char *str)
{
	s.Append(str);
	return s;
}

Utf8String operator+=(Utf8String &s, const Utf8String &str)
{
	s.Append(str);
	return s;
}



} // namespace Laxkit

