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
//    Copyright (C) 2024 by Tom Lechner
//

#include <lax/laxzip.h>
#include <lax/debug.h>

#include <iostream>

using namespace std;


namespace Laxkit {


//----------------------------------- ZipReader ----------------------------

/*! \class ZipReader
 * C++ wrapper for libzip, specifically for reading zip files.
 */


ZipReader::ZipReader()
{
}


ZipReader::~ZipReader()
{
	if (zip) {
		zip_close(zip);
	}
}


bool ZipReader::Open(const char *path)
{
	if (zip || !path || path[0] == '\0') zip_close(zip);

	int err = 0;
    if ((zip = zip_open(path, ZIP_RDONLY, &err)) == NULL) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
 		DBG cerr << "cannot open zip archive " << path <<": "<< zip_error_strerror(&error) << endl;
        zip_error_fini(&error);
        return false;
    }
	return true;
}


bool ZipReader::Close()
{
	if (!zip) return false;
	zip_close(zip);
	zip = nullptr;
	return true;
}


int ZipReader::NumEntries()
{
	if (!zip) return -1;
	int num = zip_get_num_entries(zip, 0);
	return num;
}


///*! Return the index of fname, or -1 if not found.
// */
//int ZipReader::FindEntry(const char *fname)
//{
//
//}


Utf8String ZipReader::EntryName(int index)
{
	const char *name = zip_get_name(zip, index, 0);
	return Utf8String(name);
}


unsigned long ZipReader::EntrySize(int index, int *err)
{
	struct zip_stat info;
	int result = zip_stat_index(zip, index, 0, &info);
	if (result != 0) {
		cerr << "could not stat element "<<index<<endl;
		if (err) *err = -1;
		return 0;
	}

	return info.size;
}


unsigned long ZipReader::EntrySize(const char *fname, int *err)
{
	struct zip_stat info;
	int result = zip_stat(zip, fname, 0, &info);
	if (result != 0) {
		cerr << "could not stat "<<fname<<endl;
		if (err) *err = -1;
		return 0;
	}

	return info.size;
}


/*! If buffer == nullptr, then create and return a new char[].
 * Otherwise fill buffer up to buffer_len.
 */
unsigned long ZipReader::EntryContents(int index, char *buffer, unsigned long buffer_len, int *err)
{
	if (!zip) return 0;
	if (!buffer || (buffer && buffer_len == 0)) return 0;

	//char *buf = buffer;
	//if (!buf) {
	//	int er = 0;
	//	buffer_len = EntrySize(index, &er);
	//	if (er != 0) {
	//		if (err) *err = er;
	//		return 0;
	//	}
	//	buf = new char[buffer_len];
	//}

	zip_int64_t len = 0;
	zip_file_t *f = zip_fopen_index(zip, index, 0);

	if (!f) {
		if (err) *err = 1;
		return 0;
	} else {
		len = zip_fread(f, buffer, buffer_len);
		zip_fclose(f);
	}
	return len;
}

/*! If buffer == nullptr, then create and return a new char[].
 * Otherwise fill buffer up to buffer_len.
 *
 * Return the number of bytes read into buffer.
 */
unsigned long ZipReader::EntryContents(const char *fname, char *buffer, unsigned long buffer_len, int *err)
{
	if (!zip) return 0;
	if (!buffer || (buffer && buffer_len == 0)) return 0;

	//char *buf = buffer;
	//if (!buf) {
	//	int er = 0;
	//	buffer_len = EntrySize(fname, &er);
	//	if (er != 0) {
	//		if (err) *err = er;
	//		return 0;
	//	}
	//	buf = new char[buffer_len];
	//}

	zip_int64_t len = 0;
	zip_file_t *f = zip_fopen(zip, fname, 0);

	if (!f) {
		if (err) *err = 1;
		return 0;
	} else {
		len = zip_fread(f, buffer, buffer_len);
		zip_fclose(f);
	}
	return len;
}


//----------------------------------- ZipWriter ----------------------------

/*! \class ZipWriter
 * C++ wrapper for libzip, specifically for writing zip files.
 */


ZipWriter::ZipWriter()
{
}


ZipWriter::~ZipWriter()
{
	if (zip) {
		zip_close(zip);
	}
}


/*! Return true for successful open. else false. */
bool ZipWriter::Open(const char *path, int mode)
{
	if (!path || path[0] == '\0') return false;
	if (zip) { zip_close(zip); zip = nullptr; }

	int err = 0;
	
	int flags = 0;
	if (mode == Truncate) flags = ZIP_TRUNCATE | ZIP_CREATE;
	else if (mode == Append) flags = ZIP_CREATE;

	zip = zip_open(path, flags, &err);
	if (zip == NULL) {
	    zip_error_t error;
	    zip_error_init_with_code(&error, err);
		DBG cerr << "cannot open zip archive " << path <<": "<< zip_error_strerror(&error) << endl;
	    zip_error_fini(&error);
	    return false;
}
	return true;
}


bool ZipWriter::Close()
{
	if (!zip) return false;
	zip_close(zip);
	zip = nullptr;
	return true;
}


bool ZipWriter::WriteFile(const char *file, const char *buffer, long buffer_len)
{
	if (!zip) return false;

	zip_source_t *source = zip_source_buffer(zip, buffer, buffer_len, 0);

	int new_index = zip_file_add(zip, file, source, ZIP_FL_OVERWRITE);
	if (new_index < 0) return false;
	return true;
}


} // namespace Laxkit

