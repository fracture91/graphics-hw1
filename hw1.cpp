// Starter program for HW 0. 
// Program draws a triangle. Study the program first
// Then modify the function generateGeometry to draw a two-Dimensional Sierpinski Gasket       
// Generated using randomly selected vertices and bisection

// Angel.h is homegrown include file that also includes glew and freeglut

#include "Angel.h"
#include "GRSReader.cpp"

//----------------------------------------------------------------------------

// remember to prototype
void generateGeometry( void );
void initGPUBuffers(vec2* points, int numPoints);
void shaderSetup( void );
void display( void );
void keyboard( unsigned char key, int x, int y );

GLuint program;
GRSInfo* fileInfos; // describes all GRS files
unsigned numFiles;
mat4 ortho;


void initGPUBuffers(vec2* points, int numPoints) {
	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	GLsizeiptr size = sizeof(points)*numPoints;
	glBufferData(GL_ARRAY_BUFFER, size, points, GL_STATIC_DRAW);
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
	
	GRSInfo info = fileInfos[0];
	
	// make the drawing scale to fit the viewport
	ortho = info.extents.ortho;
	GLuint projloc = glGetUniformLocation(program, "Proj");
	glUniformMatrix4fv(projloc, 1, GL_TRUE, ortho);
	// It took me about 5 hours of debugging to figure out that 3rd argument
	// should be GL_TRUE instead of GL_FALSE.  True story.

	// iterate through all lines and draw each one by drawing the
	// appropriate subset of points on the GPU
	unsigned pointIndex = 0;
	GRSViewport* vp = &info.viewport;
	glViewport(vp->x, vp->y, vp->width, vp->height);
	for(unsigned i = 0; i < info.numLines; i++) {
		unsigned len = info.lines[i].numPoints;
		glDrawArrays(GL_LINE_STRIP, pointIndex, len);
		pointIndex += len;
	}
	glFlush(); // force output to graphics hardware
}

//----------------------------------------------------------------------------

// keyboard handler
	void
keyboard( unsigned char key, int x, int y )
{
	switch ( key ) {
		case 033:
			exit( EXIT_SUCCESS );
			break;
	}
}

void reshape(int screenWidth, int screenHeight) {
	GRSInfo* info = &fileInfos[0];
	
	// take up the whole screen
	GRSViewport* wi = &info->within;
	wi->x = 0;
	wi->y = 0;
	wi->width = screenWidth;
	wi->height = screenHeight;

	// viewport is entire "within" bounds by default
	GRSViewport* vp = &info->viewport;
	vp->x = wi->x;
	vp->y = wi->y;
	vp->width = wi->width;
	vp->height = wi->height;
	
	// adjust viewport to maintain aspect ratio in "within" bounds
	GRSExtents* ex = &info->extents;
	float targetRatio = (float)wi->width / (float)wi->height;
	float sourceRatio = (ex->right - ex->left)
		/ (ex->top - ex->bottom);
	if(sourceRatio > targetRatio) {
		vp->height = wi->width / sourceRatio;
	} else if (sourceRatio < targetRatio) {
		vp->width = wi->height * sourceRatio;
	}
}

// copy all points from infos into given array
void getAllPoints(GRSInfo* infos, unsigned infosLen, vec2* points_out) {
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

//----------------------------------------------------------------------------
// entry point
int main(int argc, char **argv) {

	const char* filenames[] = {"drawings/birdhead.dat", "drawings/dino.dat",
		"drawings/dragon.dat", "drawings/house.dat", "drawings/knight.dat",
		"drawings/rex.dat", "drawings/scene.dat", "drawings/usa.dat",
		"drawings/vinci.dat"};
	numFiles = sizeof(filenames)/sizeof(filenames[0]);
	fileInfos = new GRSInfo[numFiles];

	cout << "Reading files..." << endl;
	for(unsigned i = 0; i < numFiles; i++)  {
		GRSReader reader = GRSReader(filenames[i]);
		reader.read(&fileInfos[i]);
	}
	cout << "Done reading files." << endl;

	// need to copy all points into a single array to be sent to GPU
	int numPoints = getNumPoints(fileInfos, numFiles);
	vec2* points = new vec2[numPoints];
	getAllPoints(fileInfos, numFiles, points);

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

	initGPUBuffers(points, numPoints);
	shaderSetup();

	// assign handlers
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	// should add menus
	// add mouse handler
	// add resize window functionality (should probably try to preserve aspect ratio)

	// enter the drawing loop
	// frame rate can be controlled with 
	glutMainLoop();
	return 0;
}
