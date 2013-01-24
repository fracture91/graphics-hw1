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
	mat4 ortho;
};

struct GRSLine {
	unsigned numPoints;
	vec2* points;
};

struct GRSInfo {
	const char* name;
	GRSExtents extents;
	unsigned bufferIndex; // same - where data starts in GPU buffer
	unsigned numLines;
	GRSLine* lines;
};

int getNumPoints(GRSInfo& info) {
	int sum = 0;
	for(unsigned i = 0; i < info.numLines; i++) {
		sum += info.lines[i].numPoints;
	}
	return sum;
}

int getNumPoints(GRSInfo* infos, int numInfos) {
	int sum = 0;
	for(int i = 0; i < numInfos; i++) {
		sum += getNumPoints(infos[i]);
	}
	return sum;
}

void print(GRSExtents& ex) {
	cout << ex.left << ", " << ex.top << ", " << ex.right << ", "
		<< ex.bottom << endl;
}

void print(GRSLine& line) {
	cout << line.numPoints << " points:" << endl;
	for(unsigned i = 0; i < line.numPoints; i++) {
		cout << line.points[i].x << ", " << line.points[i].y << endl;
	}
}

void print(GRSInfo& info) {
	print(info.extents);
	cout << "numLines: " << info.numLines << endl;
	for(unsigned i = 0; i < info.numLines; i++) {
		print(info.lines[i]);
	}
}

class GRSReader {
	private:
		const char* filename;
		string content;
		string line;
		GRSInfo* info;
		unsigned linesIndex;
		unsigned pointsIndex;

	public:
		GRSReader(const char* _filename) {
			content = string(textFileRead(_filename));
			filename = _filename;
		}

		// fill in the given GRSInfo object with the information in the file
		void read(GRSInfo* _info) {
			info = _info;
			info->name = filename;
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
			int lineno = -1;
			linesIndex = -1;
			pointsIndex = 0;
			while(getline(stream, line)) {
				lineno++;
				char junk[2]; // one for null
				// ignore lines that are whitespace only
				if(sscanf(line.c_str(), "%1s", junk) != 1) {
					lineno--;
					continue;
				}
				switch(lineno) {
					case 0:
						parseExtents(); break;
					case 1:
						parseNumLines(); break;
					default:
						parseLengthOrPoint(); break;
				}
			}
		}

	private:
		void parseExtents() {
			GRSExtents* x = &info->extents;
			int result = sscanf(line.c_str(), "%f %f %f %f",
					&x->left, &x->top, &x->right, &x->bottom);
			if(result != 4) {
				throw ReaderException("Error parsing extents");
			}
			x->ortho = Ortho2D(x->left, x->right, x->bottom, x->top);
		}
		void parseNumLines() {
			int result = sscanf(line.c_str(), "%d", &info->numLines);
			if(result != 1) {
				throw ReaderException("Error parsing number of lines");
			}
			info->lines = new GRSLine[info->numLines];
		}
		void parseLengthOrPoint() {
			float junk1, junk2;
			bool isPoint = sscanf(line.c_str(), "%f %f", &junk1, &junk2) == 2;
			if(isPoint) {
				if(info->lines[linesIndex].numPoints <= pointsIndex) {
					throw ReaderException("More points than expected");
				}
				float x, y;
				if(sscanf(line.c_str(), "%f %f", &x, &y) != 2) {
					throw ReaderException("Error reading point");
				}
				info->lines[linesIndex].points[pointsIndex] = vec2(x, y);
				pointsIndex++;
			} else {
				unsigned numPoints;
				linesIndex++;
				// check if we're ending a previous line...
				if(linesIndex > 0
						&& info->lines[linesIndex - 1].numPoints > pointsIndex) {
					throw ReaderException("Too few points");
				}
				pointsIndex = 0;
				if(sscanf(line.c_str(), "%d", &numPoints) != 1
						&& linesIndex < info->numLines - 1) {
					throw ReaderException("Too few lines or error reading line");
				}
				if(info->numLines <= linesIndex) {
					throw ReaderException("More lines than expected");
				}
				info->lines[linesIndex].numPoints = numPoints;
				info->lines[linesIndex].points = new vec2[numPoints];
			}
		}
};
