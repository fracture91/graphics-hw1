#include "Angel.h"
#include "textfile.cpp"

using std::string;
using std::cout;
using std::endl;

struct GRSInfo {
	struct {
		float left, top, right, bottom;
	} extents;
	unsigned numLines;
	struct line {
		unsigned numPoints;
		vec2 points[];
	} lines[];
};

class GRSReader {
	private:
		string file;
	public:
		GRSReader(char* filename) {
			file = string(textFileRead(filename));
		}

		// fill in the given GRSInfo object with the information in the file
		void read(GRSInfo info) {
			// find the first line starting with an asterisk
			// this indicates end of comment block
			size_t firstStar = file.find("\n*");
			if(firstStar != string::npos) {
				// pull out comment and print it, just because
				cout << "Comment:" << endl << file.substr(0, firstStar) << endl;
				size_t dataStart = file.find("\n", firstStar + 2);
				if(dataStart != string::npos) {
					file = file.substr(dataStart + 1, string::npos);
				} else {
					file = string();
				}
			} else {
				cout << "No comment" << endl;
			}

			if(file.size() == 0) {
				throw "No data";
			}
		}
};
