/*
 *
 * This file is based in part on utf.c/utf.h from 
 * the FLTK project: http://www.fltk.org, with the following copyright.
 * Further modifications to this file are subject to the same copyright.
 * 
 * Copyright 1998-2006 by Bill Spitzak and others.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/*! \file
 * Functions to manipulate UTF-8 strings and convert from/to legacy encodings.
 */

#ifndef _LAX_UTF8UTILS_H
#define _LAX_UTF8UTILS_H

#include <stdlib.h>

namespace Laxkit {
	
int	utf8bytes(unsigned ucs);

unsigned utf8decode(const char*, const char* end, int* len);
int	utf8encode(unsigned, char*);
const char* utf8fwd(const char*, const char* start, const char* end);
const char* utf8back(const char*, const char* start, const char* end);
long utf8back_index(const char* p, long pos, long end);

unsigned utf8towc(const char*, unsigned, wchar_t*, unsigned);
unsigned utf8tomb(const char*, unsigned, char*, unsigned);
unsigned utf8toa (const char*, unsigned, char*, unsigned);
unsigned utf8fromwc(char*, unsigned, const wchar_t*, unsigned);
unsigned utf8frommb(char*, unsigned, const char*, unsigned);
unsigned utf8froma (char*, unsigned, const char*, unsigned);
int utf8locale();
int utf8test(const char*, unsigned);

} //namespace Laxkit


#endif


