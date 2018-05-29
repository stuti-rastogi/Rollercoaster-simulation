/**
	CSCI 420
	Assignment 2: Simulating a Roller Coaster
	Date: 03/22/18
	Name: Stuti Rastogi
	USC ID: 8006687469
	Username: srrastog
*/

// include headers
#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "openGLHeader.h"
#include "glutHeader.h"

#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"

using namespace std;

#ifdef WIN32
	#ifdef _DEBUG
		#pragma comment(lib, "glew32d.lib")
	#else
	    #pragma comment(lib, "glew32.lib")
	#endif
#endif

// directory where the shaders sit
#ifdef WIN32
	char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
	char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

// printing some basic information on the terminal
using namespace std;

// window properties
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework II";

// pipeline program and matrix variables
OpenGLMatrix * matrix;

BasicPipelineProgram * pipelineProgramColor;
GLuint programHandlerColor;

BasicPipelineProgram * pipelineProgramTexture;
GLuint programHandlerTexture;

// variables needed in drawArrays call
GLint first;
GLsizei countVertices;

// variables for setting the perspective projection matrix
GLfloat FoV = 45;
GLfloat aspectRatio;

// the matrices used to fetch projection and model view matrices
float p[16];
float m[16];

// variables needed for mapping shader variables to VBO
GLuint loc;
const void * offset;
GLsizei stride;
GLboolean normalized;

// variables used in idle function to create animation
int screenshotCount = 0;
int stop = 0;

/*********** HW 2 *****************/
// represents one control point along the spline 
struct Point 
{
	double x;
	double y;
	double z;
};

// spline struct 
// contains how many control points the spline has, 
// and an array of control points 
struct Spline 
{
	int numControlPoints;
	Point * points;
};

// the spline array 
Spline * splines;

// total number of splines 
int numSplines;

// Level 1
// for milestone
// GLuint splineVAO;
// GLuint splineVBO;

vector<float> splineVertices;
// vector<float> splineColors;			// for milestone

float s = 0.5;		// tension parameter for Catmull-Rom spline
float basis1, basis2, basis3, basis4;
float x_calc, y_calc, z_calc;	// intermediate position calculations
float x_tan, y_tan, z_tan;		// intermediate tangent calculations

// Level 2
int code;						// to check if texture initialised

GLuint groundTexHandle;
GLuint groundVAO;
GLuint groundVBO;

vector<float> groundVertices;
vector<float> groundTexCoords;

// Level 3
GLuint skyTexHandle;			// same texture for 5 faces of sky box
GLuint skyFrontVAO;
GLuint skyFrontVBO;

vector<float> skyFrontVertices;
vector<float> skyFrontTexCoords;

GLuint skyBackVAO;
GLuint skyBackVBO;

vector<float> skyBackVertices;
vector<float> skyBackTexCoords;

GLuint skyLeftVAO;
GLuint skyLeftVBO;

vector<float> skyLeftVertices;
vector<float> skyLeftTexCoords;

GLuint skyRightVAO;
GLuint skyRightVBO;

vector<float> skyRightVertices;
vector<float> skyRightTexCoords;

GLuint skyTopVAO;
GLuint skyTopVBO;

vector<float> skyTopVertices;
vector<float> skyTopTexCoords;

// Level 4
// store the Frenet frame for each point
vector<Point> normals;
vector<Point> tangents;
vector<Point> binormals;

// store the spline points as a point array to make it easier to iterate over
// in order to create cross section points
vector<Point> splinePoints;			

// intermediate calculation result
Point tangent, normal, binormal;

// parameters for the LookAt function
float eyeX, eyeY, eyeZ;
float centerX, centerY, centerZ;
float upX, upY, upZ;

// index variables that are used to iterate over arrays
int pointIndex = 0;				// pointIndex iterates over splineVertices
int FernetIndex = 0;			// FernetIndex iterates over normals, tangents, 
								// binormals

// Level 5
// stores all the 4 points per spline point generated for cross section
vector<Point> crossSectionPoints;
// the 4 directions generated using the normal and binormal at a point
Point dir0, dir1, dir2, dir3;
float alpha, beta;			// scalars for cross section and rails respectively
Point v0, v1, v2, v3;			// cross section points
Point a00, a01, a02, a03;		// 4 points for one rail
Point a10, a11, a12, a13;		// 4 points for the other rail

// Extra Credit
// vectors to texture map the track plane
vector<float> crossSectionVertices;
vector<float> crossSectionTexCoords;

GLuint crossSectionTexHandle;
GLuint crossSectionVAO;
GLuint crossSectionVBO;

// fetch intermediate point and add to appropriate VBO vector
Point v;

// Extra Credit
// Double Rail - additional 4 points per bottom point of each rail
vector<Point> trackPoints;

// texture map the 4 faces of each rail
vector<float> trackVertices;
vector<float> trackTexCoords;

GLuint trackTexHandle;
GLuint trackVAO;
GLuint trackVBO;

// Given method - loads the splines from the input files to the array of
// Spline objects
int loadSplines(char * argv) 
{
	char * cName = (char *) malloc(128 * sizeof(char));
	FILE * fileList;
	FILE * fileSpline;
	int iType, i = 0, j, iLength;

	// load the track file 
	fileList = fopen(argv, "r");
	if (fileList == NULL) 
	{
		printf ("can't open file\n");
		exit(1);
	}
  
	// stores the number of splines in a global variable 
	fscanf(fileList, "%d", &numSplines);

	splines = (Spline*) malloc(numSplines * sizeof(Spline));

	// reads through the spline files 
	for (j = 0; j < numSplines; j++) 
	{
		i = 0;
		fscanf(fileList, "%s", cName);
		fileSpline = fopen(cName, "r");

		if (fileSpline == NULL) 
		{
			printf ("can't open file\n");
			exit(1);
		}

		// gets length for spline file
		fscanf(fileSpline, "%d %d", &iLength, &iType);

		// allocate memory for all the points
		splines[j].points = (Point *)malloc(iLength * sizeof(Point));
		splines[j].numControlPoints = iLength;

		// saves the data to the struct
		while (fscanf(fileSpline, "%lf %lf %lf", 
				&splines[j].points[i].x, 
				&splines[j].points[i].y, 
				&splines[j].points[i].z) != EOF) 
		{
	  		i++;
		}
	}

	free(cName);

	return 0;
}

// NOTE: Point struct also used for vector3

// Helper function to calculate the length of a vector
float length(Point p)
{
	return sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}

// Helper function that calculate the cross product a x b
Point crossProduct(Point a, Point b)
{
	Point result;
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
	return result;
}

// Helper function to add 2 vectors
Point add(Point a, Point b)
{
	Point result;
	result.x = a.x + b.x;
	result.y = a.y + b.y;
	result.z = a.z + b.z;
	return result;
}

// Helper function to multiply a vector3 with a scalar
Point multiplyWithScalar(float scalar, Point a)
{
	Point result;
	result.x = scalar * a.x;
	result.y = scalar * a.y;
	result.z = scalar * a.z;
	return result;
}

// Function that calculates the normal, tangent and binormal (Frenet frame) at
// a given point. The 4 control points, and u of current point are inputs
// and the computed normal, tangent and binormal are stored in the appropriate
// vectors.
void computeFrenet(float u, Point p1, Point p2, Point p3, Point p4)
{	
	// derivation of Catmull-Rom basis functions
	basis1 = (-3 * s * pow(u, 2)) + (4 * s * u) - s;
	basis2 = (3 * (2 - s) * pow(u, 2)) + (2 * (s - 3) * u);
	basis3 = (3 * (s - 2) * pow(u, 2)) + (2 * (3 - 2*s) * u) + s;
	basis4 = (3 * s * pow(u, 2)) - (2 * s * u);

	x_tan = (basis1 * p1.x) + (basis2 * p2.x) + 
			(basis3 * p3.x) + (basis4 * p4.x);
	y_tan = (basis1 * p1.y) + (basis2 * p2.y) + 
			(basis3 * p3.y) + (basis4 * p4.y);
	z_tan = (basis1 * p1.z) + (basis2 * p2.z) + 
			(basis3 * p3.z) + (basis4 * p4.z);

	// tangent vector - un-normalised
	Point t = {x_tan, y_tan, z_tan};
	float magnitude = length(t);
	// normalisation
	tangent.x = x_tan / magnitude;
	tangent.y = y_tan / magnitude;
	tangent.z = z_tan / magnitude;

	// for first point, use v
	if (normals.empty())
	{
		//pick unit V
		Point v = {1.0/sqrt(3), 1.0/sqrt(3), 1.0/sqrt(3)};
		
		Point n0 = crossProduct(tangent, v);
		magnitude = length(n0);
		normal.x = n0.x / magnitude;
		normal.y = n0.y / magnitude;
		normal.z = n0.z / magnitude;
	}
	// for other points use previous binormal
	else
	{
		Point b0 = binormals.back();
		Point n1 = crossProduct(b0, tangent);
		magnitude = length(n1);
		normal.x = n1.x / magnitude;
		normal.y = n1.y / magnitude;
		normal.z = n1.z / magnitude;
	}

	Point b = crossProduct(tangent, normal);
	
	magnitude = length(b);
	binormal.x = b.x / magnitude;
	binormal.y = b.y / magnitude;
	binormal.z = b.z / magnitude;

	tangents.push_back(tangent);
	normals.push_back(normal);
	binormals.push_back(binormal);
}

// Function to create the Catmull Rom Spline
void createPoints()
{
	for (int i = 0; i < numSplines; i++)
	{
		// take 4 points at a time
		for (int j = 0; j <= splines[i].numControlPoints-4; j++)
		{
			for (float u = 0; u <= 1; u = u + 0.01)
			{
				basis1 = (-s * pow(u, 3)) + (2 * s * pow(u,2)) - (s * u);
				basis2 = ((2 - s) * pow(u, 3)) + ((s - 3) * pow(u, 2)) + 1;
				basis3 = ((s - 2) * pow(u, 3)) + ((3 - 2 * s) * pow(u, 2)) + 
							(s * u);
				basis4 = (s * pow(u, 3)) - (s * pow(u, 2));

				// control points
				Point p1 = splines[i].points[j];
				Point p2 = splines[i].points[j+1];
				Point p3 = splines[i].points[j+2];
				Point p4 = splines[i].points[j+3];

				x_calc = (basis1 * p1.x) + (basis2 * p2.x) + 
						(basis3 * p3.x) + (basis4 * p4.x);
				y_calc = (basis1 * p1.y) + (basis2 * p2.y) + 
						(basis3 * p3.y) + (basis4 * p4.y);
				z_calc = (basis1 * p1.z) + (basis2 * p2.z) + 
						(basis3 * p3.z) + (basis4 * p4.z);

				computeFrenet(u, p1, p2, p3, p4);

				Point currentP = {x_calc, y_calc, z_calc};
				splinePoints.push_back(currentP);

				splineVertices.push_back(x_calc);
				splineVertices.push_back(y_calc);
				splineVertices.push_back(z_calc);

				// for milestone
				// splineColors.push_back(1.0);			// r
				// splineColors.push_back(0.0);			// g
				// splineColors.push_back(0.0);			// b
				// splineColors.push_back(1.0);			// a

				if (j == 0 && u == 0)
					continue;

				if (j == splines[i].numControlPoints-4 && u == 1)
					continue;

				// add every vertex twice except first and last - GL_LINES
				splineVertices.push_back(x_calc);
				splineVertices.push_back(y_calc);
				splineVertices.push_back(z_calc);

				//for milestone
				// splineColors.push_back(1.0);			// r
				// splineColors.push_back(0.0);			// g
				// splineColors.push_back(0.0);			// b
				// splineColors.push_back(1.0);			// a
			}
		}
	}
}

// Function that creates the 4 cross sections points per point on spline.
// This function also generates the additional points for the double rails.
// It fills the correct arrays and then accesses those arrays to generate
// appropriate triangles for the VBOs for texture mapping
void createCrossSection()
{
	for (int i = 0; i < splinePoints.size(); i++)
	{
		Point p = splinePoints[i];
		Point normalAtP = normals[i];
		Point binormalAtP = binormals[i];

		alpha = 0.2;			// scalar for cross section
		beta = 0.04;			// scalar for rails

		dir0 = add(binormalAtP, multiplyWithScalar(-1, normalAtP));	//-n + b
		v0 = add(p, multiplyWithScalar(alpha, dir0));

		dir1 = add(binormalAtP, normalAtP);	//-n + b
		v1 = add(p, multiplyWithScalar(alpha, dir1));

		dir2 = add(normalAtP, multiplyWithScalar(-1, binormalAtP));	//-n + b
		v2 = add(p, multiplyWithScalar(alpha, dir2));

		dir3 = add(multiplyWithScalar(-1, binormalAtP), 
								multiplyWithScalar(-1, normalAtP));	//-n + b
		v3 = add(p, multiplyWithScalar(alpha, dir3));

		a00 = add(v2, multiplyWithScalar(beta, dir0));
		a01 = add(v2, multiplyWithScalar(beta, dir1));
		a02 = add(v2, multiplyWithScalar(beta, dir2));
		a03 = add(v2, multiplyWithScalar(beta, dir3));

		a10 = add(v3, multiplyWithScalar(beta, dir0));
		a11 = add(v3, multiplyWithScalar(beta, dir1));
		a12 = add(v3, multiplyWithScalar(beta, dir2));
		a13 = add(v3, multiplyWithScalar(beta, dir3));

		crossSectionPoints.push_back(v0);
		crossSectionPoints.push_back(v1);
		crossSectionPoints.push_back(v2);
		crossSectionPoints.push_back(v3);

		trackPoints.push_back(a00);
		trackPoints.push_back(a01);
		trackPoints.push_back(a02);
		trackPoints.push_back(a03);

		trackPoints.push_back(a10);
		trackPoints.push_back(a11);
		trackPoints.push_back(a12);
		trackPoints.push_back(a13);
	}

	// see Fig. 1 in README.pdf for numbering
	for (int i = 2; i <= crossSectionPoints.size()-10; i = i + 8)
	{
		//2-6-7 one triangle and 2-3-7 second triangle
		//point 2
		v = crossSectionPoints[i];
		crossSectionVertices.push_back(v.x);
		crossSectionVertices.push_back(v.y);
		crossSectionVertices.push_back(v.z);

		crossSectionTexCoords.push_back(1.0);
		crossSectionTexCoords.push_back(0.0);

		//point 6
		v = crossSectionPoints[i+4+4];
		crossSectionVertices.push_back(v.x);
		crossSectionVertices.push_back(v.y);
		crossSectionVertices.push_back(v.z);

		crossSectionTexCoords.push_back(1.0);
		crossSectionTexCoords.push_back(1.0);

		//point 7
		v = crossSectionPoints[i+5+4];
		crossSectionVertices.push_back(v.x);
		crossSectionVertices.push_back(v.y);
		crossSectionVertices.push_back(v.z);

		crossSectionTexCoords.push_back(0.0);
		crossSectionTexCoords.push_back(1.0);

		//point 7
		v = crossSectionPoints[i+5+4];
		crossSectionVertices.push_back(v.x);
		crossSectionVertices.push_back(v.y);
		crossSectionVertices.push_back(v.z);

		crossSectionTexCoords.push_back(0.0);
		crossSectionTexCoords.push_back(1.0);

		//point 2
		v = crossSectionPoints[i];
		crossSectionVertices.push_back(v.x);
		crossSectionVertices.push_back(v.y);
		crossSectionVertices.push_back(v.z);

		crossSectionTexCoords.push_back(1.0);
		crossSectionTexCoords.push_back(0.0);

		//point 3
		v = crossSectionPoints[i+1];
		crossSectionVertices.push_back(v.x);
		crossSectionVertices.push_back(v.y);
		crossSectionVertices.push_back(v.z);

		crossSectionTexCoords.push_back(0.0);
		crossSectionTexCoords.push_back(0.0);
	}

	// See fig 2. in README.pdf for numbering
	for (int i = 0; i <= trackPoints.size()-12; i = i+4)
	{	
		//0-1-9 and 9-0-8
		//point 0
		v = trackPoints[i];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 1
		v = trackPoints[i+1];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//point 9
		v = trackPoints[i+9];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 9
		v = trackPoints[i+9];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 0
		v = trackPoints[i];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 8
		v = trackPoints[i+8];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//1-2-10 and 10-1-9
		//point 1
		v = trackPoints[i+1];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 2
		v = trackPoints[i+2];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//point 10
		v = trackPoints[i+10];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 10
		v = trackPoints[i+10];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 1
		v = trackPoints[i+1];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 9
		v = trackPoints[i+9];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//0-3-11 and 11-0-8
		//point 0
		v = trackPoints[i];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 3
		v = trackPoints[i+3];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//point 11
		v = trackPoints[i+11];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 11
		v = trackPoints[i+11];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 0
		v = trackPoints[i];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 8
		v = trackPoints[i+8];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//3-2-10 and 10-3-11
		//point 3
		v = trackPoints[i+3];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 2
		v = trackPoints[i+2];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);

		//point 10
		v = trackPoints[i+10];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 10
		v = trackPoints[i+10];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(1.0);

		//point 3
		v = trackPoints[i+3];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(0.0);
		trackTexCoords.push_back(0.0);

		//point 11
		v = trackPoints[i+11];
		trackVertices.push_back(v.x);
		trackVertices.push_back(v.y);
		trackVertices.push_back(v.z);

		trackTexCoords.push_back(1.0);
		trackTexCoords.push_back(0.0);
	}
}

// Function that creates the ground plane and fills the VBO array and texture
// coordinates array
void createGround()
{
	// Ground plane from -50 to 50 (x), -50 t0 50 (y), at z = 20
	// vertex 1: (-50, -50, 20)
	// vertex 2: (-50, 50, 20)
	// vertex 3: (50, -50, 20)
	// vertex 4: (50, -50, 20)
	// vertex 5: (50, 50, 20)
	// vertex 6: (-50, 50, 20)

	groundVertices.push_back(-50.0);
	groundVertices.push_back(-50.0);
	groundVertices.push_back(20.0);
	groundTexCoords.push_back(0.0);
	groundTexCoords.push_back(0.0);

	groundVertices.push_back(-50.0);
	groundVertices.push_back(50.0);
	groundVertices.push_back(20.0);
	groundTexCoords.push_back(0.0);
	groundTexCoords.push_back(1.0);

	groundVertices.push_back(50.0);
	groundVertices.push_back(-50.0);
	groundVertices.push_back(20.0);
	groundTexCoords.push_back(1.0);
	groundTexCoords.push_back(0.0);

	groundVertices.push_back(50.0);
	groundVertices.push_back(-50.0);
	groundVertices.push_back(20.0);
	groundTexCoords.push_back(1.0);
	groundTexCoords.push_back(0.0);

	groundVertices.push_back(50.0);
	groundVertices.push_back(50.0);
	groundVertices.push_back(20.0);
	groundTexCoords.push_back(1.0);
	groundTexCoords.push_back(1.0);

	groundVertices.push_back(-50.0);
	groundVertices.push_back(50.0);
	groundVertices.push_back(20.0);
	groundTexCoords.push_back(0.0);
	groundTexCoords.push_back(1.0);
}

// Function that generates the sky box. It generates 5 planes and fills the
// appropriate VBO arrays and texture co-ordinate arrays for each
void createSky()
{	
	//1. back face
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(25.0);
	skyBackTexCoords.push_back(0.0);
	skyBackTexCoords.push_back(0.0);

	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(-200.0);
	skyBackTexCoords.push_back(0.0);
	skyBackTexCoords.push_back(1.0);

	skyBackVertices.push_back(50.0);
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(-200.0);
	skyBackTexCoords.push_back(1.0);
	skyBackTexCoords.push_back(1.0);

	skyBackVertices.push_back(50.0);
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(-200.0);
	skyBackTexCoords.push_back(1.0);
	skyBackTexCoords.push_back(1.0);

	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(25.0);
	skyBackTexCoords.push_back(0.0);
	skyBackTexCoords.push_back(0.0);

	skyBackVertices.push_back(50.0);
	skyBackVertices.push_back(-50.0);
	skyBackVertices.push_back(25.0);
	skyBackTexCoords.push_back(1.0);
	skyBackTexCoords.push_back(0.0);

	//2. front face
	skyFrontVertices.push_back(-50.0);
	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(25.0);
	skyFrontTexCoords.push_back(0.0);
	skyFrontTexCoords.push_back(0.0);

	skyFrontVertices.push_back(-50.0);
	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(-200.0);
	skyFrontTexCoords.push_back(0.0);
	skyFrontTexCoords.push_back(1.0);

	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(-200.0);
	skyFrontTexCoords.push_back(1.0);
	skyFrontTexCoords.push_back(1.0);

	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(-200.0);
	skyFrontTexCoords.push_back(1.0);
	skyFrontTexCoords.push_back(1.0);

	skyFrontVertices.push_back(-50.0);
	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(25.0);
	skyFrontTexCoords.push_back(0.0);
	skyFrontTexCoords.push_back(0.0);

	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(50.0);
	skyFrontVertices.push_back(25.0);
	skyFrontTexCoords.push_back(1.0);
	skyFrontTexCoords.push_back(0.0);

	//3. left face
	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(50.0);
	skyLeftVertices.push_back(25.0);
	skyLeftTexCoords.push_back(0.0);
	skyLeftTexCoords.push_back(0.0);

	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(50.0);
	skyLeftVertices.push_back(-200.0);
	skyLeftTexCoords.push_back(0.0);
	skyLeftTexCoords.push_back(1.0);

	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(-200.0);
	skyLeftTexCoords.push_back(1.0);
	skyLeftTexCoords.push_back(1.0);

	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(-200.0);
	skyLeftTexCoords.push_back(1.0);
	skyLeftTexCoords.push_back(1.0);

	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(50.0);
	skyLeftVertices.push_back(25.0);
	skyLeftTexCoords.push_back(0.0);
	skyLeftTexCoords.push_back(0.0);

	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(-50.0);
	skyLeftVertices.push_back(25.0);
	skyLeftTexCoords.push_back(1.0);
	skyLeftTexCoords.push_back(0.0);

	//4. right face
	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(-50.0);
	skyRightVertices.push_back(25.0);
	skyRightTexCoords.push_back(0.0);
	skyRightTexCoords.push_back(0.0);

	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(-50.0);
	skyRightVertices.push_back(-200.0);
	skyRightTexCoords.push_back(0.0);
	skyRightTexCoords.push_back(1.0);

	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(-200.0);
	skyRightTexCoords.push_back(1.0);
	skyRightTexCoords.push_back(1.0);

	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(-200.0);
	skyRightTexCoords.push_back(1.0);
	skyRightTexCoords.push_back(1.0);

	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(-50.0);
	skyRightVertices.push_back(25.0);
	skyRightTexCoords.push_back(0.0);
	skyRightTexCoords.push_back(0.0);

	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(50.0);
	skyRightVertices.push_back(25.0);
	skyRightTexCoords.push_back(1.0);
	skyRightTexCoords.push_back(0.0);

	//5. top face
	skyTopVertices.push_back(-50.0);
	skyTopVertices.push_back(50.0);
	skyTopVertices.push_back(-200.0);
	skyTopTexCoords.push_back(0.0);
	skyTopTexCoords.push_back(0.0);

	skyTopVertices.push_back(-50.0);
	skyTopVertices.push_back(-50.0);
	skyTopVertices.push_back(-200.0);
	skyTopTexCoords.push_back(0.0);
	skyTopTexCoords.push_back(1.0);

	skyTopVertices.push_back(50.0);
	skyTopVertices.push_back(-50.0);
	skyTopVertices.push_back(-200.0);
	skyTopTexCoords.push_back(1.0);
	skyTopTexCoords.push_back(1.0);

	skyTopVertices.push_back(50.0);
	skyTopVertices.push_back(-50.0);
	skyTopVertices.push_back(-200.0);
	skyTopTexCoords.push_back(1.0);
	skyTopTexCoords.push_back(1.0);

	skyTopVertices.push_back(-50.0);
	skyTopVertices.push_back(50.0);
	skyTopVertices.push_back(-200.0);
	skyTopTexCoords.push_back(0.0);
	skyTopTexCoords.push_back(0.0);

	skyTopVertices.push_back(50.0);
	skyTopVertices.push_back(50.0);
	skyTopVertices.push_back(-200.0);
	skyTopTexCoords.push_back(1.0);
	skyTopTexCoords.push_back(0.0);
}

// slide 12-39
// Given function - to create the appropriate texture unit to use for display
void setTextureUnit(GLint unit)
{
	glActiveTexture(unit); // select the active texture unit
	// get a handle to the “textureImage” shader variable
	GLint h_textureImage = 
				glGetUniformLocation(programHandlerTexture, "textureImage");
	// deem the shader variable “textureImage” to read from texture unit “unit”
	glUniform1i(h_textureImage, unit - GL_TEXTURE0);
} 

// Given function - this method writes a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
	unsigned char * screenshotData = 
							new unsigned char[windowWidth * windowHeight * 3];
	glReadPixels(0, 0, windowWidth, windowHeight, 
					GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

	ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

	// to avoid slowing down FPS, removed the print statement for every
	// screenshot

	// if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
	// 	// cout << "File " << filename << " saved successfully." << endl;
	// else cout << "Failed to save file " << filename << '.' << endl;

	if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
		cout << "Failed to save file " << filename << '.' << endl;
	delete [] screenshotData;
}

// This function accesses the correct point based on the point index and 
// fetches the Frenet frame vectors to set camera LookAt parameters
void calculateCameraLocation()
{
	if (pointIndex == 0)
		FernetIndex = 0;
	else if (pointIndex == 3)
		FernetIndex = 1;
	else
		FernetIndex = (pointIndex / 3) - FernetIndex;
	eyeX = splineVertices[pointIndex];
	eyeY = splineVertices[pointIndex+1];
	eyeZ = splineVertices[pointIndex+2];

	centerX = eyeX + tangents[FernetIndex].x;
	centerY = eyeY + tangents[FernetIndex].y;
	centerZ = eyeZ + tangents[FernetIndex].z;

	upX = binormals[FernetIndex].x;
	upY = binormals[FernetIndex].y;
	upZ = binormals[FernetIndex].z;
}

// this is the method for display callback
void displayFunc()
{
	// every frame clear the color buffer and depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// slide 06-12
	// changing matrix mode to Model View Matrix to set up camera
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
	matrix->LoadIdentity();

	//for milestone
	// matrix->LookAt(0, 0, -50, 0, 0, 0, 0, 1, 0);

	// this call updates the Eye, Center and Up values
	calculateCameraLocation();
	matrix->LookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);

	// fetching the model view matrix into m and setting it
	matrix->GetMatrix(m);

	pipelineProgramColor->Bind();
	pipelineProgramColor->SetModelViewMatrix(m);

	pipelineProgramTexture->Bind();
	pipelineProgramTexture->SetModelViewMatrix(m);

	pipelineProgramTexture->Bind();
	
	// select the active texture unit
	setTextureUnit(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
	
	// draw the rails
	glBindTexture(GL_TEXTURE_2D, trackTexHandle); 

	glBindVertexArray(trackVAO); // bind the VAO
	first = 0;
	countVertices = trackVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	// draw the ground
	glBindTexture(GL_TEXTURE_2D, groundTexHandle); 

	glBindVertexArray(groundVAO); // bind the VAO
	first = 0;
	countVertices = groundVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	// draw each of the sky planes
	// 1. back
	glBindTexture(GL_TEXTURE_2D, skyTexHandle); 

	glBindVertexArray(skyBackVAO); // bind the VAO
	first = 0;
	countVertices = skyBackVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	// 2. front
	glBindVertexArray(skyFrontVAO); // bind the VAO
	first = 0;
	countVertices = skyFrontVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	// 3. left
	glBindVertexArray(skyLeftVAO); // bind the VAO
	first = 0;
	countVertices = skyLeftVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	// 4. right
	glBindVertexArray(skyRightVAO); // bind the VAO
	first = 0;
	countVertices = skyRightVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	// 5. top
	glBindVertexArray(skyTopVAO); // bind the VAO
	first = 0;
	countVertices = skyTopVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO
	
	glBindTexture(GL_TEXTURE_2D, crossSectionTexHandle); 

	// draw the track plane
	glBindVertexArray(crossSectionVAO); // bind the VAO
	first = 0;
	countVertices = crossSectionVertices.size()/3;
	glDrawArrays(GL_TRIANGLES, first, countVertices);
	glBindVertexArray(0); // unbind the VAO

	glutSwapBuffers();
}

// this is the method for idle callback
// we take repeated screenshots here to create the animation
void idleFunc()
{
	//if 1000 screenshots taken, still run but just stop taking screenshots
	// if (screenshotCount >= 1000)
	// {
	// 	stop = 1;
	// }

	// //take screenshots while count is less than 1000
	// if (stop == 0)
	// {
	// 	char saveName [10] = "000.jpg";

	// 	// logic to generate names of the continuous screenshots
	// 	saveName[2] = 48 + (screenshotCount % 10);
	// 	saveName[1] = 48 + ((screenshotCount / 10) % 10);
	// 	saveName[0] = 48 + (screenshotCount / 100);
	// 	saveScreenshot(saveName);
	// 	screenshotCount++;
	// }
  	
  	// point index is updated every frame to update the camera location
  	// since point index iterates over splineVertices we increment by 3 for
  	// first point (occurs once) and by 6 (for rest) because of the way spline 
  	// points are stored for GL_LINES
  	if (pointIndex == 0)
  		pointIndex = pointIndex + 3;
  	else if (pointIndex < splineVertices.size() - 3)
  		pointIndex = pointIndex + 6;
  	else
  	{
  		exit(0);
  	}

  	// make the screen update 
	glutPostRedisplay();
}

// this is the method for reshape callback
// set the perspective projection matrix here
void reshapeFunc(int w, int h)
{
	glViewport(0, 0, w, h);

	aspectRatio = (GLfloat)w / (GLfloat) h;       // do not hard code
	matrix->SetMatrixMode(OpenGLMatrix::Projection);
	matrix->LoadIdentity();

	// chose Znear and Zfar to be able to show images upto 768x768
	matrix->Perspective(FoV, aspectRatio, 0.01, 5000.0);

	//upload to GPU
	// NOTE TO SELF: You are getting the current matrix acc to mode
	// loaded in this p that you declared
	// You need this to pass to the BasicPipelineProgram methods
	matrix->GetMatrix(p); 
	pipelineProgramColor->Bind();    
	pipelineProgramColor->SetProjectionMatrix(p);

	pipelineProgramTexture->Bind();
	pipelineProgramTexture->SetProjectionMatrix(p);

	//reset mode back
	matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

// this is the method for keyboard callback
// using the keys pressed we can change the modes and the transformations
void keyboardFunc(unsigned char key, int x, int y)
{
	switch (key)
	{
	  	// exit on ESC key
	    case 27: // ESC key
			exit(0); // exit the program
	    break;

	    case 'x':
	      // take a screenshot
	      saveScreenshot("screenshot.jpg");
	    break;
  	}
}

/*********** HW 2 *****************/

// Given function - initialise texture from image
int initTexture(const char * imageFilename, GLuint textureHandle)
{
	// read the texture image
	ImageIO img;
	ImageIO::fileFormatType imgFormat;
	ImageIO::errorType err = img.load(imageFilename, &imgFormat);

	if (err != ImageIO::OK) 
	{
		printf("Loading texture from %s failed.\n", imageFilename);
		return -1;
	}

	// check that the number of bytes is a multiple of 4
	if (img.getWidth() * img.getBytesPerPixel() % 4) 
	{
		printf
		("Error (%s): The width*numChannels must be a multiple of 4.\n", 
			imageFilename);
		return -1;
	}

	// allocate space for an array of pixels
	int width = img.getWidth();
	int height = img.getHeight();
	unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; 
	// we will use 4 bytes per pixel, i.e., RGBA

	// fill the pixelsRGBA array with the image pixels
	memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
	for (int h = 0; h < height; h++)
		for (int w = 0; w < width; w++) 
		{
			// assign some default byte values 
			//(for the case where img.getBytesPerPixel() < 4)
			pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
			pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
			pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
			pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel 
														// fully opaque

			// set the RGBA channels, based on the loaded image
			int numChannels = img.getBytesPerPixel();
			for (int c = 0; c < numChannels; c++) 
			// only set as many channels as are available in the loaded image; 
				// the rest get the default value
				pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
		}

	// bind the texture
	glBindTexture(GL_TEXTURE_2D, textureHandle);

	// initialize the texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, 
				GL_UNSIGNED_BYTE, pixelsRGBA);

	// generate the mipmaps for this texture
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
					GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// query support for anisotropic texture filtering
	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	printf("Max available anisotropic samples: %f\n", fLargest);
	// set anisotropic texture filtering
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 
		0.5f * fLargest);

	// query for any errors
	GLenum errCode = glGetError();
	if (errCode != 0) 
	{
		printf("Texture initialization error. Error code: %d.\n", errCode);
		return -1;
	}
  
	// de-allocate the pixel array -- it is no longer needed
	delete [] pixelsRGBA;

	return 0;
}

// this method initialises the pipeline program and sets the program handle
void initPipelineColor()
{
	pipelineProgramColor = new BasicPipelineProgram();
	// now we pass name of shader files also since there are 2
	pipelineProgramColor->Init(shaderBasePath, "basic.vertexShader.glsl", 
		"basic.fragmentShader.glsl");
	pipelineProgramColor->Bind();
	programHandlerColor = pipelineProgramColor->GetProgramHandle();
}

void initPipelineTexture()
{
	pipelineProgramTexture = new BasicPipelineProgram();
	pipelineProgramTexture->Init(shaderBasePath, "textureVS.glsl", 
		"textureFS.glsl");
	pipelineProgramTexture->Bind();
	programHandlerTexture = pipelineProgramTexture->GetProgramHandle();
}

// this method initialises the VBOs, VAOs and binds shader variables
// for milestone
// void initSplineBuffer()
// {
// 	//spline VBO
// 	glGenBuffers(1, &splineVBO);
// 	glBindBuffer(GL_ARRAY_BUFFER, splineVBO);

// 	glBufferData(GL_ARRAY_BUFFER, 
//				(splineVertices.size() + splineColors.size()) * sizeof(float), 
// 				NULL, GL_STATIC_DRAW);
// 	glBufferSubData(GL_ARRAY_BUFFER, 0, splineVertices.size() * sizeof(float), 
//					splineVertices.data());
// 	glBufferSubData(GL_ARRAY_BUFFER, splineVertices.size() * sizeof(float), 
// 					splineColors.size() * sizeof(float), splineColors.data());

// 	//spline VAO
// 	glGenVertexArrays(1, &splineVAO);
// 	glBindVertexArray(splineVAO);

// 	pipelineProgramColor->Bind();

// 	glBindBuffer(GL_ARRAY_BUFFER, splineVBO);
// 	glBindVertexArray(splineVBO);
// 	loc = glGetAttribLocation(programHandlerColor, "position");
// 	// check location glsl in - layout qualifier 
// 	glEnableVertexAttribArray(loc);
// 	offset = (const void*) 0; 
// 	stride = 0;
// 	normalized = GL_FALSE;
// 	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

// 	loc = glGetAttribLocation(programHandlerColor, "color");
// 	glEnableVertexAttribArray(loc);
// 	offset = (const void*) (splineVertices.size() * sizeof(float));
// 	stride = 0;
// 	normalized = GL_FALSE;
// 	glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
// }

// this function initialises the VBO, VAO and shader variables 
// for the double rails
void initTrackBuffer()
{
	glGenBuffers(1, &trackVBO);
	glBindBuffer(GL_ARRAY_BUFFER, trackVBO);

	glBufferData(GL_ARRAY_BUFFER, 
				(trackVertices.size() + trackTexCoords.size()) * sizeof(float), 
				NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, trackVertices.size() * sizeof(float), 
					trackVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, trackVertices.size() * sizeof(float), 
					trackTexCoords.size() * sizeof(float), 
					trackTexCoords.data());

	glGenVertexArrays(1, &trackVAO);
	glBindVertexArray(trackVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, trackVBO);
	glBindVertexArray(trackVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, trackVBO);
	glBindVertexArray(trackVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (trackVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);
}

// this function initialises the VBO, VAO and shader variables 
// for the track plane
void initCrossSectionBuffer()
{
	glGenBuffers(1, &crossSectionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, crossSectionVBO);

	glBufferData(GL_ARRAY_BUFFER, 
				(crossSectionVertices.size() + crossSectionTexCoords.size()) 
															* sizeof(float), 
				NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 
					crossSectionVertices.size() * sizeof(float), 
					crossSectionVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, 
					crossSectionVertices.size() * sizeof(float), 
					crossSectionTexCoords.size() * sizeof(float), 
					crossSectionTexCoords.data());

	glGenVertexArrays(1, &crossSectionVAO);
	glBindVertexArray(crossSectionVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, crossSectionVBO);
	glBindVertexArray(crossSectionVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, crossSectionVBO);
	glBindVertexArray(crossSectionVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (crossSectionVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);
}

// this function initialises the VBO, VAO and shader variables 
// for the ground plane
void initGroundBuffer()
{
	//ground VBO
	glGenBuffers(1, &groundVBO);
	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);

	glBufferData(GL_ARRAY_BUFFER, 
			(groundVertices.size() + groundTexCoords.size()) * sizeof(float), 
			NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, groundVertices.size() * sizeof(float), 
					groundVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, groundVertices.size() * sizeof(float), 
					groundTexCoords.size() * sizeof(float), 
					groundTexCoords.data());

	//ground VAO
	glGenVertexArrays(1, &groundVAO);
	glBindVertexArray(groundVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBindVertexArray(groundVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
	glBindVertexArray(groundVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (groundVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);
}

// this function initialises the VBO, VAO and shader variables 
// for each of the 5 sky planes
void initSkyBuffer()
{
	//1. Back
	// sky back VBO
	glGenBuffers(1, &skyBackVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyBackVBO);

	glBufferData(GL_ARRAY_BUFFER, 
			(skyBackVertices.size() + skyBackTexCoords.size()) * sizeof(float), 
			NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, skyBackVertices.size() * sizeof(float), 
					skyBackVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, skyBackVertices.size() * sizeof(float), 
					skyBackTexCoords.size() * sizeof(float), 
					skyBackTexCoords.data());

	//sky back VAO
	glGenVertexArrays(1, &skyBackVAO);
	glBindVertexArray(skyBackVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, skyBackVBO);
	glBindVertexArray(skyBackVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, skyBackVBO);
	glBindVertexArray(skyBackVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (skyBackVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);

	//sky Front VBO
	glGenBuffers(1, &skyFrontVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyFrontVBO);

	glBufferData(GL_ARRAY_BUFFER, 
		(skyFrontVertices.size() + skyFrontTexCoords.size()) * sizeof(float), 
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 
		skyFrontVertices.size() * sizeof(float), 
		skyFrontVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, skyFrontVertices.size() * sizeof(float), 
					skyFrontTexCoords.size() * sizeof(float), 
					skyFrontTexCoords.data());

	//2. Front
	//sky Front VAO
	glGenVertexArrays(1, &skyFrontVAO);
	glBindVertexArray(skyFrontVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, skyFrontVBO);
	glBindVertexArray(skyFrontVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, skyFrontVBO);
	glBindVertexArray(skyFrontVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (skyFrontVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);

	// 3. Left
	//sky Left VBO
	glGenBuffers(1, &skyLeftVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyLeftVBO);

	glBufferData(GL_ARRAY_BUFFER, 
			(skyLeftVertices.size() + skyLeftTexCoords.size()) * sizeof(float), 
			NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, skyLeftVertices.size() * sizeof(float), 
					skyLeftVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, skyLeftVertices.size() * sizeof(float), 
					skyLeftTexCoords.size() * sizeof(float), 
					skyLeftTexCoords.data());

	//sky Left VAO
	glGenVertexArrays(1, &skyLeftVAO);
	glBindVertexArray(skyLeftVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, skyLeftVBO);
	glBindVertexArray(skyLeftVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, skyLeftVBO);
	glBindVertexArray(skyLeftVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (skyLeftVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);

	// 4. Right
	//sky Right VBO
	glGenBuffers(1, &skyRightVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyRightVBO);

	glBufferData(GL_ARRAY_BUFFER, 
		(skyRightVertices.size() + skyRightTexCoords.size()) * sizeof(float), 
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 
					skyRightVertices.size() * sizeof(float), 
					skyRightVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, skyRightVertices.size() * sizeof(float), 
					skyRightTexCoords.size() * sizeof(float), 
					skyRightTexCoords.data());

	//sky Right VAO
	glGenVertexArrays(1, &skyRightVAO);
	glBindVertexArray(skyRightVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, skyRightVBO);
	glBindVertexArray(skyRightVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, skyRightVBO);
	glBindVertexArray(skyRightVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (skyRightVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);

	// 5. Top
	//sky Top VBO
	glGenBuffers(1, &skyTopVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyTopVBO);

	glBufferData(GL_ARRAY_BUFFER, 
			(skyTopVertices.size() + skyTopTexCoords.size()) * sizeof(float), 
			NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, skyTopVertices.size() * sizeof(float), 
					skyTopVertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, skyTopVertices.size() * sizeof(float), 
					skyTopTexCoords.size() * sizeof(float), 
					skyTopTexCoords.data());

	//sky Top VAO
	glGenVertexArrays(1, &skyTopVAO);
	glBindVertexArray(skyTopVAO);

	pipelineProgramTexture->Bind();

	glBindBuffer(GL_ARRAY_BUFFER, skyTopVBO);
	glBindVertexArray(skyTopVAO);
	loc = glGetAttribLocation(programHandlerTexture, "position");
	glEnableVertexAttribArray(loc);
	offset = (const void*) 0; 
	stride = 0;
	normalized = GL_FALSE;
	glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);

	glBindBuffer(GL_ARRAY_BUFFER, skyTopVBO);
	glBindVertexArray(skyTopVAO);
	loc = glGetAttribLocation(programHandlerTexture, "texCoord");
	glEnableVertexAttribArray(loc);
	offset = (const void*) (skyTopVertices.size() * sizeof(float));
	stride = 0;
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, stride, offset);
}

// this method takes care of all the initialisations
void initScene(int argc, char *argv[])
{
	// set background color
	glClearColor(0.85, 0.85, 0.85, 1.0);

	glEnable(GL_DEPTH_TEST);
	matrix = new OpenGLMatrix();

	//slide 12-34
	// create an integer handle for the texture
	glGenTextures(1, &groundTexHandle);
	code = initTexture("grass.jpg", groundTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}

	glGenTextures(1, &skyTexHandle);
	code = initTexture("night.jpg", skyTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}

	glGenTextures(1, &crossSectionTexHandle);
	code = initTexture("bars.jpg", crossSectionTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}

	glGenTextures(1, &trackTexHandle);
	code = initTexture("metal.jpg", trackTexHandle);
	if (code != 0)
	{
		printf("Error loading the texture image.\n");
		exit(EXIT_FAILURE);
	}

	createPoints();
	createGround();
	createSky();
	createCrossSection();

	initPipelineColor();
	//initSplineBuffer();			// for milestone
	initPipelineTexture();
	initGroundBuffer();
	initSkyBuffer();
	initCrossSectionBuffer();
	initTrackBuffer();
}

// main method
int main(int argc, char *argv[])
{
	if (argc<2)
	{  
		printf ("usage: %s <trackfile>\n", argv[0]);
		exit(0);
	}

	cout << "Initializing GLUT..." << endl;
	glutInit(&argc,argv);

	cout << "Initializing OpenGL..." << endl;

	#ifdef __APPLE__
		glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | 
							GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
	#else
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | 
							GLUT_STENCIL);
	#endif

	// initialise the window
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(0, 0);  
	glutCreateWindow(windowTitle);

	cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
	cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
	cout << "Shading Language Version: " << 
				glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// tells glut to use a particular display function to redraw 
	glutDisplayFunc(displayFunc);
	// perform animation inside idleFunc
	glutIdleFunc(idleFunc);
	// callback for resizing the window
	glutReshapeFunc(reshapeFunc);
	// callback for pressing the keys on the keyboard
	glutKeyboardFunc(keyboardFunc);

	// init glew
	#ifdef __APPLE__
		// nothing is needed on Apple
	#else
		// Windows, Linux
		GLint result = glewInit();
		if (result != GLEW_OK)
		{
			cout << "error: " << glewGetErrorString(result) << endl;
			exit(EXIT_FAILURE);
		}
	#endif

	// load the splines from the provided filename
	loadSplines(argv[1]);

	printf("Loaded %d spline(s).\n", numSplines);
	for(int i=0; i<numSplines; i++)
    	printf("Num control points in spline %d: %d.\n", i, 
    		splines[i].numControlPoints);

	// do initialization
	initScene(argc, argv);

	// sink forever into the glut loop
	glutMainLoop();

	return (0);
}