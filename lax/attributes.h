//
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
//    Copyright (C) 2004-2013 by Tom Lechner
//
#ifndef _LAX_ATTRIBUTES_H
#define _LAX_ATTRIBUTES_H

#include <lax/screencolor.h>
#include <lax/lists.h>
#include <lax/vectors.h>
#include <cstdio>

namespace LaxFiles {
	
//---------------------------------- class Attribute ---------------------------------

class Attribute {
 public:
	char *name;
	char *atttype; // string, alias to this name, use contents of file "value" as string
	Laxkit::PtrStack<Attribute> attributes;

	char *value;
	unsigned int flags;

	Attribute() { name=value=atttype=NULL; flags=0; }
	Attribute(const char *nn, const char *nval,const char *nt=NULL);
	virtual ~Attribute();
	virtual Attribute *duplicate();
	virtual Attribute *find(const char *fromname,int *i_ret=NULL);
	virtual const char *findValue(const char *fromname,int *i_ret=NULL);
	virtual double findDouble(const char *fromname,int *i_ret=NULL);
	virtual long findLong(const char *fromname,int *i_ret=NULL);
	virtual Attribute *pushSubAtt(const char *nname, const char *nvalue=NULL);
	virtual int push(Attribute *att,int where);
	virtual int push(const char *nname);
	virtual int push(const char *nname,const char *nval,int where=-1);
	virtual int push(const char *nname,long nval,int where=-1);
	virtual int push(const char *nname,unsigned long nval,int where=-1);
	virtual int push(const char *nname,int nval,int where=-1);
	virtual int push(const char *nname,double nval,int where=-1);
	virtual Attribute *Top() { if (attributes.n) return attributes.e[attributes.n-1]; return NULL; }
	virtual int remove(int index);
	virtual void clear();
	virtual int dump_in(const char *filename, Attribute *defines=NULL);
	virtual int   dump_in          (FILE *f, int Indent,Attribute **stopatsub=NULL);
	virtual char *dump_in_indented (FILE *f, int indent);
	virtual char *dump_in_until    (FILE *f, const char *str, int indent=0);
	virtual void  dump_out         (FILE *f, int Indent);
	virtual void  dump_out_full    (FILE *f, int Indent);
};

class AttributeObject : public Attribute
{
  public:
	AttributeObject();
	AttributeObject(const char *nn, const char *nval,const char *nt=NULL);
	virtual ~AttributeObject();
	virtual Attribute *duplicate(); 
};

//---------------------------------- Dump helper functions ---------------------------------
void dump_out_value(FILE *f,int indent,const char *value, int noquotes=0);
void dump_out_escaped(FILE *f, const char *str, int n);
void dump_out_indented(FILE *f, int indent, const char *str);

void skip_to_next_attribute(FILE *f,int indent);

//---------------------------------- Value Conversion Routines -----------------------------------	
int ByteSizeAttribute(const char *s, long *ll, char towhat);
double *TransformAttribute(const char *v,double *m,char **endptr=NULL);
int DoubleAttribute(const char *v,double *d,char **endptr=NULL);
int DoubleListAttribute(const char *v,double *d,int maxn,char **endptr=NULL);
int DoubleListAttribute(const char *v,double **d,int *n_ret);
int FloatAttribute(const char *v,float *f,char **endptr=NULL);
int IntAttribute(const char *v,int *i,char **endptr=NULL);
int UIntAttribute(const char *v,unsigned int *i,char **endptr=NULL);
int LongAttribute(const char *v,long *l,char **endptr=NULL);
int ULongAttribute(const char *v,unsigned long *l,char **endptr=NULL);
int IntListAttribute(const char *v,int *i,int maxn,char **endptr=NULL);
int IntListAttribute(const char *v,int **i,int *n_ret,char **endptr=NULL);
char *QuotedAttribute(const char *v,char **endptr=NULL);
char *WholeQuotedAttribute(const char *v);
int BooleanAttribute(const char *v);
int NameValueAttribute(const char *str, char **name, char **value, char **end_ptr,
					   char assign='=',char delim=0,const char *stopat=NULL);
Attribute *NameValueToAttribute(Attribute *att,const char *str, char assign, char delim);
int SimpleColorAttribute(const char *v,unsigned long *color_ret, Laxkit::ScreenColor *scolor_ret, const char **end_ptr);
int SimpleColorAttribute(const char *v, double *colors, const char **end_ptr);
int HexColorAttributeRGB(const char *v,unsigned long *l,const char **endptr);
int HexColorAttributeRGB(const char *v,Laxkit::ScreenColor *color,const char **endptr);
int FlatvectorAttribute(const char *v,flatvector *l,char **endptr=NULL);


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


} //namespace LaxFiles

#endif

