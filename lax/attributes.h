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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_ATTRIBUTES_H
#define _LAX_ATTRIBUTES_H

#include <lax/lists.h>
#include <lax/vectors.h>
#include <lax/anobject.h>
#include <lax/iobuffer.h>
#include <lax/screencolor.h>

#include <cstdio>
#include <functional>


namespace Laxkit {

	
//---------------------------------- class Attribute ---------------------------------


enum AttributeTypes {
	ATT_None = -1,
	ATT_Att = 0,
	ATT_Json,
	ATT_Xml,
	ATT_Css,
	ATT_MAX
};

class Attribute {
 public:
	char *name;
	char *value;
	char *atttype; // hint about what data type value is
	char *comment;
	Laxkit::PtrStack<Attribute> attributes;

	unsigned int flags;

	Attribute() { name = value = atttype = comment = nullptr;  flags = 0; }
	Attribute(const char *nn, const char *nval, const char *nt=nullptr);
	virtual ~Attribute();
	virtual Attribute *duplicateAtt();
	virtual anObject *duplicate() { return dynamic_cast<anObject*>(duplicateAtt()); }
	virtual Attribute  *find      (const char *fromname, int *i_ret=nullptr);
	virtual const char *findValue (const char *fromname, int *i_ret=nullptr);
	virtual double      findDouble(const char *fromname, int *i_ret=nullptr);
	virtual long        findLong  (const char *fromname, int *i_ret=nullptr);
	virtual Attribute *pushSubAtt(const char *nname, const char *nvalue=nullptr, const char *ncomment=nullptr);
	virtual int push(Attribute *att, int where);
	virtual int push(const char *nname);
	virtual int pushn(const char *nname, int len);
	virtual int pushStr(const char *nname, int where, const char *fmt, ...);
	virtual int push(const char *nname,const char *nval, const char *ncomment, int where=-1);
	virtual int push(const char *nname,const char *nval,  int where=-1);
	virtual int push(const char *nname,long nval,         int where=-1);
	virtual int push(const char *nname,unsigned long nval,int where=-1);
	virtual int push(const char *nname,int nval,          int where=-1);
	virtual int push(const char *nname,double nval,       int where=-1);
	virtual Attribute *Top() { if (attributes.n) return attributes.e[attributes.n-1]; return nullptr; }
	virtual int remove(int index);
	virtual void clear();
	virtual void Comment(const char *ncomment);
	virtual int NumAtts() { return attributes.n; }
	virtual Attribute *Att(int index) { return index >= 0 && index < attributes.n ? attributes.e[index] : nullptr; }

	virtual int   dump_in     (const char *filename, int what=0);
	virtual int   dump_in_str (const char *str);
	virtual int   dump_in_json(const char *str);
	virtual int   dump_in_xml (const char *str);

	virtual int   dump_in     (FILE *f,             int Indent,Attribute **stopatsub=nullptr);
	virtual int   dump_in     (IOBuffer &f, int Indent,Attribute **stopatsub=nullptr);

	virtual char *dump_in_indented (IOBuffer &f, int indent);
	virtual char *dump_in_until    (IOBuffer &f, const char *str, int indent=0);

	virtual void  dump_out         (FILE *f, int Indent);
	virtual void  dump_out_full    (FILE *f, int Indent);
};

class AttributeObject : public Laxkit::anObject, public Attribute
{
  public:
	anObject *data;
	AttributeObject();
	AttributeObject(const char *nn, const char *nval,const char *nt=nullptr);
	virtual ~AttributeObject();
	virtual Attribute *duplicateAtt(); 
	virtual void SetData(anObject *ndata, int absorb);
};

//---------------------------------- Dump helper functions ---------------------------------
void dump_out_value(FILE *f, int indent, const char *value, int valuewidth = 1, const char *comment = nullptr, int commentindent = 1);
void dump_out_escaped(FILE *f, const char *str, int n);
void dump_out_indented(FILE *f, int indent, const char *str);
void dump_out_quoted(FILE *f, const char *value, char quote);
void dump_out_comment(FILE *f, int indent, const char *comment);

char *escape_string(const char *value, char quote, bool include_quotes);

void skip_to_next_attribute(IOBuffer &f, int indent);

//---------------------------------- Basic Type Conversion Routines -----------------------------------	
int ByteSizeAttribute(const char *s, long *ll, char towhat);
double *TransformAttribute(const char *v,double *m,char **endptr=nullptr);
bool IsOnlyDouble(const char *v, int len, double *d_ret);
int DoubleAttribute(const char *v,double *d,char **endptr=nullptr);
int DoubleListAttribute(const char *v,double *d,int maxn,char **endptr=nullptr);
int DoubleListAttribute(const char *v,double **d,int *n_ret);
int FloatAttribute(const char *v,float *f,char **endptr=nullptr);
bool IsOnlyInt(const char *v, int len, int *i_ret);
int IntAttribute(const char *v,int *i,char **endptr=nullptr);
int UIntAttribute(const char *v,unsigned int *i,char **endptr=nullptr);
int LongAttribute(const char *v,long *l,char **endptr=nullptr);
int ULongAttribute(const char *v,unsigned long *l,char **endptr=nullptr);
int IntListAttribute(const char *v,int *i,int maxn,char **endptr=nullptr);
int IntListAttribute(const char *v,int **i,int *n_ret,char **endptr=nullptr);
char *QuotedAttribute(const char *v,char **endptr=nullptr);
char *WholeQuotedAttribute(const char *v);
int BooleanAttribute(const char *v);
int NameValueAttribute(const char *str, char **name, char **value, char **end_ptr,
					   char assign='=',char delim=0,const char *stopat = nullptr, const char *stopat2 = nullptr);
Attribute *NameValueToAttribute(Attribute *att,const char *str, char assign, char delim);
int SimpleColorAttribute(const char *v,unsigned long *color_ret, Laxkit::ScreenColor *scolor_ret, const char **end_ptr);
int SimpleColorAttribute(const char *v, double *colors, const char **end_ptr);
int HexColorAttributeRGB(const char *v,unsigned long *l,const char **endptr);
int HexColorAttributeRGB(const char *v,Laxkit::ScreenColor *color,const char **endptr);
int FlatvectorAttribute(const char *v,flatvector *l,char **endptr=nullptr);
int SpacevectorAttribute(const char *v,spacevector *l,char **endptr=nullptr);
int QuaternionAttribute(const char *v,Quaternion *l,char **endptr=nullptr);


//---------------------------------- XML Conversion helpers -------------------------------
int AttributeToXMLFile(FILE *f, Attribute *att, int indent);
int SubAttributesToXMLFile(FILE *f, Attribute *att, int indent);
char *AttributeToXML(Attribute *att,char *&appendtothis, char **error_ret);
Attribute *XMLFileToAttribute (Attribute *att,const char *file,const char **stand_alone_tag_list);
Attribute *XMLFileToAttributeLocked (Attribute *att,const char *file,const char **stand_alone_tag_list);
Attribute *XMLChunkToAttribute(Attribute *att,FILE *f,const char **stand_alone_tag_list);
Attribute *XMLChunkToAttribute(Attribute *att,const char *buf,long n,
							   long *C,const char *until,const char **stand_alone_tag_list);


//---------------------------------- CSS Conversion helpers -------------------------------
int AttributeToCSSFile(FILE *f, Attribute *att, int indent);
char *AttributeToCSS(Attribute *att,char **appendtothis, char **error_ret);
Attribute *CSSFileToAttribute (const char *cssfile,Attribute *att);
Attribute *CSSToAttribute (const char *css,Attribute *att);
Attribute *InlineCSSToAttribute (const char *css,Attribute *att);


//---------------------------------- JSON Conversion helpers -------------------------------
enum JsonAttTypes {
	JSON_Null = 0,
	JSON_True,
	JSON_False,
	JSON_Int,
	JSON_Float,
	JSON_String,
	JSON_Array,
	JSON_Object,
	JSON_Object_Name,

	JSON_MAX
};

char *AttributeToJsonString(Attribute *att,char **appendtothis, int indent, char **error_ret);
int AttributeToJsonFile(const char *jsonfile, Attribute *att, int indent);
int DumpAttributeToJson(FILE *f, Attribute *att, int indent);
Attribute *JsonFileToAttribute (const char *jsonfile, Attribute *att);
Attribute *JsonStringToAttribute (const char *jsonstring, Attribute *att, const char **end_ptr);


//---------------------------------- CSV Conversion helpers -------------------------------
Attribute *CSVFileToAttribute  (Attribute *att, const char *file, const char *delimiter, bool has_headers, int *error_ret);
Attribute *CSVStringToAttribute(Attribute *att, const char *str, const char *delimiter, bool has_headers, int *error_ret);
Attribute *CSVToAttribute(Attribute *att, IOBuffer &f, const char *delimiter, bool has_headers, int *error_ret);
int ParseCSV(IOBuffer &f, const char *delimiter, bool has_headers,
		std::function<void()> NewRow,
		std::function<void(const char *content, int len)> NewHeader,
		std::function<void(const char *content, int len)> NewCell,
		std::function<void(const char *error)> OnError
	);


} //namespace Laxkit

#endif

