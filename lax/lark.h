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
//    Copyright (C) 2004-2006,2015 by Tom Lechner
//
#ifndef _LAX_LARK_H
#define _LAX_LARK_H


#include <lax/lists.h>


namespace Laxkit {


const char *lark_str_from_id(int id);
int lark_id_from_str(const char *str, char createifabsent=0);


//-------------------------- IdSet ----------------------------
class IdSet 
{
  protected:
	PtrStack<char> strs;
	NumStack<int> ids;
	
  public:
	IdSet();
	virtual ~IdSet();

	virtual int NumIds();
	virtual const char *StrFromId(int id);
	virtual int IdFromStr(const char *str);
	virtual int FindIndex(const char *str);
	virtual int FindIndex(int id);
	virtual int FindId(const char *str);
	virtual const char *FindStr(int id);
	virtual int Add(const char *str, int id);
	virtual int Remove(const char *str);
	virtual int Remove(int id);
};


} // namespace Laxkit

#endif

