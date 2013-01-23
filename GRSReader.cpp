#include <sstream>
#include <stdexcept>
#include "Angel.h"
#include "textfile.cpp"

using std::string;
using std::cout;
using std::endl;
using std::getline;
using std::stringstream;

// thrown if the reader chokes and dies
struct ReaderException : public std::runtime_error {
	string reason;
	public:
		ReaderException(string _reason) : std::runtime_error(_reason) {
			reason = _reason;
		}
		const char* what() const throw() {
			return reason.c_str();
		}
		~ReaderException() throw() {
		}
};

struct GRSExtents {
	float left, top, right, bottom;
};

struct GRSLine {
	unsigned numPoints;
	vec2* points;
};

struct GRSInfo {
	GRSExtents extents;
	unsigned numLines;
	GRSLine* lines;
};

class GRSReader {
	private:
		string content;
		string line;
		GRSInfo info;

	public:
		GRSReader(char* filename) {
			content = string(textFileRead(filename));
		}

		// fill in the given GRSInfo object with the information in the file
		void read(GRSInfo _info) {
			info = _info;
			// find the first line starting with an asterisk
			// this indicates end of comment block
			size_t firstStar = content.find("\n*");
			if(firstStar != string::npos) {
				// pull out comment and print it, just because
				cout << "Comment:" << endl << content.substr(0, firstStar) << endl;
				size_t dataStart = content.find("\n", firstStar + 2);
				if(dataStart != string::npos) {
					content = content.substr(dataStart + 1, string::npos);
				} else {
					content = string();
				}
			} else {
				cout << "No comment" << endl;
			}

			if(content.size() == 0) {
				throw ReaderException("No GRS data");
			}

			stringstream stream(content, stringstream::in);
			unsigned lineno = 0;
			while(getline(stream, line)) {
				switch(lineno) {
					case 0:
						parseExtents(); break;
					case 1:
						parseNumLines(); break;
					default:
						// check if line length or point
						break;
				}
				lineno++;
			}
		}

	private:
		void parseExtents() {
			GRSExtents* x = &info.extents;
			int result = sscanf(line.c_str(), "%f %f %f %f",
					&x->left, &x->top, &x->right, &x->bottom);
			if(result != 4) {
				throw ReaderException("Error parsing extents");
			}
		}
		void parseNumLines() {
			int result = sscanf(line.c_str(), "%d", &info.numLines);
			if(result != 1) {
				throw ReaderException("Error parsing number of lines");
			}
			info.lines = new GRSLine[info.numLines];
		}
};
