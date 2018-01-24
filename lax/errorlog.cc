//
//	
// Laidout, for laying out
// Please consult http://www.laidout.org about where to send any
// correspondence about this software.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// For more details, consult the COPYING file in the top directory.
//
// Copyright (C) 2012 by Tom Lechner
//

#include <lax/strmanip.h>
#include "errorlog.h"


//template implementation:
#include <lax/lists.cc>

#include <cstdio>
#include <cstdarg>

#include <iostream>
using namespace std;
#define DBG



namespace Laxkit {



//---------------------------------- ErrorLog -----------------------------


//! Dump to cout.
void dumperrorlog(const char *mes,ErrorLog &log)
{
	if (mes) cout <<mes<<"("<<log.Total()<<")"<<endl;
	ErrorLogNode *e;
	for (int c=0; c<log.Total(); c++) {
		e=log.Message(c);
		if (e->severity==ERROR_Ok) cout <<"Ok: ";
		else if (e->severity==ERROR_Warning) cout <<"Warning: ";
		else if (e->severity==ERROR_Fail) cout <<"Error! ";

		cout <<e->description<<", id:"<<e->object_id<<","<<(e->objectstr_id?e->objectstr_id:"(no str)")<<" ";
		if (e->path) cout << e->path;
		cout <<endl;

	}
}

//---------------------------------- ErrorLogNode
/*! \class ErrorLogNode
 * \brief Stack node for ErrorLog class.
 */

ErrorLogNode::ErrorLogNode()
{
	path=NULL;
	objectstr_id=NULL;
	object_id=0;
	description=NULL;
	severity=0;
	info=0;
}

ErrorLogNode::ErrorLogNode(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int nseverity,
							int ninfo, int npos,int nline)
{
	path=newstr(npath);
	description=newstr(desc);
	severity=nseverity;
	object_id=objid;
	objectstr_id=newstr(objidstr);
	info=ninfo;
	pos=npos;
	line=nline;
}

ErrorLogNode::~ErrorLogNode()
{
	if (description) delete[] description;
	if (objectstr_id) delete[] objectstr_id;
}

void ErrorLogNode::Set(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int nseverity,
						int ninfo, int npos,int nline)
{
	makestr(path,npath);
	makestr(description,desc);
	severity=nseverity;
	object_id=objid;
	makestr(objectstr_id,objidstr);
	info=ninfo;
	pos=npos;
	line=nline;
}

//---------------------------------- ErrorLog
/*! \class ErrorLog
 * \brief Class to simplify keeping track of offending objects.
 *
 * This is used by importers and exporters to tag and describe various incompatibilities.
 * It allows one to quickly locate problems.
 */

ErrorLog::ErrorLog()
{
}

ErrorLog::~ErrorLog()
{
}

void ErrorLog::Clear()
{ messages.flush(); }


//! Override if subclassed error logs require extra info in a log message.
ErrorLogNode *ErrorLog::newErrorLogNode()
{ return new ErrorLogNode(); }

const char *ErrorLog::Message(int i,int *severity,int *info, int *pos,int *line)
{
	if (i>=0 && i<messages.n) {
		if (severity) *severity=messages.e[i]->severity;
		if (info)     *info    =messages.e[i]->info;
		if (pos)      *pos     =messages.e[i]->pos;
		if (line)     *line    =messages.e[i]->line;
		return messages.e[i]->description;
	}

	if (severity) *severity=-1;
	if (info) *info=0;
	if (pos) *pos=-1;
	if (line) *line=-1;
	return NULL;
}

/*! Printf style variadic version.
 */
int ErrorLog::AddMessage(int severity, int ninfo, int npos,int nline, const char *fmt, ...)
{
	va_list arg;

    va_start(arg, fmt);
    int c = vsnprintf(NULL, 0, fmt, arg);
    va_end(arg);

	char *message = new char[c+1];
	va_start(arg, fmt);
	vsnprintf(message, c+1, fmt, arg);
	va_end(arg);

	int status = AddMessage(0,NULL,NULL, message, severity,ninfo, npos,nline);
	delete[] message;
	return status;
}

int ErrorLog::AddMessage(const char *desc, int severity, int ninfo, int pos,int line)
{
	return AddMessage(0,NULL,NULL,desc,severity,ninfo, pos,line);
}

/*! Returns number of messages including this one.
 */
int ErrorLog::AddMessage(unsigned int objid, const char *objidstr, const char *npath, const char *desc, int severity, int ninfo, int pos,int line)
{
	ErrorLogNode *node=newErrorLogNode();
	node->Set(objid,objidstr,npath, desc,severity,ninfo,pos,line);
	messages.push(node);
	return messages.n;
}

//! Return message i, or NULL if i out or range.
const char *ErrorLog::MessageStr(int i)
{
	if (i>=0 && i<messages.n) return messages.e[i]->description;
	return NULL;
}

//! Return new char[] with all messages, or NULL if there are none.
char *ErrorLog::FullMessageStr()
{
	if (!messages.n) return NULL;

	char *str=NULL;
	for (int c=0; c<messages.n; c++) {
		 //write out something like: "objectstr_id (path):\n" or "path:\n"
		if (messages.e[c]->objectstr_id) appendstr(str, messages.e[c]->objectstr_id);
		if (messages.e[c]->path) {
			if (messages.e[c]->objectstr_id) appendstr(str, " (");
			appendstr(str, messages.e[c]->path);
			if (messages.e[c]->objectstr_id) appendstr(str, "):\n");
			else appendstr(str, ":\n"); 
		} else if (messages.e[c]->objectstr_id) {
			appendstr(str, "\n");
		}

		appendline(str,messages.e[c]->description);
	}
	return str;
}

ErrorLogNode *ErrorLog::Message(int i)
{
	if (i>=0 && i<messages.n) return messages.e[i];
	return NULL;
}

//! Return the number of ok notes.
int ErrorLog::Oks(int since)
{
	if (since<0) since=0;
	int n=0;
	for (int c=0; c<messages.n; c++) if (messages.e[c]->severity==ERROR_Ok) n++;
	return n;
}

//! Return the number of warnings.
int ErrorLog::Warnings(int since)
{
	if (since<0) since=0;
	int n=0;
	for (int c=0; c<messages.n; c++) if (messages.e[c]->severity==ERROR_Warning) n++;
	return n;
}

//! Return the number of failing errors.
int ErrorLog::Errors(int since)
{
	if (since<0) since=0;
	int n=0;
	for (int c=0; c<messages.n; c++) if (messages.e[c]->severity==ERROR_Fail) n++;
	return n;
}

} //namespace Laxkit



