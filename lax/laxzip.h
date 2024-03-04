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


#include <lax/utf8string.h>

#include <zip.h>


namespace Laxkit {


class ZipReader
{
  protected:
	zip_t *zip = nullptr;

  public:
	ZipReader();
	virtual ~ZipReader();

	virtual bool Open(const char *path);
	//virtual bool OpenBlob(const char *buffer, unsigned long buffer_len);
	virtual bool Close();

	virtual int NumEntries();
	//virtual int FindEntry(const char *fname);
	virtual Utf8String EntryName(int index);
	virtual unsigned long EntrySize(int index, int *err = nullptr);
	virtual unsigned long EntrySize(const char *fname, int *err = nullptr);
	virtual unsigned long EntryContents(int index, char *buffer = nullptr, unsigned long buffer_len = 0, int *err = nullptr);
	virtual unsigned long EntryContents(const char *fname, char *buffer = nullptr, unsigned long buffer_len = 0, int *err = nullptr);

};

class ZipWriter
{
  protected:
	zip_t *zip = nullptr;

  public:
	ZipWriter();
	virtual ~ZipWriter();

	enum ZipMode {
		Truncate,
		Append
	};

	virtual bool Open(const char *path, int mode);
	virtual bool Close();

	virtual bool WriteFile(const char *file, const char *buffer, long buffer_len);
};

} // namespace Laxkit

