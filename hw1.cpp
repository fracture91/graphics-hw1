// Starter program for HW 0. 
// Program draws a triangle. Study the program first
// Then modify the function generateGeometry to draw a two-Dimensional Sierpinski Gasket       
// Generated using randomly selected vertices and bisection

// Angel.h is homegrown include file that also includes glew and freeglut

#include "Angel.h"
#include "GRSReader.cpp"
#include <vector>
#include <time.h>

using std::vector;

//----------------------------------------------------------------------------

// remember to prototype
void generateGeometry( void );
void initGPUBuffers(vec2* points, int numPoints);
void shaderSetup( void );
void display( void );
void keyboard( unsigned char key, int x, int y );

struct Viewport {
	GLint x, y;
	GLsizei width, height;
};

// a canvas holds data to draw, and a location to draw it within
struct Canvas {
	// where this should be painted on the screen
	// aspect ratio adjustments will likely make the drawing smaller than this
	Viewport location;
	GRSInfo* data;
};

GLuint program;
GRSInfo* fileInfos; // describes all GRS files
unsigned numFiles;
mat4 ortho;
// every canvas in here will be drawn, corrected to preserve aspect ratio
vector<Canvas> canvases;
int numToolbarItems;
GRSInfo drawingInfo; // holds info for the free draw mode


void bufferPoints(vec2* points, int numPoints) {
	GLsizeiptr size = sizeof(points[0])*numPoints;
	glBufferData(GL_ARRAY_BUFFER, size, points, GL_STATIC_DRAW);
}

// copy all points from infos into given array
void copyAllPoints(GRSInfo* infos, unsigned infosLen, vec2* points_out) {
	unsigned outIndex = 0;
	for(unsigned i = 0; i < infosLen; i++) {
		GRSInfo info = infos[i];
		for(unsigned j = 0; j < info.numLines; j++) {
			GRSLine line = info.lines[j];
			for(unsigned k = 0; k < line.numPoints; k++) {
				points_out[outIndex] = line.points[k];
				outIndex++;
			}
		}
	}
}

void bufferAllPoints() {
	// need to copy all points into a single array to be sent to GPU
	int fileNumPoints = getNumPoints(fileInfos, numFiles);
	int totalNumPoints = fileNumPoints + getNumPoints(&drawingInfo, 1);
	vec2* points = new vec2[totalNumPoints];
	copyAllPoints(fileInfos, numFiles, points);
	copyAllPoints(&drawingInfo, 1, points + fileNumPoints);

	bufferPoints(points, totalNumPoints);
	delete points; // points are on GPU, delete the copy
}

void initGPUBuffers() {
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
}


void shaderSetup( void )
{
	// Load shaders and use the resulting shader program
	program = InitShader( "vshader1.glsl", "fshader1.glsl" );
	glUseProgram( program );

	// Initialize the vertex position attribute from the vertex shader
	GLuint loc = glGetAttribLocation( program, "vPosition" );
	glEnableVertexAttribArray( loc );
	glVertexAttribPointer( loc, 2, GL_FLOAT, GL_FALSE, 0,
			BUFFER_OFFSET(0) );

	// sets the default color to clear screen
	glClearColor( 1.0, 1.0, 1.0, 1.0 ); // white background

}

// draw the data within the canvas' location, correcting for aspect ratio
// if the canvas has no data, nothing happens
void drawCanvas(Canvas* canvas) {
	if(canvas->data == NULL) {
		return;
	}

	// make the drawing scale to fit the viewport
	ortho = canvas->data->extents.ortho;
	GLuint projloc = glGetUniformLocation(program, "Proj");
	glUniformMatrix4fv(projloc, 1, GL_TRUE, ortho);
	// It took me about 5 hours of debugging to figure out that 3rd argument
	// should be GL_TRUE instead of GL_FALSE.  True story.

	GRSInfo* info = canvas->data;
	GRSExtents ex = info->extents;
	Viewport loc = canvas->location;
	Viewport final = loc; // final is entire location by default

	// adjust viewport to maintain aspect ratio within loc bounds
	float targetRatio = (float)loc.width / (float)loc.height;
	float sourceRatio = (ex.right - ex.left) / (ex.top - ex.bottom);
	if(sourceRatio > targetRatio) {
		final.height = loc.width / sourceRatio;
	} else if (sourceRatio < targetRatio) {
		final.width = loc.height * sourceRatio;
	}

	glViewport(final.x, final.y, final.width, final.height);

	// iterate through all lines and draw each one by drawing the
	// appropriate subset of points on the GPU
	unsigned lastLineIndex = canvas->data->bufferIndex;
	for(unsigned i = 0; i < canvas->data->numLines; i++) {
		unsigned len = canvas->data->lines[i].numPoints;
		glDrawArrays(GL_LINE_STRIP, lastLineIndex, len);
		lastLineIndex += len;
	}
}


//----------------------------------------------------------------------------
// this is where the drawing should happen
void display(void) {
	// TIP 1: remember to enable depth buffering when drawing in 3d

	// TIP 2: If you want to be sure your math functions are right, 
	// prototype your operations in Matlab or Mathematica first to verify correct behavior
	// both programs support some sort of runtime interface to C++ programs

	// TIP3: check your graphics specs. you have a limited number of loop iterations, storage, registers, texture space etc.


	// TIP4: avoid using glTranslatex, glRotatex, push and pop
	// pass your own view matrix to the shader directly
	// refer to the latest OpenGL documentation for implementation details

	glClear(GL_COLOR_BUFFER_BIT);     // clear the window

	// draw every canvas
	for(vector<Canvas>::iterator c = canvases.begin(); c != canvases.end(); ++c) {
		drawCanvas(&*c); // *c - current iterator val, & to get pointer to canvas
	}
	
	glFlush(); // force output to graphics hardware
}

//----------------------------------------------------------------------------

void displayRandomFile(Canvas* canvas) {
	// set canvas data to a random file
	int randomIndex = rand() % numFiles;
	canvas->data = &fileInfos[randomIndex];
}

const int TILE_ROWS = 6, TILE_COLUMNS = 6;

void clearRandomTiles() {
	for(vector<Canvas>::iterator c = canvases.begin(); c != canvases.end(); ++c) {
		Canvas* canvas = &*c;
		int index = c - canvases.begin();
		if(index > numToolbarItems) { // tile canvases are after toolbar and main canvas
			canvas->data = NULL;
		}
	}
}

// show 6x6 random tiles in the main area
void displayRandomTiles() {
	for(vector<Canvas>::iterator c = canvases.begin(); c != canvases.end(); ++c) {
		Canvas* canvas = &*c;
		int index = c - canvases.begin();
		if(index > numToolbarItems) { // tile canvases are after toolbar and main canvas
			displayRandomFile(canvas);
		}
	}
}

Canvas* getMainCanvas() {
	return &canvases.at(numToolbarItems);
}

void clearDrawingInfo() {
	GRSExtents* ex = &drawingInfo.extents;
	Viewport vp = getMainCanvas()->location;

	// make our drawing extents the same as the main canvas size
	ex->left = vp.x;
	ex->right = vp.x + vp.width;
	ex->bottom = vp.y;
	ex->top = vp.y + vp.height;
	ex->ortho = Ortho2D(ex->left, ex->right, ex->bottom, ex->top);

	// free memory for any existing lines and their points
	for(unsigned i = 0; i < drawingInfo.numLines; i++) {
		delete drawingInfo.lines[i].points;
	}
	delete drawingInfo.lines;
	
	// draw default points to indicate drawing bounds
	// (since aspect ratio preservation may make it smaller than canvas)
	drawingInfo.numLines = 1;
	drawingInfo.lines = new GRSLine[1];
	drawingInfo.lines[0].numPoints = 5;
	drawingInfo.lines[0].points = new vec2[5];
	vec2* points = drawingInfo.lines[0].points;
	points[0] = vec2(ex->left + 1, ex->bottom + 1);
	points[1] = vec2(ex->left + 1, ex->top - 1);
	points[2] = vec2(ex->right - 1, ex->top - 1);
	points[3] = vec2(ex->right - 1, ex->bottom + 1);
	points[4] = points[0];

	bufferAllPoints();
}

enum ProgramState {P, T, E};
ProgramState progState = P;

void changeState(ProgramState state) {
	ProgramState origState = progState;
	progState = state;

	// cleanup for states that need it
	if(origState != progState) {
		switch(origState) {
			case P:
				getMainCanvas()->data = NULL;
				break;
			case T:
				clearRandomTiles();
				break;
			case E:
				getMainCanvas()->data = NULL;
				break;
		}
	}

	switch (state) {
		case P:
			displayRandomFile(getMainCanvas());
			break;
		case T:
			displayRandomTiles();
			break;
		case E:
			clearDrawingInfo();
			getMainCanvas()->data = &drawingInfo;
			break;
	}

}

// keyboard handler
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
		case 27: // ESC
			exit( EXIT_SUCCESS );
			break;
		case 112: // P
			changeState(P);
			break;
		case 116: // T
			changeState(T);
			break;
		case 101: // E
			changeState(E);
			break;
	}
	display(); // doesn't display automatically, need to call this
}

bool coordWithinViewport(int x, int y, Viewport* loc) {
	if(  x > loc->x && x < (loc->x + loc->width)
	  && y > loc->y && y < (loc->y + loc->height)) {
		return true;
	}
	return false;
}

int g_screenHeight = 0;

void mouse(int button, int state, int x, int y) {
	y = g_screenHeight - y; // unfuck y coordinate

	// see if user clicked on a toolbar item
	for(vector<Canvas>::iterator c = canvases.begin(); c != canvases.end(); ++c) {
		Canvas* canvas = &*c;
		Viewport* loc = &canvas->location;
		int index = c - canvases.begin();
		if(index < numToolbarItems) {
			if(coordWithinViewport(x, y, loc)) {
				changeState(P);
				getMainCanvas()->data = canvas->data;
				break;
			}
		}
	}
	display();
}

void reshape(int screenWidth, int screenHeight) {
	
	const int toolbarHeight = 100;
	int lastBoxEnd = 0;
	g_screenHeight = screenHeight;

	// recalculate the position of each canvas
	for(vector<Canvas>::iterator c = canvases.begin(); c != canvases.end(); ++c) {
		Canvas* canvas = &*c;
		Viewport* loc = &canvas->location;
		int index = c - canvases.begin();
		if(index < numToolbarItems) { //toolbar canvases
			// split the toolbar so every item has equal width, same height
			loc->x = lastBoxEnd;
			loc->y = screenHeight - toolbarHeight;
			loc->width = (float)screenWidth / numToolbarItems;
			loc->height = toolbarHeight;
			lastBoxEnd += loc->width;
		} else if(canvas == getMainCanvas()) { // main canvas right after toolbar items
			loc->x = loc->y = 0;
			loc->width = screenWidth;
			int height = screenHeight - toolbarHeight;
			loc->height = height < 0 ? 0 : height;
		} else { // random tiles
			loc->width = screenWidth / TILE_COLUMNS;
			loc->height = (screenHeight - toolbarHeight) / TILE_ROWS;
			index -= numToolbarItems + 1;
			int row = index / TILE_COLUMNS;
			int column = index % TILE_COLUMNS;
			loc->x = column * loc->width;
			loc->y = row * loc->height;
		}
	}
}



//----------------------------------------------------------------------------
// entry point
int main(int argc, char **argv) {

	const char* filenames[] = {"drawings/birdhead.dat", "drawings/dino.dat",
		"drawings/dragon.dat", "drawings/house.dat", "drawings/knight.dat",
		"drawings/rex.dat", "drawings/scene.dat", "drawings/usa.dat",
		"drawings/vinci.dat"};
	numFiles = sizeof(filenames)/sizeof(filenames[0]);
	numToolbarItems = numFiles + 1;
	fileInfos = new GRSInfo[numFiles];

	cout << "Reading files..." << endl;
	for(unsigned i = 0; i < numFiles; i++)  {
		GRSReader reader = GRSReader(filenames[i]);
		reader.read(&fileInfos[i]);
	}
	cout << "Done reading files." << endl;

	// set up toolbar canvases
	for(unsigned i = 0; i < numFiles + 1; i++) {
		Canvas c;
		if(i == numFiles) {
			c.data = &fileInfos[i - 1]; // last drawing appears twice
		} else {
			c.data = &fileInfos[i];
		}
		// location is handled by reshape
		canvases.push_back(c);
	}

	// add the main canvas
	Canvas c;
	c.data = NULL;
	canvases.push_back(c);

	srand(time(NULL));
	displayRandomFile(getMainCanvas());

	// set up our random tile canvases
	for(int x = 0; x < TILE_COLUMNS; x++) {
		for(int y = 0; y < TILE_ROWS; y++) {
			Canvas c;
			canvases.push_back(c);
			// reshape handles the sizing and position
		}
	}
	clearRandomTiles();

	// init glut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
	glutInitWindowSize(640, 480);

	// If you are using freeglut, the next two lines will check if 
	// the code is truly 3.2. Otherwise, comment them out

	//glutInitContextVersion( 3, 3 );
	//glutInitContextProfile( GLUT_CORE_PROFILE );

	// create GLUT window for drawing
	glutCreateWindow("PoliBook");

	// init glew
	glewInit();

	initGPUBuffers();
	
	// set up bufferIndex
	unsigned lastIndex = 0;
	for(unsigned i = 0; i < numFiles; i++) {
		fileInfos[i].bufferIndex = lastIndex;
		lastIndex += getNumPoints(fileInfos[i]);
	}

	drawingInfo.name = "drawing";
	drawingInfo.bufferIndex = lastIndex; // all drawing points at end of buffer
	
	bufferAllPoints();

	shaderSetup();

	// assign handlers
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	// should add menus
	// add mouse handler
	// add resize window functionality (should probably try to preserve aspect ratio)

	// enter the drawing loop
	// frame rate can be controlled with 
	glutMainLoop();
	return 0;
}
