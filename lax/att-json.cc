
//---------------------------------- Json Conversion helpers -------------------------------
int AttributeToJsonFile(FILE *f, Attribute *att, int indent);
char *AttributeToJson(Attribute *att,char **appendtothis, char **error_ret);
Attribute *JsonFileToAttribute (const char *cssfile,Attribute *att);
Attribute *JsonToAttribute (const char *css,Attribute *att);





int AttributeToJsonFile(FILE *f, Attribute *att, int indent)
{
}

/*! Negative indent means output with no whitespace.
 */
char *AttributeToJson(Attribute *att,char **appendtothis, int indent, char **error_ret)
{
	char spc[indent<0?0:indent+1]; memset(spc,' ',indent<0?0:indent); spc[indent<0?0:indent]='\0';

//	if (indent<0) appendstr(*appendtothis,"{");
//	else {
//		appendstr(*appendtothis,spc);
//		appendstr(*appendtothis,"{\n");
//	}

	for (int c=0; c<att->attributes.n; c++) {
		if (indent>0) appendstr(*appendtothis,spc);

		 //name
		appendstr(*appendtothis,"\"");
		appendstr(*appendtothis,att->attributes.e[c]->name);
		appendstr(*appendtothis,"\": ");

		 //value
		if (***is string) {
		} else (*** is value) {
		} else (*** is object) {
			appendstr(*appendtothis,"{");
			AttributeToJson(att->attributes.e[c],appendtothis,indent+4,error_ret);
			if (indent>0) appendstr(*appendtothis,spc);
			appendstr(*appendtothis,"}");

		} else (*** is array) {
		}

		if (c<att->attributes.n-1) appendstr(*appendtothis,",");
	}


//	appendstr(*appendtothis,"}\n");
}

Attribute *JsonFileToAttribute (const char *cssfile,Attribute *att)
{
}

Attribute *JsonToAttribute (const char *css,Attribute *att)
{
}

