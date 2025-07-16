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
//    Copyright (C) 2004-2007,2010 by Tom Lechner
//

#include <lax/iconmanager.h>
#include <lax/singletonkeeper.h>
#include <lax/strmanip.h>
#include <lax/fileutils.h>

#include <lax/debug.h>


namespace Laxkit {


//----------------------------- IconManager default ---------------------------

static SingletonKeeper loaderKeeper;
static IconManager *default_iconmanager = nullptr;

/*! If default_iconmanager is NULL, create and install a new Laxkit::IconManager as default.
 * Set with IconManager::SetDefault().
 * 
 * By default, there are NO icon paths defined. Use AddPath() to register new paths.
 */
IconManager* IconManager::GetDefault()
{
	if (!loaderKeeper.GetObject()) {
		default_iconmanager = new IconManager();
		loaderKeeper.SetObject(default_iconmanager, true);
	}
	return default_iconmanager;
}

/*! If calling with NULL, then destruct the default_windowmanager.
 * The next call to GetDefault() will create a new one.
 *
 * Will inc_count of newmanager (if not null).
 *
 * Returns what is now the default icon manager.
 */
IconManager* IconManager::SetDefault(IconManager *newmanager)
{
	if (newmanager == default_iconmanager) return default_iconmanager;

	loaderKeeper.SetObject(newmanager, false);
	default_iconmanager = newmanager;
	return default_iconmanager;
}


//----------------------------- IconNode ---------------------------
/*! \class IconNode
 * \brief Stacked in an IconManager.
 */


//! img's count is not incremented.
IconNode::IconNode(const char *nname, int nid, LaxImage *img)
{
	id=nid;
	name=newstr(nname);
	image=img;
	image->doneForNow();
}

IconNode::~IconNode()
{
	if (image) image->dec_count();
	if (name) delete[] name;
}


//----------------------------- IconManager ---------------------------
/*! \class IconManager
 * \brief Simplify maintenance of icons with this stack of IconNode objects.
 *
 * This is essentially a Laxkit::RefStackPtr<IconNode> with some helper functions
 * to ease lookup of icons as button boxes come and go. The stack is sorted by id.
 *
 * \todo Eventually, it might be in charge of generating pixmap icons from an icons.svg or
 *   icons.laidout or something of the kind.
 */


/*! Icons are added only when needed by GetIcon(), which uses findicon().
 */
IconManager::IconManager()
	: icon_path(LISTS_DELETE_Array),
	  broken(LISTS_DELETE_Array)
{
	remember_broken = true;
}

IconManager::~IconManager()
{
}

//! Return -1 for fail to load file.
int IconManager::InstallIcon(const char *nname, int nid, const char *file)
{
	LaxImage *img = ImageLoader::LoadImage(file);
	if (!img) return -1;
	return InstallIcon(nname,nid,img);
}

/*! If nid<=0, then make the id one more than the maximum nid in stack.
 * \todo *** must check that the icon is not already installed
 */
int IconManager::InstallIcon(const char *nname, int nid, LaxImage *img)
{
	//remove from broken if necessary
	for (int i=broken.n-1; i >= 0; i--) {
		if (strEquals(nname, broken.e[i]))
			broken.remove(i);
	}

	int c;
	if (nid<=0) {
		if (n) nid=e[n-1]->id+1; else nid=1;
		c=n;
	} else {
		c=0;
		while (c<n && e[c]->id>nid) c++;
	}
	
	IconNode *node = new IconNode(nname,nid,img);
	return push(node,1,c);
}

//! Search for a icon file "name.png" in all the icon paths.
/*! Install and return the icon if found, else NULL.
 * 
 * This function assumes that name is not already in the stack.
 */
LaxImage *IconManager::findicon(const char *name, bool save_broken)
{
	char *path;
	LaxImage *img=NULL;
	for (int c=0; c<icon_path.n; c++) {
		path=newstr(icon_path.e[c]);
		appendstr(path,"/");
		appendstr(path,name);
		appendstr(path,".png");
		img = ImageLoader::LoadImage(path);
		delete[] path;
		if (img) break;
	}

	if (img) {
		InstallIcon(name,-1,img);
		img->inc_count();

	} else if (save_broken) {
		int c;
		for (c=0; c<broken.n; c++) {
			if (!strcmp(broken.e[c], name)) break;
		}

		if (c == broken.n) broken.push(newstr(name));
	}

	return img;
}

/*! Return the first path that contains the file filename.
 */
const char *IconManager::FindPathForFile(const char *filename)
{
	if (isblank(filename)) return nullptr;

	for (int c = 0; c < icon_path.n; c++) {
		char *path = newstr(icon_path.e[c]);
		appendstr(path, "/");
		appendstr(path, filename);
		int status = file_exists(path, 1, nullptr);
		delete[] path;
		if (status) return icon_path.e[c];
	}
	return nullptr;
}

const char *IconManager::Broken(int i)
{
	if (i < 0 || i >= broken.n) return nullptr;
	return broken.e[i];
}

/*! For each name in broken, rescan from current paths.
 * Return the number that are still broken.
 */
int IconManager::ScanForBroken()
{
	for (int c=broken.n-1; c>= 0; c--) {
		LaxImage *img = nullptr;

		//scan existing in case the name snuck into broken list
		for (int c2=0; c2<PtrStack<IconNode>::n; c2++) {
			if (strEquals(broken.e[c], PtrStack<IconNode>::e[c2]->name)) {
				img = PtrStack<IconNode>::e[c2]->image;
			}
		}

		if (!img) img = findicon(broken.e[c], false);
		if (img) {
			// found! remove from broken
			broken.remove(c);
		}
	}

	return broken.n;
}

//! Return how many icons are currently installed.
int IconManager::HowMany()
{
	return PtrStack<IconNode>::n;
}

//! Returns the icon. The icon's count is incremented.
Laxkit::LaxImage *IconManager::GetIconByIndex(int index)
{
	if (index<0 || index>=PtrStack<IconNode>::n) return NULL;

	PtrStack<IconNode>::e[index]->image->inc_count();
	return PtrStack<IconNode>::e[index]->image;
}

//! Returns the the existing icon with id. The icon's count is incremented.
Laxkit::LaxImage *IconManager::GetIcon(int id)
{
	 //rather slow, but then, there won't be a million of them
	for (int c=0; c<PtrStack<IconNode>::n; c++)
		if (id==PtrStack<IconNode>::e[c]->id)  {
			PtrStack<IconNode>::e[c]->image->inc_count();
			return PtrStack<IconNode>::e[c]->image;
		}
	return NULL;
}

//! Returns the icon. The icon's count is incremented.
/*! If name does not exist, then the icon is searched for in the icon paths,
 * and the first one found is installed.
 */
Laxkit::LaxImage *IconManager::GetIcon(const char *name)
{
	 //rather slow, but then, there won't be a million of them
	for (int c=0; c<PtrStack<IconNode>::n; c++)
		if (strcmp(name,PtrStack<IconNode>::e[c]->name)==0) {
			PtrStack<IconNode>::e[c]->image->inc_count();
			return PtrStack<IconNode>::e[c]->image;
		}
	return findicon(name, remember_broken);
}

const char *IconManager::GetPath(int index)
{
	if (index<0 || index>=icon_path.n) return NULL;
	return icon_path.e[index];
}

//! Add path to index 0 position of the path stack.
/*! When using GetIcon(name), and name that is unrecognized, then all the icon paths are searched for
 * a loadable image named name.png.
 */
void IconManager::AddPath(const char *newpath)
{
	for (int c=0; c<icon_path.n; c++) {
		if (!strcmp(newpath, icon_path.e[c])) return; //already there!
	}
	icon_path.push(newstr(newpath),2,0);
}

/*! Return 0 for success, and removed, or 1 for oldpath not found.
 */
int IconManager::RemovePath(const char *oldpath)
{
	if (!oldpath) return 1;

	for (int c=0; c<icon_path.n; c++) {
		if (!strcmp(icon_path.e[c],oldpath)) {
			icon_path.remove(c);
			return 0;
		}
	}
	return 1;
}


/*! Load in all icons found in the paths. This means all openable image files.
 * Return the number of icons read in.
 */
int IconManager::PreloadAll()
{
	//PtrStack<char> icons_to_load(LISTS_DELETE_Array);
	//PtrStack<char> icon_ids(LISTS_DELETE_Array);
	
	char *base;
	int n = 0;

	for (int c = 0; c < icon_path.n; c++) {
		//scan directory recunsively: icon_path.e[c]
		std::cerr << " *** FINISH ME!!!" <<std::endl;
		
		const char *file = "";
		const char *bname = lax_basename(file);
		const char *ext = lax_extension(file);
		if (!bname) continue;
		if (!ext) ext = bname + strlen(bname);

		makenstr(base, bname, ext-bname);
	
		int i = -1;
		for (int c=0; c<PtrStack<IconNode>::n; c++) {
			if (strcmp(base, PtrStack<IconNode>::e[c]->name) == 0) {
				i = c;
				break;
			}
		}

		if (i >= 0) continue;

		LaxImage *img = ImageLoader::LoadImage(file);
		if (img) {
			InstallIcon(base, -1, img);
			n++;
		}
	}

	delete[] base;
	return n;
}



} //namespace Laxkit

