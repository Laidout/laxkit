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
//    Copyright (C) 2012-2018 by Tom Lechner
//
#ifndef ERRORLOG_H
#define ERRORLOG_H


#include <lax/anobject.h>
#include <lax/lists.h>
#include <cstdarg>


namespace Laxkit {


//---------------------------------- ErrorLog -----------------------------

/*! ERROR_Ok, means everything checks out.
 * ERROR_Fail is an extreme error which should interrupt whatever you are doing.
 * ERROR_Warning is a generic error which does not halt anything, but users can attend to it.
 * Other values above ERROR_MAX can be used for other warnings.
 */
enum ErrorSeverity {
		ERROR_Unknown =-1,
		ERROR_Ok      =0,
		ERROR_Fail    =1,
		ERROR_Warning =2,
		//ERROR_Missing_Glyphs,
		//ERROR_Unexposed_Text,
		//ERROR_Has_Transparency,
		//ERROR_Broken_Image,
		//ERROR_Broken_Resource,
		//ERROR_Image_Not_At_Desired_Resolution,
		ERROR_MAX

	};

class ErrorLogNode
{
  public:
	char *path;
	char *objectstr_id;
	unsigned int object_id;
	char *description;
	int severity; //"ok", "warning", "fail", "version fail"
	int info, pos,line; //extra info
	anObject *extra;
	ErrorLogNode();
	ErrorLogNode(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int nseverity,int ninfo, int npos,int nline,anObject *ext);
	virtual ~ErrorLogNode();
	virtual void Set(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int nseverity,int ninfo, int npos,int nline,anObject *ext);
};

class ErrorLog
{
  protected:
	virtual ErrorLogNode *newErrorLogNode();
  public:
	Laxkit::PtrStack<ErrorLogNode> messages;

	ErrorLog();
	virtual ~ErrorLog();
	virtual int AddError(int ninfo, int npos,int nline, const char *fmt, ...);
	virtual int AddError(const char *desc, int ninfo=0, int npos=0,int nline=0);
	virtual int AddWarning(int ninfo, int npos,int nline, const char *fmt, ...);
	virtual int AddWarning(const char *desc, int ninfo=0, int npos=0,int nline=0);
	virtual int vAddMessage(int severity, int ninfo, int npos,int nline, const char *fmt, va_list arg);
	virtual int AddMessage(int severity, int ninfo, int npos,int nline, const char *fmt, ...);
	virtual int AddMessage(const char *desc, int severity, int ninfo=0, int npos=0,int nline=0, anObject *extra = nullptr);
	virtual int AddMessage(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int severity, int ninfo=0, int npos=0,int nline=0, anObject *extra = nullptr);
	virtual const char *Message(int i,int *severity,int *info, int *pos = nullptr, int *line = nullptr);
	virtual int Total() { return messages.n; }
	virtual ErrorLogNode *Message(int i);
	virtual ErrorLogNode *LastMessage();
	virtual const char *MessageStr(int i);
	virtual char *FullMessageStr();
	virtual int Warnings(int since=0);
	virtual int Errors(int since=0);
	virtual int Oks(int since=0);
	virtual void Clear();
};

void dumperrorlog(const char *mes,ErrorLog &log);


} //namespace Laxkit


#endif

