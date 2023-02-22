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

#include <lax/debug.h>
#include <fstream>

using namespace std;


namespace Laxkit {

// Here, \033 is the ESC character, ASCII 27. It is followed by [, then zero or more numbers separated by ;, 
// and finally the letter m. The numbers describe the colour and format to switch to from that point onwards.
// 
// The codes for foreground and background colours are:
// 
//          foreground background
// black        30         40
// red          31         41
// green        32         42
// yellow       33         43
// blue         34         44
// magenta      35         45
// cyan         36         46
// white        37         47
// 
// Additionally, you can use these:
// 
// reset             0  (everything back to normal)
// bold/bright       1  (often a brighter shade of the same colour)
// underline         4
// inverse           7  (swap foreground and background colours)
// bold/bright off  21
// underline off    24
// inverse off      27

const char *output_file = (getenv("LAXKIT_DEBUG_FILE"));
std::ofstream output_stream;

static std::ostream *DefaultStream()
{
	//if (Debug::Stream()) return;
	if (output_file) {
		output_stream.open(output_file, ios_base::out);
		if (output_stream.is_open()) {
			//Debug::SetStream(&output_stream);
			return &output_stream;
		}
	}
	//Debug::SetStream(&cerr);
	return &cerr;
}
ostream *Debug::output = DefaultStream();


bool Debug::enabled = true;
bool Debug::include_file = true;
bool Debug::include_line = true;
std::string Debug::warning_tag = (getenv("LAXKIT_COLOR_DEBUG") == nullptr ? "" : "\033[1;33m");
std::string Debug::error_tag   = (getenv("LAXKIT_COLOR_DEBUG") == nullptr ? "" : "\033[1;31m");
std::string Debug::normal_tag  = (getenv("LAXKIT_COLOR_DEBUG") == nullptr ? "" : "\033[0m"   );



void Debug::SetStream(ostream *stream)
{
	output = stream;
	if (output == nullptr) output = &cerr;
}

void Debug::LineHeader(const char *file, int line, int what)
{
	if (!enabled) return;

	if (what == 1)      Laxkit::Debug::WarningTag();  
	else if (what == 2) Laxkit::Debug::ErrorTag();    
	if (include_file) (*output) << file;
	if (include_line) (*output)	<< (include_file ? " #" : "#") << line;
	if (what == 0) {
		if (include_file || include_line) (*output) << ": ";
	} else (*output) << (what == 1 ? " WARNING: " : " ERROR: ");
}

void Debug::Log(std::string msg, const char *file, int line)
{
	if (!enabled) return;

	if (include_file) (*output) << file;
	if (include_line) (*output)	<< (include_file ? " #" : "#") << line;
	if (include_file || include_line) (*output) << ": ";
	(*output) << msg << endl;
}

void Debug::LogWarning(std::string msg, const char *file, int line)
{
	if (!enabled) return;

	(*output) << warning_tag;
	if (include_file) (*output) << file;
	if (include_line) (*output)	<< (include_file ? " #" : "#") << line;
	//if (include_file || include_line) (*output) << ": ";
	(*output) << "WARNING: " << msg << normal_tag << endl;
}

void Debug::LogError(std::string msg, const char *file, int line)
{
	if (!enabled) return;

	(*output) << error_tag;
	if (include_file) (*output) << file;
	if (include_line) (*output)	<< (include_file ? " #" : "#") << line;
	//if (include_file || include_line) (*output) << ": ";
	(*output) << "ERROR: " << msg << normal_tag << endl;
}

} // namespace Laxkit
