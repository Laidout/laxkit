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
//    Copyright (C) 2004-2009 by Tom Lechner
//


#include <lax/fileutils.h>
#include <lax/strmanip.h>

#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <climits>
#include <unistd.h>


//! Namespace for various file utilities.
/*! Contains functions to check file read/write ability, filename sanitizing, plus
 * a couple of parsing routines to extend the standard getline() to skip comments
 * and blank lines.
 *
 * Also the Attribute class, which is kind of a much simplified xml sort of thing,
 * such that data in files is arranged by indenting, rather than &lt;tags&gt;. It provides
 * reading and writing facilities for such files.
 *
 * Plus, there are a number of convenient conversion routines to convert 
 * char[]s to integers, longs, doubles,
 * integer arrays, etc.
 */
namespace LaxFiles {

////! Skip lines filled with space, tab, and comments. Assumes already positioned at beginning of line.
///*! \ingroup fileutils
// * Returns the current position of the file pointer.
// */
//long skipBlankLines(FILE *f,const char *comment)
//{*** broken should read in buffer and parse the buffer?
//	if (!f) return 0;
//	int ch;
//	long linestart;
//	char *line=NULL;
//	size_t n=0;
//	while (!feof(f)) {
//		linestart=ftell(f);
//		for (ch=fgetc(f); ch!='\n' && ch!=EOF && (ch==' ' || ch=='\t'); ch=fgetc(f)) ;
//		 // now f points to nonspace/tab *** but ch is first possibly non-ws char!!
//		if (ch==EOF) break;
//		if (ch=='\n') continue;
//		 // read in the line, and check if it is only a comment
//		c=getline(&line,&n,f);
//		if (!c) continue; // was line with just space and tab
//		if (!strncmp(comment,line,strlen(comment))) {
//			 // first thing after ws in line is comment
//			continue;
//		}
//		 // first in line was not comment so this is valid line.
//		fseek(f,linestart,SEEK_SET);
//		break;
//	}
//	if (line) free(line);
//	return ftell(f);
//}

//! Return the number of whitespace characters at the beginning of str.
/*! If strt!=NULL, then make strt point to the first non-whitespace character.
 */
int how_indented(char *str,char **strt)//strt=NULL;
{
	int i=0;
	while (isspace(str[i])) i++;
	if (strt) *strt=str+i;
	return i;
}




//! Read the line if indented>=indent, skipping blank lines and comments.
/*! comment is comment marker. It might be "#" or "//", etc. Any trailing whitespace
 * is ignored.
 *
 * If line is not indented enough, then reset file position to the start of
 * the non-blank line. If there is only ws and comments following, then the
 * file marker is advanced to eof.
 *
 * If line!=NULL, then it is assumed to have been malloc'd, not new'd.
 *
 * If lineindent!=NULL, then return how indented the current line actually is.
 * 
 * Returns the number of character bytes in the current line.
 */
int getline_indent_nonblank(char **line, size_t *n, IOBuffer &f, int indent, 
							const char *comment,char quote,char skiplines, int *lineindent)//skiplines=1
{
	long pos;
	int c=0,cc=0; //cc=total chars read, c in line
	int i=0; //actual indent of current line

	 //one iteration per line of the file. If the line is just whitespace or
	 //whitespace plus a comment, then skip it.
	while (!f.IsEOF()) {
		pos = f.Curpos();
		c   = f.GetLine(line,n);
		if (c <= 0) break;

		c = strlen(*line); //does getline read in \0 chars?
		cc += c;
		c -= cut_comment(*line,comment,quote);
		i  = how_indented(*line);

		 //if i==c, that means there was whitespace then comment
		if (skiplines) {
			if (c==0 || i==c) continue; //skip blank line..
		}
		if (i < indent) { // line too short, so return now with blank line..
			f.SetPos(pos);
			if (f.IsEOF()) f.Clearerr();
			if (*line) f.FreeGetLinePtr(*line); //free(*line);
			*line=NULL;
			*n=0;
			if (lineindent) *lineindent=i;
			return 0;
		}
		break; //else is on line indented at least enough..
	}
	if (i==c) i=0;
	if (lineindent) *lineindent=i;
	if (!skiplines) {
		 //remove trailing whitespace ONLY when line is not all whitespace if !skiplines
		if (c>0 && (*line)[c-1]=='\n') { (*line)[c-1]='\0'; c--; }
		i=0;
		while (i<c && isspace((*line)[i])) i++;
		if (i!=c) while (c>0 && isspace((*line)[c-1])) { (*line)[c-1]='\0'; c--; } //remove trailing whitespace
	} else while (c>0 && isspace((*line)[c-1])) { (*line)[c-1]='\0'; c--; } //remove trailing whitespace

	return c; //return the number of characters remaining in line
}

//! Remove single line comments. Return number of characters removed.
/*! Usually removes trailing whitespace. Does not remove whitespace 
 * at the front if there is non-whitespace stuff following.
 * Thus if there is only whitespace followed by a comment, the comment is removed, but
 * not the initial whitespace.
 * Does not reallocate str, but it does change the contents of str.
 * A comment starts with the first non escaped or quoted occurrence of cm, and
 * continues until the end of the line. Escaped characters are preceded by a '\'.
 * You can specify the quote and comment delimiter.
 *
 * Any trailing whitespace after cutting out the comment is removed.
 *
 * \todo *** this could go in strmanip
 * \todo there appears to be a snag when using dos files, the '\\r' does not 
 *   appear to be recognized as whitespace by isspace()....
 */
int cut_comment(char *str,const char *cm,char quote)//cm="#",quote='"'
{
	char *comment=NULL, //position of comment string, if any
		 *s=str;
	int n=strlen(str),rm=0;
	char escaped=0,inquote=0;
	comment=strstr(str,cm);
	if (comment!=str) while (comment) {
		if (!comment || comment==str) break; //comment is at beginning of line, or otherwise found
		
		 // check to see if the comment is within "" or escaped
		for ( ; s<comment; s++) {
			if (*s=='\\') escaped=!escaped;
			else if (escaped) escaped=0;
			if (*s==quote && !escaped) inquote=!inquote;
		}
		if (!escaped && !inquote) break; //unescaped, unquoted comment found
		comment=strstr(comment+strlen(cm),cm);
	}
	if (comment) { 
		rm=n-(comment-str);
		*comment='\0'; 
		n-=rm;
	}

	 // now remove any trailing whitespace as long as line is not all whitespace now.
	s=str;
	while (isspace(*s)) s++;
	if (s-str!=n) while (n && isspace(str[n-1])) {
		str[n-1]='\0';
		rm++;
	}
	return rm;
}

const char *legalfilechars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+=-_.%/ ";

//! Basic filename sanitizer checker. sanitize_filename() does actual sanitizing.
/*! \ingroup fileutils
 * Allows <tt>[a-zA-Z0-9+=-_.%/ ]</tt>.
 *
 * \todo this could be broken down into is_good_path() and is_good_basename();
 */
int is_good_filename(const char *filename)
{
	size_t legal=strspn(filename,legalfilechars);
	return legal==strlen(filename);
}

//! Check directory components for existence, and make them if (make_too).
/*! \ingroup fileutils
 * If everything checks out, then -1 is returned.
 *
 * Expects something like "aaaa/bbbbb/cccc/ddd". Each component must be a directory.
 * 
 * If make_too, this is like the command 'install -d 1/2/3', else return first dir (counting from
 * 0) that isn't there.
 * If making too, and a component exists already but is not a directory is encountered,
 * then abruptly stop, and return the index of the problem component (counting from 0).
 * Note that this may leave newly created directories lying around.
 *
 * \todo *** on failure, should remove newly created directories... when doing that, should
 *   check that those new dirs are still empty?
 * \todo *** always follows links, maybe should have some sanity check for that..
 */
int check_dirs(const char *dirs,char make_too)
{
	char *dir=newstr(dirs),*p;
	p=dir;
	while (*p=='/') p++;
	int c=-1,t=1;
	do {
		c++;
		p=strchr(p,'/');
		if (p) *p='\0';
		
		t=file_exists(dir,1,NULL);
		if (t && t!=S_IFDIR) { 
			 // existed, but was not a directory
			break;
		} else if (t==0) {
			 // create the dir
			if (mkdir(dir,0755)!=0) { t=-1; break; }
			t=S_IFDIR;
		}
		if (p) { *p='/'; p++; }
	} while (p);
	
	delete[] dir;
	return t!=S_IFDIR?c:-1;
}

/*! Create directories in dirs if they don't exist. Returns number of directories created.
 * If depth==0, then assume every path component in dirs is a directory.
 * If depth>0, then only do that many deep, so "1/2/3/4/5" with a depth of 3 will
 * create "1/2/3".
 * If depth<0, then create (num components)+depth, so "1/2/3/4/5" with a depth
 * of -1 will create "1/2/3/4".
 */
int CheckDirs(const char *dirstr, int depth)
{
	int n = 0, nn = 0;
	char **dirs = split(dirstr, '/', &n);

	if (depth > 0 || depth == 0) depth = n;
	if( depth < 0) depth = n + depth;
	if (depth<0) depth = 0;

	//store old curdir
	char *curdir = current_directory();
	
	int t = 0;
	if (chdir(dirs[0]) != 0) {
		//could not go to dir!!

	} else {
		for (int c=0; c<depth; c++) {
			t = file_exists(dirs[c],1,NULL);
			if (t && t != S_IFDIR) {
				 // existed, but was not a directory
				break;
			} else if (t == 0) {
				 // not there, create the dir
				if (mkdir(dirs[c],0755) != 0) { t =- 1; break; }
				t = S_IFDIR;
				chdir(dirs[c]);
			}
			nn++;
		}

		//restore old curdir
		chdir(curdir);
	}

	delete[] curdir;
	deletestrs(dirs, n);
	return t != S_IFDIR ? nn : -1;
}

//! Strip out characters not in: <tt>[a-zA-Z0-9+=-_.% ]</tt>.
/*! If the filename is nothing but illegal characters, then return NULL.
 */
char *sanitize_filename(char *filename,int makedup)
{
	if (!filename) return NULL;
	char *str;
	if (makedup) { str=newstr(filename); }
	else str=filename;
	//unsigned int n=strlen(str);
	size_t reject,allow;
	char *s=str;
	while (!*s) {
		allow=strspn(s,legalfilechars);
		s+=allow;
		if (!*s) break;
		reject=strcspn(s,legalfilechars);
		memmove(s,s+reject,reject);
	};
	return str;
}

//! Wrapper around stat().
/*! Return 0 for success, or whatever stat sets errno to when it fails.
 */
int lax_stat(const char *file, int followlink, struct stat *buf)
{
	int c;
	c=followlink?stat(file, buf):lstat(file, buf);
	if (c) return errno;
	return 0;
}
	
//! Return the size in bytes of the file, or -1 for error.
/*! This uses stat(), and error gets whatever stat sets errno to
 * when it fails.
 */
long file_size(const char *filename, int followlink, int *error)
{
	struct stat statbuf;
	int c;
	c=followlink?stat(filename, &statbuf):lstat(filename, &statbuf);
	if (c) {
		if (error) *error=errno;
		return -1;
	}
	if (error) *error=0;
	return statbuf.st_size;
}

//! Return statbuf.st_mode&S_IFMT if exists. Else return 0.
/*! This return value can be S_IFSOCK, S_IFREG, S_IFLNK, S_IFBLK, S_IFDIR, S_IFCHR, or S_IFIFO. 
 * These are defined in sys/stat.h, which also defines macros that you can use to check
 * if it is various things: S_ISREG(file_exists("blah.png",1,NULL)), S_ISDIR(...), etc.
 *
 * If stat fails on the file, then error is set to whatever errno was if error is non-null, and 0 is returned.
 */
int file_exists(const char *filename, int followlink, int *error)
{
	if (!filename) return 0;
	struct stat statbuf;
	int c;
	c=followlink?stat(filename, &statbuf):lstat(filename, &statbuf);
	if (c) {
		if (error) *error=errno;
		return 0;
	}
	if (error) *error=0;
	return statbuf.st_mode&S_IFMT;
}

//IsReadableDir
//
//IsDirectory
//
//IsRegularFile

//! Return if filename is a regular readable file.
/*! \ingroup fileutils
 * This attempts to open the file for reading to see if it is readable.
 * If ff==NULL, then close the file. Otherwise return the opened file in ff.
 *
 * \todo does not currently check sanity of file when opening.
 * \todo add a return error string
 */
int readable_file(const char *filename,FILE **ff)//ff=NULL
{
	struct stat statbuf;
	if (stat(filename, &statbuf)) {
		return 0;
	}
	unsigned long ft;
	ft=statbuf.st_mode&S_IFMT;

	if (ft!=S_IFREG) return 0;
	
	FILE *f;
	f=fopen(filename,"r");
	if (!f) return 0;
	if (ff) *ff=f; else fclose(f);
	return 1;
}

//! Collapses any excess stuff like '/a/../b' which becomes '/b'.
/*! \ingroup fileutils
 * Returns a new'd char[] if modorig==0. Otherwise, the content of origfile is
 * modified to be the simplified path (which is always smaller or of equal length
 * as the original), and origfile is returned. Also in this case, note that
 * origfile is not reallocated, the string is merely takes up less of the
 * space allocated to origfile.
 *
 * Uses the standard that "/../" (one up from root dir)
 * is the same as "/" (root dir itself).
 *
 * Also makes '//' be '/' and '/./x' is '/x'.
 *
 * This does not consult the file system, so if you have some convoluted
 * path with a billion symbolic links, then you're on your own. Also please note 
 * that "~/" is NOT expanded here. Use expand_home() for that.
 *
 * Returns the new path os success or NULL for invalid path.
 *
 * <pre>
 *  "../a/../"  ->  "../"
 *  "a/../"     ->  "."
 *  "//////"    ->  "/"
 *  "a/b/.."    ->  "a/"
 *  ""          ->  "." (if !modorig)
 *  ""          ->  ""  (if  modorig)
 *  "~/../a"    ->  "a" (WARNING! Does not expand '~')
 * </pre>
 */
char *simplify_path(char *origfile, int modorig)
{
	if (!origfile) return NULL;
	
	 // check for orig=="" should return "." but orig might not be big enough
	if (*origfile=='\0') {
		if (modorig) return origfile;
		else return newstr(".");
	}
	
	
	char *file=newstr(origfile);//modorig==0 means this gets returned eventually
	char *t,*t2; //temp pointers
	
	t=file;
	if (*t=='/') t++;
	while (*t) {
		 // here, t should be pointing to the start of a segment, 
		 // just after a '/'
		 
		 // check for something like "//"
		if (*t=='/') {
			memmove(t,t+1,strlen(t));
			continue;
		}
		
		 // check for something like "/./"
		if (t[0]=='.' && (t[1]=='/' || t[1]=='\0')) {
			if (t[1]=='/') memmove(t,t+2,strlen(t)-1);
			else *t='\0';
			continue;
		}
		
		 // check for something like "/../"
		if (t[0]=='.' && t[1]=='.' && (t[2]=='/' || t[2]=='\0')) {
			t2=t;
			if (t!=file) {
				t--;
				while (t>file && *(t-1)!='/') t--;
				if (t[0]=='.' && t[1]=='.' && (t[2]=='/' || t[2]=='\0')) {
					 // was "../.." so do nothing
					t=t2+2;
					if (*t) t++;
					continue;
				}
				if (t2[2]) memmove(t,t2+3,strlen(t2+3)+1);
				else memmove(t,t2+2,strlen(t2+2)+1);
			} else t+=3;
			continue;
		}
		
		while (*t && *t!='/') t++;
		if (*t=='/') t++;
	}

	if (*file=='\0') strcpy(file,".");
	if (modorig) {
		strcpy(origfile,file);
		delete[] file;
		return origfile;
	}
	return file;
}

//! For "~/blah", will return "file:///home/whoever/blah" for instance.
/*! If file already begins "file://" then nothing further is done, and a
 * copy of file is returned.
 *
 * Relative pathnames such as "blah", "../blah", and "./blah" will cause
 * NULL to be returned.
 *
 * Returns a new char[].
 */
char *file_to_uri(const char *file)
{
	if (!file) return NULL;
	if (!strncmp(file,"file://",7)) return newstr(file);
	if (file[0]!='/' && !(file[0]=='~' && file[1]=='/')) return NULL;

	char *str=newstr(file);
	if (str[0]=='~' && str[1]=='/') expand_home_inplace(str);
	prependstr(str,"file://");
	return str;
}

//! Return a new char[] with the expansion performed.
char *expand_home(const char *file)
{
	char *f=newstr(file);
	return expand_home_inplace(f);
}

//! Replace an initial "~/" with the $HOME environment variable.
/*! If file does not begin like that, then if modrig==0, then a new char[] copy
 * of the original is returned. If modorig!=0, then return file without reallocating.
 * 
 * If expansion does occurs, file is delete[]'d and set to a new char[], and the new value of file is
 * returned.
 */
char *expand_home_inplace(char *&file)
{
	char *expanded;
	if (file && file[0]=='~' && file[1]=='/') {
		expanded=newstr(file+2);
		prependstr(expanded,"/");
		prependstr(expanded,getenv("HOME"));
		
		delete[] file;
		file=expanded;

		return expanded;
	} 
	
	return file;
}

//! Convert "blah" to "/full/path/to/blah". Works with (const char *), rather than (char *).
/*! \ingroup fileutils
 *
 * Creates a new string and returns convert_to_full_path(f,path).
 */
char *full_path_for_file(const char *file, const char *path)
{
	char *f=newstr(file);
	return convert_to_full_path(f,path);
}

//! Basically convert 'blah' to '/full/path/to/blah', changing contents of file.
/*! \ingroup fileutils
 * This changes what file points to.
 *
 * If the file is not an absolute pathname, assumes that file is relative to 
 * the path, or if path==NULL, then the current directory. In other words,
 * if file[0]!='/', then prepend the directory. In any case, then collapse any excess stuff
 * like '/a/../b' which will become '/b'. Note that this does not check the
 * file system for existence of the file after collapsing any ".." and ".". 
 *
 * Note that this does NOT currently expand "~/".
 *
 * Returns whatever file points to now after conversion (after having deleted the old contents).
 * A NULL file returns NULL.
 */
char *convert_to_full_path(char *&file,const char *path)
{
	if (!file) return NULL;
	char *nfile=newstr(file);
	if (nfile[0]!='/') {
		char *dir=const_cast<char *>(path);
		if (!path) dir=getcwd(NULL,0);
		if (dir) {
			if (dir[strlen(dir)-1]!='/') prependstr(nfile,"/");
			prependstr(nfile,dir);
			if (!path) free(dir);
		}
	} 
	simplify_path(nfile,1);
	
	makestr(file,nfile);
	delete[] nfile;
	return file;
}

/*! Return whether file starts with "./" or "../", or is "." or "..".
 */
bool is_relative_path(const char *file)
{
	if (!file) return false;
	if (file[0]=='.' && file[1]=='\0') return true;
	if (file[0]=='.' && file[1]=='.' &&file[2]=='\0') return true;
	if (file[0]=='.' && file[1]=='/') return true;
	if (file[0]=='.' && file[1]=='.' &&file[2]=='/') return true;
	return false;
}

//! Return a new char[] containing the path to file, relative to relativeto.
/*! Both file and relativeto must be absolute paths.
 *
 * Say file="/home/blah/2/3.jpg" and relativeto="/home/blah/bleah/ack", then the
 * string returned is "../2/3.jpg" if isdir==0, and "../../2/3.jpg"
 * if isdir!=0. This does not actually consult the filesystem, so relativeto and file
 * need not currently exist to get a relative path. Plus, this means that "~/"
 * is not expanded.
 */
char *relative_file(const char *file,const char *relativeto,char isdir)
{
	if (!file || !relativeto) return newstr(file);

	const char *d1=file, *d11=file, *d2=relativeto, *d22=relativeto;

	 //skip common directories
	while (*d1 && *d2 && *d1==*d2) {
		if (*d1=='/') { d11=d1; d22=d2; }
		++d1;
		++d2;
	}

	file=d11;
	relativeto=d22;

	 // Now file simply needs as many "../" as there are remaining directories in relativeto
	int n=0;
	while (*d22) {
		++d22;
		if (*d22=='/') n++;
	}
	if (*(d22-1)!='/' && isdir) n++;
	char *newfile=new char[n*3+strlen(file)+1];
	newfile[0]='\0';
	while (n) strcat(newfile,"../");
	strcat(newfile,file);
	return newfile;
}

//char *relative_file(const char *file,const char *relativeto,char isdir,char *buffer,int len);
//{***} maybe have more functions to write to existing buffers

//! Return whether file has dir as its initial path.
int is_in_subdir(const char *file,const char *dir)
{
	int n=strlen(dir);
	if (strncmp(file,dir,n)) return 0;
	if (dir[n-1]!='/' && file[n]!='\0' && file[n]!='/') return 0;
	return 1;
}


//! Turns "blah###.eps" into "blah%03d.eps".
/*! \ingroup fileutils
 *
 * Any '%' chars that existed already are turned into "%%".
 *
 * Also "blah.eps" into "blah%d.eps", "blah" into "blah%d",
 * "blah/blah###.eps" to "blah/blah%03d.eps" and NULL into "%d".
 * 
 * Returns a new'd char[].
 *
 * \todo have something like "blah##-##.ext" -> "blah%02d-%02d.ext" and maybe
 *   "blah##\###" -> "blah%02d%03d"? pass in option to say how many fields there
 *   should be, and how many there ended up being
 */
char *make_filename_base(const char *f)
{
	if (!f) return newstr("%d");
	char *newf;
	const char *p;
	int n=0;
	p=strchr(f,'#');
	if (!p) p=strrchr(f,'.');
	if (!p) p=f+strlen(f);
	while (p && *p=='#') { p++; n++; }
	newf=new char[strlen(f)-n+6];
	if (n>20) n=20;
	if (p-f-n) strncpy(newf,f,p-f-n);
	if (n) sprintf(newf+(p-f)-n,"%%0%dd",n);
	else sprintf(newf+(p-f),"%%d");
	//else newf[p-f-n]='\0';
	if (*p) strcat(newf,p);

	return newf;
}

//! Return a new, null terminated char[] containing the contents of file, or NULL on error.
/*! If chars_ret!=NULL, then return the number of characters actually read, (not including
 * the final '\\0'.
 *
 * Please note that there is no binary checking done, so the array may contain '\\0' characters
 * not at the end.
 *
 * If maxchars>0, then read at most this many characters.
 */
char *read_in_whole_file(const char *file, int *chars_ret, int maxchars)
{
	if (isblank(file)) return NULL;
	if (file_exists(file,1,NULL) != S_IFREG) return NULL;
	int size = file_size(file,1,NULL);
	if (size <= 0) return NULL;
	if (maxchars > 0 && size > maxchars) size = maxchars;
	FILE *f = fopen(file,"r");
	if (!f) return NULL;
	char *str = new char[size+1];
	int numread = fread(str,1,size,f);
	if (numread <= 0) { delete str; str=NULL; numread=0; }
	else str[numread]='\0';
	fclose(f);
	if (chars_ret) *chars_ret = numread;
	return str;
}

/*! Like read_in_whole_file(), but on open FILE where you don't know how much is there.
 * Here, grab chunks of 1024 bytes until eof.
 * Return number of bytes read in chars_read_ret.
 * Makes returned_bytes[chars_read_ret]='\\0'.
 */
char *pipe_in_whole_file(FILE *f, int *chars_read_ret)
{
	char *data = NULL; //the data
	char *buf = NULL;  //running counter per read chunk within data
	int c;
	int n = 0, max = 0;

	data = new char[1024];
	buf = data;
	max = 1024;


	while (!feof(f)) {
		c = fread(buf,1,1024, f);
		n += c;

		if (c == 0) {
			if (feof(f)) {
				break;
			} else if (ferror(f)) {
				break;
			}
		}

		 //realloc
		if (!feof(f)) {
			char *ndata = new char[max+1024];
			memcpy(ndata, data, max);
			delete[] data;
			data = ndata;
			buf = data + n;
			max += 1024;
		}
	}
	
	if (n==max) {
		 //this'll only happen about 1/1024 times
		char *ndata = new char[max+5];
		memcpy(ndata, data, max);
		delete[] data;
		data = ndata;
		max += 5;
	}
	data[n] = '\0';

	if (chars_read_ret) *chars_read_ret = n;
	return data;
}

/*! Return 0 for success, or nonzero for couldn't.
 * If n<0, use strlen(str).
 */
int save_string_to_file(const char *str,int n, const char *file)
{
	FILE *ff = fopen(file, "w");
	if (!ff) return 1;

	if (n<0) n = strlen(str);
	fwrite(str,1,n, ff);

	fclose(ff);

	return 0;
}

/*! Uses get_current_dir_name(), then returns a new char[], not the malloc'd one get_current_dir_name fetches.
 */
char *current_directory()
{
	char *t=get_current_dir_name(); // this is a gnu extension _GNU_SOURCE must have been defined somewhere..
    char *npath=newstr(t);
	free(t);
	return npath;
}


/*! Return new char[] with path to current executable, grabbed from /proc/self/exe.
 */
char *ExecutablePath()
{
	char path[PATH_MAX];
    ssize_t nbytes = readlink("/proc/self/exe", path, PATH_MAX-1);
	if (nbytes > 0) return newstr(path);
	return nullptr;
}



} //namespace LaxFiles



#ifdef _LAX_PLATFORM_MAC


/*! Return -1 for eof or other failure.
 * Otherwise, return the number of characters read.
 */
long getline(char **lineptr, size_t *n, FILE *stream)
{
    size_t len  = 0;
    size_t last = 0;
	char *nl;

	if (feof(stream)) return -1;

    do {
		if ((*n)==0) {
			 //initialize buffer
			*n = BUFSIZ;
			*lineptr = realloc(*lineptr, *n);
		}

        /* Actually do the read. Note that fgets puts a terminal '\0' on the
           end of the string, so we make sure we overwrite this */
        fgets((*lineptr) + last, *n-last, stream);
        len = strlen(*lineptr);
		
		nl = strchr(*lineptr, '\n');
		if (nl) {
			len = nl-(*lineptr);
			*(nl+1)='\0';
			return len;

		}
		
		last = len - 1;

		 //lengthen buffer
		size += BUFSIZ; //BUFSIZ is defined as "the optimal read size for this platform"
		*lineptr = realloc(buf,size); 

    } while (!feof(f) && (*lineptr)[last]!='\n');


    return buf;
}



#endif  //_LAX_PLATFORM_MAC





