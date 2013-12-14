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
//    Copyright (C) 2004-2006 by Tom Lechner
//

#include <lax/lark.h>
#include <lax/lists.cc>
#include <lax/strmanip.h>



namespace Laxkit {

	
static PtrStack<char> lax_larks(2);
/*! lark_alpha_sorted[0] is index in lax_larks of lowest string.
 *  lark_alpha_sorted[lark_alpha_sorted.n-1] is index in lax_larks of highest string.
 */
static NumStack<int> lark_alpha_sorted;
	
//! Find the index in the lark_alpha_sorted stack that str would be found.
/*! If str is not there, then c is filled with that position in lark_alpha_sorted
 * that the id of the lark must be pushed into.
 *
 * \todo make this faster via binary search 
 */
static int lark_position(const char *str,int *c)
{
	int p,i;
	for (i=0; i<lark_alpha_sorted.n; i++) {
		p=strcmp(str,lax_larks.e[lark_alpha_sorted.e[i]]);
		if (p==0) { *c=i; return 1; }
		if (p>0) { *c=i; return 0; }
	}
	*c=i;
	return 0;
//	----------broken:
//	if (lark_alpha_sorted.n==0) return 0;
//	int cmp, d=0, s=0, e=lark_alpha_sorted.n-1, l=e/2;
//	do {
//		if (strcmp(lark_alpha_sorted.e[s],str)<=0) return s;
//		if (strcmp(lark_alpha_sorted.e[e],str)>=0) return e+1;
//		if (l==s || l==e) return e;
//		cmp=strcmp(lark_alpha_sorted.e[l],str);
//		if (cmp==0) return l;
//		if (cmp<0) e=l;
//			else   s=l;
//		l=(s+e)/2;
//	} while (s!=e)
//	return s;
}

//! Return pointer to string associated with id, or NULL if none.
/*! Yes, lark is a take off on Glib's quarks: Laxkit+quARK.
 *
 * Laxkit maintains a hash of strings and associated ids, that can be used
 * as a shortcut for checking string equality for commonly used strings, namely
 * event names. These are a replacement for X Atoms, so that Laxkit events do not
 * clutter up the X server.
 * 
 * \todo *** make threadsafe
 */
const char *lark_str_from_id(int id)
{
	return (id>0 && id<=lax_larks.n ? lax_larks.e[id-1] : NULL);
}

//! Return the id associated with str.
/*! If createifabsent!=0 and str is not known, then a new association is
 * added, and the new id is returned. 
 *
 * If createifabsent==0 and the string is not known, then 0 is returned. No string
 * can have 0 associated with it.
 * 
 * \todo *** make threadsafe
 */
int lark_id_from_str(const char *str, char createifabsent)
{
	int c;

	 // find lark
	if (lark_position(str,&c)) return lark_alpha_sorted.e[c]+1;

	 // create new lark
	if (createifabsent==0) return 0;
	lax_larks.push(newstr(str));
	lark_alpha_sorted.push(lax_larks.n,c);
	
	return lax_larks.n;
}

} // namespace Laxkit

