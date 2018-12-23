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
#ifndef _LAX_UTF8STRING_H
#define _LAX_UTF8STRING_H


#include <ostream>


namespace Laxkit {


class Utf8String
{
  protected:
	char *s;
	long bytes_allocated;
	long num_chars;
	long num_bytes;

	void updateNumChars();

  public:
	static unsigned int CHARBLOCKSIZE;

	Utf8String();
	Utf8String(const char *str, int n);
	Utf8String(const Utf8String &str);
	Utf8String(const Utf8String *str);
	Utf8String(int i);
	Utf8String(double d);
	Utf8String(const char *fmt, ...);
	virtual ~Utf8String();

	long Length() { return num_chars; }
	long Bytes() { return num_bytes; }
	long Capacity() { return bytes_allocated; }
	void Clear();

	//operator=(const char *str);
	//operator=(const Utf8String &str);

	int Strcmp(const Utf8String &str) const;
	int Strcasecmp(const Utf8String &str) const;

	char *ExtractBytes(int *chars, int *bytes, int *allocated);
	void InsertBytes(char *newstr, int len);

	const char *c_str() const { return s; }
	unsigned int ch(int pos);
	unsigned int ch(int pos, unsigned int newch);
	unsigned int ch_b(int pos);
	unsigned int ch_b(int pos, unsigned int newch);
	unsigned int byte(int pos) { if (pos>=0 && pos<num_bytes) return s[pos]; return 0; }
	unsigned int byte(int pos, unsigned int newbyte);

	unsigned long next(int byte_index);
	unsigned long prev(int byte_index);
	
	void Append(const Utf8String &str);
	void Append(const char *str);
	void Prepend(const Utf8String &str);
	void Prepend(const char *str);
	int Replace(const char *newstr, const char *oldstr, bool all);
	int Erase(long from, long to);
	void Trim();
	void LTrim();
	void RTrim();

	Utf8String *Split(const char *on_this, int *num_ret=nullptr);
	Utf8String Substr(long from, long to);

	long  Find(const char *str, int startat, int ignorecase);
	long  Find(const Utf8String &str, int startat, int ignorecase);
	long  Find(unsigned int ch, int startat_byte_pos);
	long Findr(const char *str, int startat, int ignorecase);
	long Findr(const Utf8String &str, int startat, int ignorecase);
	long Findr(unsigned int ch, int startat);

	bool EndsWith(const char *str);
	bool EndsWith(const Utf8String &str);
	bool StartsWith(const char *str);
	bool StartsWith(const Utf8String &str);
};

Utf8String operator+=(Utf8String &s, const char *str);
Utf8String operator+=(Utf8String &s, const Utf8String &str);

bool operator==(const Utf8String &a, const Utf8String &b) { return a.Strcmp(b) == 0; }
bool operator!=(const Utf8String &a, const Utf8String &b) { return a.Strcmp(b) != 0; }
bool operator<=(const Utf8String &a, const Utf8String &b) { return a.Strcmp(b) <= 0; }
bool operator< (const Utf8String &a, const Utf8String &b) { return a.Strcmp(b) <  0; }
bool operator>=(const Utf8String &a, const Utf8String &b) { return a.Strcmp(b) >= 0; }
bool operator> (const Utf8String &a, const Utf8String &b) { return a.Strcmp(b) >  0; }

Utf8String operator+(const Utf8String &a, const char *b);
Utf8String operator+(const char *a, const Utf8String &b);
Utf8String operator+(const Utf8String &a, const Utf8String &b);

inline std::ostream &operator<<(std::ostream &os, Utf8String const &s)
{
	if (s.c_str()) return os << s.c_str();
	return os;
}

} //namespace Laxkit



#endif


