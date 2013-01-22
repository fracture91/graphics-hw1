hw1: hw1.cpp vshader1.glsl fshader1.glsl Angel.h CheckError.h\
		mat.h vec.h textfile.h textfile.cpp InitShader.cpp GRSReader.cpp
	g++ hw1.cpp -Wall -lglut -lGL -lGLEW -o hw1

clean:
	rm hw1

