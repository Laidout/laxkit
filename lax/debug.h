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
//    Copyright (C) 2023-present by AUTHORS
//

#include <iostream>


namespace Laxkit
{

class Debug
{
  	static std::ostream *output;

  public:
  	//static bool output_to_file = false;
  	//static const char *output_file;
  	//static std::string filename;
  	
  	static std::string warning_tag;
  	static std::string error_tag;
  	static std::string normal_tag;

	static bool enabled;
  	static bool include_file;
  	static bool include_line;

  	static std::ostream &Stream() { return *output; }

  	static void Log       (std::string msg, const char *file, int line);
  	static void LogWarning(std::string msg, const char *file, int line);
  	static void LogError  (std::string msg, const char *file, int line);

	static void NormalTag()  { (*output) << normal_tag << std::endl; }
	static void WarningTag() { (*output) << warning_tag;             }
	static void ErrorTag()   { (*output) << error_tag;               }
	static void LineHeader(const char *file, int line, int what);

	static void SetStream(std::ostream *stream);
};

#ifdef LAX_NO_DEBUG
#define DBG
#define DBG_(x)
#define DBGL(x)
#define DBGM(x)
#define DBGW(x)
#define DBGE(x)
#else
#define DBG
#define DBG_(x) x
#define DBGL(x) cerr << x << std::endl;
#define DBGM(x) Laxkit::Debug::LineHeader(__FILE__, __LINE__, 0); Laxkit::Debug::Stream() << x << std::endl;
#define DBGW(x) Laxkit::Debug::LineHeader(__FILE__, __LINE__, 1); Laxkit::Debug::Stream() << x; Laxkit::Debug::NormalTag();
#define DBGE(x) Laxkit::Debug::LineHeader(__FILE__, __LINE__, 2); Laxkit::Debug::Stream() << x; Laxkit::Debug::NormalTag();
#endif


} // namespace Laxkit

