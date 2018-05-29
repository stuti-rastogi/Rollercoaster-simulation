#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "basicPipelineProgram.h"
using namespace std;

int BasicPipelineProgram::Init(const char * shaderBasePath, const char * vertexShaderFile, const char * fragmentShaderFile) 
{
    if (BuildShadersFromFiles(shaderBasePath, vertexShaderFile, fragmentShaderFile) != 0)
    //if (BuildShadersFromFiles(shaderBasePath, "textureVS.glsl", "textureFS.glsl") != 0)
    {
        cout << "Failed to build the pipeline program." << endl;
        return 1;
    }

    cout << "Successfully built the pipeline program." << endl;
    return 0;
}

void BasicPipelineProgram::SetModelViewMatrix(const float * m) 
{
    // pass "m" to the pipeline program, as the modelview matrix
    // students need to implement this

    // slide 07-26
    // NOTE TO SELF: Tried putting this in the hw1.cpp file where
    // I was trying to set the matrix, but could not access
    // h_modelViewMatrix, even using the BasicPipelineProgram obj
    // WHY?? Because it is declared as protected!
    // That's why we were given these methods to fill
    GLboolean isRowMajor = false;
    glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
}

void BasicPipelineProgram::SetProjectionMatrix(const float * m) 
{
    // pass "m" to the pipeline program, as the projection matrix
    // students need to implement this

    // slide 07-27
    const GLboolean isRowMajor = false;
    glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, m);
}

int BasicPipelineProgram::SetShaderVariableHandles() 
{
    // set h_modelViewMatrix and h_projectionMatrix
    // students need to implement this

    // slide 07-27
    h_projectionMatrix = glGetUniformLocation(programHandle, "projectionMatrix");

    // return -1 in case of error
    if (h_projectionMatrix == -1)
    {
        cout << "Error setting projection matrix handle." << endl;
        return -1;
    }
    
    // return -1 in case of error
    h_modelViewMatrix = glGetUniformLocation(programHandle, "modelViewMatrix");
    if (h_modelViewMatrix == -1)
    {
        cout << "Error setting modelView matrix handle." << endl;
        return -1;
    }
    return 0;
}

