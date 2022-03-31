/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C++ starter code

  Student username: bobtran
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "imageIO.h"
#include "openGLMatrix.h"
#include "texPipelineProgram.h"
#include "basicPipelineProgram.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <iostream>
#include <cstring>
#include <vector>

#ifdef WIN32
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#ifdef WIN32
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

// represents one control point along the spline
struct Point
{
  double x;
  double y;
  double z;
};
// spline struct
// contains how many control points the spline has, and an array of control points
struct Spline
{
  int numControlPoints;
  Point * points;
};

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

OpenGLMatrix * matrix;
TexPipelineProgram * texPipelineProgram;
BasicPipelineProgram * basicPipelineProgram;
GLuint trackBuffer, skyBuffer, groundBuffer, crossbarBuffer;
GLuint trackVAO, skyVAO, groundVAO, crossbarVAO;

GLuint skyTexHandle, groundTexHandle, trackTexHandle, crossbarTexHandle;
vector<float> skyPos, groundPos, crossbarPos, trackPos;
vector<float> skyUVs, groundUVs, crossbarUVs, trackUVs;
vector<float> trackNormals;

float FOV = 90.0;
float eye[3] = {0, 0, 0};
float focus[3] = {0, 0, -1};
float up[3] = {0, 1, 0};

float u = 0;
float step = 0.001, maxLineLength = 0.002;
int uindex = 0;
int controlPoint = 0;
int splineNum = 0;
Spline * splines;
int numSplines;

float s = 0.5;
float M [4][4] = {{(-s),  (2-s),  (s-2),   (s)},
                  {(2*s), (s-3),  (3-2*s),  (-s)},
                  {(-s),  (0),    (s),    (0)},
                  {(0),   (1),    (0),   (0)}};

bool takeSS=true;
int ssCount=0;
int timer = 1000;

vector<Point> splineCoord;
vector<Point> tangentCoord;

Point tangent;
Point normal;
Point binormal;

bool shuffleSplines = false;

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

// returns a unit vector in the direction of the vector passed as a parameter
void normalize(Point &v)
{
    double mag = sqrt(pow(v.x,2) + pow(v.y,2) + pow(v.z,2));
    if (mag>=0)
    {
      v.x = v.x / mag;
      v.y = v.y / mag;
      v.z = v.z / mag;
    }
    else
    {
      v.x = 0;
      v.y = 0;
      v.z = 0;
    }
}

// returns the cross product of the vectors passed as parameters
void computeCrossProduct(Point a, Point b, Point &c)
{
  c.x = a.y * b.z - a.z * b.y;
  c.y = a.z * b.x - a.x * b.z;
  c.z = a.x * b.y - a.y * b.x;
}

// computes the normal and binormal given the tangent at a point
void computeNormal(Point tangent, Point &normal, Point &binormal)
{
  computeCrossProduct(binormal, tangent, normal);
  normalize(normal);

  computeCrossProduct(tangent, normal, binormal);
  normalize(binormal);
}

Point reversePoint(Point a) {
  Point b;
  b.x = -1.0f * a.x;
  b.y = -1.0f * a.y;
  b.z = -1.0f * a.z;
  return b;
}

// adds a triangle and its corresponding UV values to the vector variables trackPos and trackUVs - used to add track coordinates
void addTriangle(Point a, float au, float av, Point aNormal, Point b, float bu, float bv, Point bNormal, Point c, float cu, float cv, Point cNormal)
{
  trackPos.insert(trackPos.end(), {static_cast<float>(a.x), static_cast<float>(a.y), static_cast<float>(a.z)});
  trackUVs.insert(trackUVs.end(), {au, av});
  trackNormals.insert(trackNormals.end(), {static_cast<float>(aNormal.x), static_cast<float>(aNormal.y), static_cast<float>(aNormal.z)});

  trackPos.insert(trackPos.end(), {static_cast<float>(b.x), static_cast<float>(b.y), static_cast<float>(b.z)});
  trackUVs.insert(trackUVs.end(), {bu, bv});
  trackNormals.insert(trackNormals.end(), {static_cast<float>(bNormal.x), static_cast<float>(bNormal.y), static_cast<float>(bNormal.z)});

  trackPos.insert(trackPos.end(), {static_cast<float>(c.x), static_cast<float>(c.y), static_cast<float>(c.z)});
  trackUVs.insert(trackUVs.end(), {cu, cv});
  trackNormals.insert(trackNormals.end(), {static_cast<float>(cNormal.x), static_cast<float>(cNormal.y), static_cast<float>(cNormal.z)});
}

void setTextureUnit(GLint unit)
{
  GLuint program = texPipelineProgram->GetProgramHandle();
  glActiveTexture(unit); // select the active texture unit
  // get a handle to the “textureImage” shader variable
  GLint h_textureImage = glGetUniformLocation(program, "textureImage");
  // deem the shader variable “textureImage” to read from texture unit “unit”
  glUniform1i(h_textureImage, unit - GL_TEXTURE0);
}

// initializes the buffer with coordinates for the sky texture
void initSky()
{
  //back face
  skyPos.insert(skyPos.end(), {-128, 128, -128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {-128, -128, -128});
  skyUVs.insert(skyUVs.end(), {0, 0});

  skyPos.insert(skyPos.end(), {128, -128, -128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  skyPos.insert(skyPos.end(), {-128, 128, -128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {128, 128, -128});
  skyUVs.insert(skyUVs.end(), {1, 1});

  skyPos.insert(skyPos.end(), {128, -128, -128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  //front face
  skyPos.insert(skyPos.end(), {-128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {-128, -128, 128});
  skyUVs.insert(skyUVs.end(), {0, 0});

  skyPos.insert(skyPos.end(), {128, -128, 128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  skyPos.insert(skyPos.end(), {-128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {128, 128, 128});
  skyUVs.insert(skyUVs.end(), {1, 1});

  skyPos.insert(skyPos.end(), {128, -128, 128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  //right face
  skyPos.insert(skyPos.end(), {128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {128, -128, 128});
  skyUVs.insert(skyUVs.end(), {0, 0});

  skyPos.insert(skyPos.end(), {128, -128, -128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  skyPos.insert(skyPos.end(), {128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {128, 128, -128});
  skyUVs.insert(skyUVs.end(), {1, 1});

  skyPos.insert(skyPos.end(), {128, -128, -128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  //left face
  skyPos.insert(skyPos.end(), {-128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {-128, -128, 128});
  skyUVs.insert(skyUVs.end(), {0, 0});

  skyPos.insert(skyPos.end(), {-128, -128, -128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  skyPos.insert(skyPos.end(), {-128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {-128, 128, -128});
  skyUVs.insert(skyUVs.end(), {1, 1});

  skyPos.insert(skyPos.end(), {-128, -128, -128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  // top face
  skyPos.insert(skyPos.end(), {-128, 128, 128});
  skyUVs.insert(skyUVs.end(), {0, 0});

  skyPos.insert(skyPos.end(), {-128, 128, -128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {128, 128, 128});
  skyUVs.insert(skyUVs.end(), {1, 0});

  skyPos.insert(skyPos.end(), {-128, 128, -128});
  skyUVs.insert(skyUVs.end(), {0, 1});

  skyPos.insert(skyPos.end(), {128, 128, -128});
  skyUVs.insert(skyUVs.end(), {1, 1});

  skyPos.insert(skyPos.end(), {128, 128, 128});
  skyUVs.insert(skyUVs.end(), {1, 0});
}

// initializes the buffer with coordinates for the ground texture
void initGround()
{
  groundPos.insert(groundPos.end(), {-128, -128, -128});
  groundUVs.insert(groundUVs.end(), {0, 1});

  groundPos.insert(groundPos.end(), {128, -128, -128});
  groundUVs.insert(groundUVs.end(), {1, 1});

  groundPos.insert(groundPos.end(), {-128, -128, 128});
  groundUVs.insert(groundUVs.end(), {0, 0});

  groundPos.insert(groundPos.end(), {128, -128, -128});
  groundUVs.insert(groundUVs.end(), {1, 1});

  groundPos.insert(groundPos.end(), {-128, -128, 128});
  groundUVs.insert(groundUVs.end(), {0, 0});

  groundPos.insert(groundPos.end(), {128, -128, 128});
  groundUVs.insert(groundUVs.end(), {1, 0});
}

void getCoordAndTangent(Point& coord, Point& tangent, float& u, double basis_func_spline[4], double basis_func_tangent[4], float C[4][3]) {
  for (int l=0; l<4;l++)
  {
    basis_func_spline[l] = (pow(u,3) * M[0][l]) + (pow(u,2) * M[1][l]) + (pow(u,1) * M[2][l]) + (M[3][l]);
    basis_func_tangent[l] = (3 * pow(u,2) * M[0][l]) + (2 * u * M[1][l]) + M[2][l]; //derivative of p(u)
  }

  coord.x = 0.0;
  coord.y = 0.0;
  coord.z = 0.0;

  tangent.x = 0.0;
  tangent.y = 0.0;
  tangent.z = 0.0;

  for (int m=0;m<4;m++)
  {
    coord.x += basis_func_spline[m]*C[m][0];
    coord.y += basis_func_spline[m]*C[m][1];
    coord.z += basis_func_spline[m]*C[m][2];

    tangent.x += basis_func_tangent[m]*C[m][0];
    tangent.y += basis_func_tangent[m]*C[m][1];
    tangent.z += basis_func_tangent[m]*C[m][2];
  }
}

void recursiveSubdivision(float u0, float u1, float maxLineLength, double basis_func_spline[4], double basis_func_tangent[4], float C[4][3]) {
  Point coord1, coord2, tangent1, tangent2;
  float umid = (u0 + u1) / 2.0f;
  getCoordAndTangent(coord1, tangent1, u0, basis_func_spline, basis_func_tangent, C);
  getCoordAndTangent(coord2, tangent2, u1, basis_func_spline, basis_func_tangent, C);
  double mag = sqrt(pow(coord1.x - coord2.x, 2) + pow(coord1.y - coord2.y, 2) + pow(coord1.z - coord2.z, 2) );
  if (mag > maxLineLength) {
    recursiveSubdivision(u0, umid, maxLineLength, basis_func_spline, basis_func_tangent, C);
    recursiveSubdivision(umid, u1, maxLineLength, basis_func_spline, basis_func_tangent, C);
  } else {
    splineCoord.push_back(coord1);
    normalize(tangent1);
    tangentCoord.push_back(tangent1);
  }
}

// initializes the buffer with coordinates for the spline, tracks and crossbars
void initSplineCoordinates()
{
  float u1 = 0;
  double basis_func_spline[4];
  double basis_func_tangent[4];
  Point coord, tangent;
  float C[4][3];

  //calculate the point on the spline and the tangent
  for (int j = 0; j < numSplines; j++)
  {
    for (int i=0;i<splines[j].numControlPoints-3;i++)
    {
      for (int k=0;k<4;k++)
      {
        C[k][0] = splines[j].points[i+k].x;
        C[k][1] = splines[j].points[i+k].y;
        C[k][2] = splines[j].points[i+k].z;
      }
      recursiveSubdivision(0.0f, 1.0f, maxLineLength, basis_func_spline, basis_func_tangent, C);
    }
  }

  //calculate the coordinates for the rails
  Point v0, v1, v2, v3, v4, v5, v6, v7, v_initial;
  Point n0, n1, b0, b1;
  float alpha = 0.008; //ratio of the normal and binormal
  float beta = 0.01;
  int cb=0;

  v_initial.x = 0.00000000000001;
  v_initial.y = 1;
  v_initial.z = 0.00000000000001;

  computeCrossProduct(tangentCoord[0], v_initial, n0);
  normalize(n0);

  computeCrossProduct(tangentCoord[0], n0, b0);
  normalize(b0);

  Point V0, V1, V2, V3, V4, V5, V6, V7;
  Point cb0, cb1, cb2, cb3;

  for (int i=1; i<splineCoord.size(); i+=10)
  {
    if (i!=0)
    {
      computeNormal(tangentCoord[i], n0, b0);
    }

    v0.x = splineCoord[i].x + alpha * ((-n0.x) + (b0.x));
    v0.y = splineCoord[i].y + alpha * ((-n0.y) + (b0.y));
    v0.z = splineCoord[i].z + alpha * ((-n0.z) + (b0.z));

    v1.x = splineCoord[i].x + alpha * ((n0.x) + (b0.x));
    v1.y = splineCoord[i].y + alpha * ((n0.y) + (b0.y));
    v1.z = splineCoord[i].z + alpha * ((n0.z) + (b0.z));

    v2.x = splineCoord[i].x + alpha * ((n0.x) - (b0.x));
    v2.y = splineCoord[i].y + alpha * ((n0.y) - (b0.y));
    v2.z = splineCoord[i].z + alpha * ((n0.z) - (b0.z));

    v3.x = splineCoord[i].x + alpha * ((-n0.x) - (b0.x));
    v3.y = splineCoord[i].y + alpha * ((-n0.y) - (b0.y));
    v3.z = splineCoord[i].z + alpha * ((-n0.z) - (b0.z));

    b1 = b0;
    computeNormal(tangentCoord[i+10], n1, b1);

    v4.x = splineCoord[i+10].x + alpha * ((-n1.x) + (b1.x));
    v4.y = splineCoord[i+10].y + alpha * ((-n1.y) + (b1.y));
    v4.z = splineCoord[i+10].z + alpha * ((-n1.z) + (b1.z));

    v5.x = splineCoord[i+10].x + alpha * ((n1.x) + (b1.x));
    v5.y = splineCoord[i+10].y + alpha * ((n1.y) + (b1.y));
    v5.z = splineCoord[i+10].z + alpha * ((n1.z) + (b1.z));

    v6.x = splineCoord[i+10].x + alpha * ((n1.x) - (b1.x));
    v6.y = splineCoord[i+10].y + alpha * ((n1.y) - (b1.y));
    v6.z = splineCoord[i+10].z + alpha * ((n1.z) -  (b1.z));

    v7.x = splineCoord[i+10].x + alpha * ((-n1.x) - (b1.x));
    v7.y = splineCoord[i+10].y + alpha * ((-n1.y) - (b1.y));
    v7.z = splineCoord[i+10].z + alpha * ((-n1.z) - (b1.z));

    //left rail
    V0.x = v3.x;
    V0.y = v3.y;
    V0.z = v3.z;

    V1.x = v2.x;
    V1.y = v2.y;
    V1.z = v2.z;

    V2.x = v2.x - beta*b0.x;
    V2.y = v2.y - beta*b0.y;
    V2.z = v2.z - beta*b0.z;

    V3.x = v3.x - beta*b0.x;
    V3.y = v3.y - beta*b0.y;
    V3.z = v3.z - beta*b0.z;

    V4.x = v7.x;
    V4.y = v7.y;
    V4.z = v7.z;

    V5.x = v6.x;
    V5.y = v6.y;
    V5.z = v6.z;

    V6.x = v6.x - beta*b1.x;
    V6.y = v6.y - beta*b1.y;
    V6.z = v6.z - beta*b1.z;

    V7.x = v7.x - beta*b1.x;
    V7.y = v7.y - beta*b1.y;
    V7.z = v7.z - beta*b1.z;

    //top
    addTriangle(V6, 0, 1, n1, V2, 0, 0, n0, V1, 1, 0, n0);
    addTriangle(V6, 0, 1, n1, V5, 1, 1, n1, V1, 1, 0, n0);
    //right
    addTriangle(V5, 0, 1, b1, V1, 0, 0, b0, V0, 1, 0, b0);
    addTriangle(V5, 0, 1, b1, V4, 1, 1, b1, V0, 1, 0, b0);
    //bottom
    addTriangle(V7, 0, 1, reversePoint(n1), V3, 0, 0, reversePoint(n0), V0, 1, 0, reversePoint(n0));
    addTriangle(V7, 0, 1, reversePoint(n1), V4, 1, 1, reversePoint(n1), V0, 1, 0, reversePoint(n0));
    //left
    addTriangle(V6, 0, 1, reversePoint(b1), V2, 0, 0, reversePoint(b0), V3, 1, 0, reversePoint(b0));
    addTriangle(V6, 0, 1, reversePoint(b1), V7, 1, 1, reversePoint(b1), V3, 1, 0, reversePoint(b0));

    //right rail
    V0.x = v0.x + beta*b0.x;
    V0.y = v0.y + beta*b0.y;
    V0.z = v0.z + beta*b0.z;

    V1.x = v1.x + beta*b0.x;
    V1.y = v1.y + beta*b0.y;
    V1.z = v1.z + beta*b0.z;

    V2.x = v1.x;
    V2.y = v1.y;
    V2.z = v1.z;

    V3.x = v0.x;
    V3.y = v0.y;
    V3.z = v0.z;

    V4.x = v4.x + beta*b1.x;
    V4.y = v4.y + beta*b1.y;
    V4.z = v4.z + beta*b1.z;

    V5.x = v5.x + beta*b1.x;
    V5.y = v5.y + beta*b1.y;
    V5.z = v5.z + beta*b1.z;

    V6.x = v5.x;
    V6.y = v5.y;
    V6.z = v5.z;

    V7.x = v4.x;
    V7.y = v4.y;
    V7.z = v4.z;

    //top
    addTriangle(V6, 0, 1, n1, V2, 0, 0, n0, V1, 1, 0, n0);
    addTriangle(V6, 0, 1, n1, V5, 1, 1, n1, V1, 1, 0, n0);
    //right
    addTriangle(V5, 0, 1, b1, V1, 0, 0, b0, V0, 1, 0, b0);
    addTriangle(V5, 0, 1, b1, V4, 1, 1, b1, V0, 1, 0, b0);
    //bottom
    addTriangle(V7, 0, 1, reversePoint(n1), V3, 0, 0, reversePoint(n0), V0, 1, 0, reversePoint(n0));
    addTriangle(V7, 0, 1, reversePoint(n1), V4, 1, 1, reversePoint(n1), V0, 1, 0, reversePoint(n0));
    //left
    addTriangle(V6, 0, 1, reversePoint(b1), V2, 0, 0, reversePoint(b0), V3, 1, 0, reversePoint(b0));
    addTriangle(V6, 0, 1, reversePoint(b1), V7, 1, 1, reversePoint(b1), V3, 1, 0, reversePoint(b0));

    //coordinates of the cross bars
    cb0 = v0;
    cb3 = v3;

    cb1.x = v0.x + 0.01*tangentCoord[i].x;
    cb1.y = v0.y + 0.01*tangentCoord[i].y;
    cb1.z = v0.z + 0.01*tangentCoord[i].z;

    cb2.x = v3.x + 0.01*tangentCoord[i].x;
    cb2.y = v3.y + 0.01*tangentCoord[i].y;
    cb2.z = v3.z + 0.01*tangentCoord[i].z;

    crossbarPos.insert(crossbarPos.end(), {static_cast<float>(cb2.x), static_cast<float>(cb2.y), static_cast<float>(cb2.z)});
    crossbarUVs.insert(crossbarUVs.end(), {0, 1});

    crossbarPos.insert(crossbarPos.end(), {static_cast<float>(cb1.x), static_cast<float>(cb1.y), static_cast<float>(cb1.z)});
    crossbarUVs.insert(crossbarUVs.end(), {1, 1});

    crossbarPos.insert(crossbarPos.end(), {static_cast<float>(cb0.x), static_cast<float>(cb0.y), static_cast<float>(cb0.z)});
    crossbarUVs.insert(crossbarUVs.end(), {1, 0});

    crossbarPos.insert(crossbarPos.end(), {static_cast<float>(cb2.x), static_cast<float>(cb2.y), static_cast<float>(cb2.z)});
    crossbarUVs.insert(crossbarUVs.end(), {0, 1});

    crossbarPos.insert(crossbarPos.end(), {static_cast<float>(cb3.x), static_cast<float>(cb3.y), static_cast<float>(cb3.z)});
    crossbarUVs.insert(crossbarUVs.end(), {0, 0});

    crossbarPos.insert(crossbarPos.end(), {static_cast<float>(cb0.x), static_cast<float>(cb0.y), static_cast<float>(cb0.z)});
    crossbarUVs.insert(crossbarUVs.end(), {1, 0});

    b0 = b1;
  }
}

void initVBOTextPipeline(GLuint& vbo, vector<float>& pos, vector<float>& uvs) {
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, (pos.size() + uvs.size()) * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, pos.size() * sizeof(float), pos.data());
  // upload uv data
  glBufferSubData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), uvs.size() * sizeof(float), uvs.data());
}

void initVBOBasicPipeline(GLuint& vbo, vector<float>& pos, vector<float>& uvs, vector<float>& normals) {
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, (pos.size() + normals.size() /*+ uvs.size()*/) * sizeof(float), NULL, GL_STATIC_DRAW);
  // upload position data
  glBufferSubData(GL_ARRAY_BUFFER, 0, pos.size() * sizeof(float), pos.data());
  // upload normals data
  glBufferSubData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), normals.size() * sizeof(float), normals.data());
  // upload uv data
  // glBufferSubData(GL_ARRAY_BUFFER, (pos.size() + normals.size()) * sizeof(float), uvs.size() * sizeof(float), uvs.data());
}

// sets the camera attributes eye, focus and up
void setCameraAttributes(int i)
{
  computeNormal(tangentCoord[i], normal, binormal);

  eye[0] = splineCoord[i].x + 0.03 * normal.x;
  eye[1] = splineCoord[i].y + 0.03 * normal.y;
  eye[2] = splineCoord[i].z + 0.03 * normal.z;

  focus[0] = splineCoord[i+1].x + 0.03 * normal.x;
  focus[1] = splineCoord[i+1].y + 0.03 * normal.y;
  focus[2] = splineCoord[i+1].z + 0.03 * normal.z;

  up[0] = normal.x;
  up[1] = normal.y;
  up[2] = normal.z;
}

void initVAOTextPipeline(GLuint& vao, GLuint& texHandle, GLuint& vbo, vector<float>& pos) {
  glGenVertexArrays(1, &vao);
  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle);

  GLuint program = texPipelineProgram->GetProgramHandle();
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GLuint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*) 0);

  GLuint loc2 = glGetAttribLocation(program, "texCoord");
  glEnableVertexAttribArray(loc2);
  glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, (const void*) (size_t)(pos.size()*sizeof(float)));
  glBindVertexArray(0);
}

void initVAOBasicPipeline(GLuint& vao, GLuint& texHandle, GLuint& vbo, vector<float>& pos, vector<float>& normals) {
  glGenVertexArrays(1, &vao);
  setTextureUnit(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle);

  GLuint program = basicPipelineProgram->GetProgramHandle();
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  GLuint loc = glGetAttribLocation(program, "position");
  glEnableVertexAttribArray(loc);
  glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, (const void*) 0);

  GLuint loc2 = glGetAttribLocation(program, "normal");
  glEnableVertexAttribArray(loc2);
  glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, (const void*) (size_t)(pos.size()*sizeof(float)));
  
  // GLuint loc3 = glGetAttribLocation(program, "texCoord");
  // glEnableVertexAttribArray(loc3);
  // glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, (const void*) (size_t)((pos.size() + normals.size())*sizeof(float)));
  glBindVertexArray(0);
}

void setupPhongShading() {
  GLuint program = basicPipelineProgram->GetProgramHandle();
  float La[4] = {0.5f,0.5f,0.5f,1.0f}; 
  float ka[4] = { 0.23125f, 0.23125f, 0.23125f, 1.0f };
  GLint h_La = glGetUniformLocation(program, "La");
  glUniform4fv(h_La, 1, La);
  GLint h_ka = glGetUniformLocation(program, "ka");
  glUniform4fv(h_ka, 1, ka);

  float Ld[4] = {0.75164f, 0.75164f, 0.75164f, 1.0f }; 
  float kd[4] = { 0.23125f, 0.23125f, 0.23125f, 1.0f };
  GLint h_Ld = glGetUniformLocation(program, "Ld");
  glUniform4fv(h_Ld, 1, Ld);
  GLint h_kd = glGetUniformLocation(program, "kd");
  glUniform4fv(h_kd, 1, kd);

  float Ls[4] = {0.5f,0.5f,0.5,1.0f}; 
  float ks[4] = {0.773911f, 0.773911f, 0.773911f, 1.0f };
  GLint h_Ls = glGetUniformLocation(program, "Ls");
  glUniform4fv(h_Ls, 1, Ls);
  GLint h_ks = glGetUniformLocation(program, "ks");
  glUniform4fv(h_ks, 1, ks);

  GLint h_alpha = glGetUniformLocation(program, "alpha");
  glUniform1f(h_alpha, 1.0f);

  float view[16]; // column-major
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->LoadIdentity();
  matrix->GetMatrix(view);

  glm::vec3 lightDirection = glm::vec3(0.0f, 1.0f, 0.0f); // the "Sun" at noon
  float viewLightDirection[3]; // light direction in the view space

  viewLightDirection[0] = (view[0] * lightDirection.x) + (view[4] * lightDirection.y) + (view[8] * lightDirection.z);
  viewLightDirection[1] = (view[1] * lightDirection.x) + (view[5] * lightDirection.y) + (view[9] * lightDirection.z);
  viewLightDirection[2] = (view[2] * lightDirection.x) + (view[6] * lightDirection.y) + (view[10] * lightDirection.z);
  //upload viewLightDirection to the GPU
  GLint h_viewLightDirection = glGetUniformLocation(program, "viewLightDirection");
  glUniform3fv(h_viewLightDirection, 1, viewLightDirection);

  GLint h_normalMatrix = glGetUniformLocation(program, "normalMatrix");
  float n[16];
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->GetNormalMatrix(n); // get normal matrix
  glUniformMatrix4fv(h_normalMatrix, 1, GL_FALSE, n);
}

void display()
{
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  texPipelineProgram->Bind();

  setCameraAttributes(uindex);
  uindex++;
  u += step;
  if (u>1)
  {
    u = 0;
    controlPoint++;
    if (controlPoint>splines[splineNum].numControlPoints-3)
    {
      controlPoint = 0;
      splineNum++;
      if (splineNum>=numSplines)
      {
        exit(0);
      }
    }
  }

  GLuint program = basicPipelineProgram->GetProgramHandle();
  //projection matrix
  GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  matrix->Perspective(FOV, (1.0*windowWidth/windowHeight), 0.01, 1000.0);
  float p[16];
  matrix->GetMatrix(p);
  texPipelineProgram->Bind();
  texPipelineProgram->SetProjectionMatrix(p);

  //modelview matrix
  GLint h_modelViewMatrix = glGetUniformLocation(program, "modelViewMatrix");
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
  matrix->LoadIdentity();
  matrix->LookAt(eye[0], eye[1], eye[2], focus[0], focus[1], focus[2], up[0], up[1], up[2]); // eye, focus, up
  matrix->Translate(landTranslate[0],landTranslate[1],landTranslate[2]);
  matrix->Rotate(landRotate[0],1,0,0);
  matrix->Rotate(landRotate[1],0,1,0);
  matrix->Rotate(landRotate[2],0,0,1);
  matrix->Scale(landScale[0],landScale[1],landScale[2]);

  float m[16];
  matrix->GetMatrix(m);
  texPipelineProgram->Bind();
  texPipelineProgram->SetModelViewMatrix(m);

  basicPipelineProgram->Bind();
  basicPipelineProgram->SetProjectionMatrix(p);
  basicPipelineProgram->SetModelViewMatrix(m);

  texPipelineProgram->Bind();
  initVAOTextPipeline(groundVAO, groundTexHandle, groundBuffer, groundPos);
  glBindVertexArray(groundVAO);
  glDrawArrays(GL_TRIANGLES, 0, groundPos.size()/3);
  glBindVertexArray(0);

  initVAOTextPipeline(skyVAO, skyTexHandle, skyBuffer, skyPos);
  glBindVertexArray(skyVAO);
  glDrawArrays(GL_TRIANGLES, 0, skyPos.size()/3);
  glBindVertexArray(0);

  initVAOTextPipeline(crossbarVAO, crossbarTexHandle, crossbarBuffer, crossbarPos);
  glBindVertexArray(crossbarVAO);
  glDrawArrays(GL_TRIANGLES, 0, crossbarPos.size()/3);
  glBindVertexArray(0);

  basicPipelineProgram->Bind();
  setupPhongShading();
  initVAOBasicPipeline(trackVAO, trackTexHandle, trackBuffer, trackPos, trackNormals);
  glBindVertexArray(trackVAO);
  glDrawArrays(GL_TRIANGLES, 0, trackPos.size()/3);
  glBindVertexArray(0);

  glutSwapBuffers();
}

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
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // allocate space for an array of pixels
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // fill the pixelsRGBA array with the image pixels
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++)
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // bind the texture
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // initialize the texture
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // generate the mipmaps for this texture
  glGenerateMipmap(GL_TEXTURE_2D);

  // set the texture parameters
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // query support for anisotropic texture filtering
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // set anisotropic texture filtering
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

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

void loadTexture(GLuint& texHandle, const char* filename) {
  glGenTextures(1, &texHandle);
  int code = initTexture(filename, texHandle);
  if (code != 0)
  {
    printf("Error loading the %s texture image.\n", filename);
    exit(EXIT_FAILURE);
  }
}

void initScene(int argc, char *argv[])
{
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable (GL_DEPTH_TEST);

  string filename = "./texture_images/sky.jpg";
  loadTexture(skyTexHandle, filename.c_str());

  filename = "./texture_images/ground.jpg";
  loadTexture(groundTexHandle, filename.c_str());

  filename = "./texture_images/track.jpg";
  loadTexture(trackTexHandle, filename.c_str());

  filename = "./texture_images/crossbar.jpg";
  loadTexture(crossbarTexHandle, filename.c_str());

  texPipelineProgram = new TexPipelineProgram();
  texPipelineProgram->Init("../openGLHelper-starterCode");
  basicPipelineProgram = new BasicPipelineProgram();
  basicPipelineProgram->Init("../openGLHelper-starterCode");
  matrix = new OpenGLMatrix();
  initGround();
  initSky();
  initSplineCoordinates();

  texPipelineProgram->Bind();
  initVBOTextPipeline(groundBuffer, groundPos, groundUVs);
  initVBOTextPipeline(skyBuffer, skyPos, skyUVs);
  initVBOTextPipeline(crossbarBuffer, crossbarPos, crossbarUVs);

  basicPipelineProgram->Bind();
  initVBOBasicPipeline(trackBuffer, trackPos, trackUVs, trackNormals);

  Point v;
  v.x = 0.0000000001;
  v.y = -1;
  v.z = 0.0000000001;

  tangent.x = tangentCoord[0].x;
  tangent.y = tangentCoord[0].y;
  tangent.z = tangentCoord[0].z;

  computeCrossProduct(tangent, v, normal);
  normalize(normal);
  computeCrossProduct(tangent, normal, binormal);
  normalize(binormal);
}

void idleFunc()
{
  // for example, here, you can save the screenshots to disk (to make the animation)
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  // setup perspective matrix...
  GLfloat aspect = (GLfloat) w / (GLfloat) h;
  glViewport(0, 0, w, h);
  matrix->SetMatrixMode(OpenGLMatrix::Projection);
  matrix->LoadIdentity();
  matrix->Perspective(FOV, aspect, 0.01, 1000.0);
  matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

void screenshotTimer(int value)
{
  switch(value){
    case 0:
      if(!takeSS) break;
      char fileName[40];
      sprintf(fileName, "screenshots/%03d.jpg", ssCount);
      saveScreenshot(fileName);
      ssCount++;
      if(ssCount == 1000)
        takeSS = false;
      break;
    default:
      break;
  }
  glutTimerFunc(timer, screenshotTimer, 0);
}

int loadSplines(char * argv)
{
  char * cName = (char *) malloc(128 * sizeof(char));
  FILE * fileList;
  FILE * fileSpline;
  int type, i = 0, j, length;

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
    fscanf(fileSpline, "%d %d", &length, &type);

    // allocate memory for all the points
    splines[j].points = (Point *)malloc(length * sizeof(Point));
    splines[j].numControlPoints = length;

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

  // Fisher-Yates shuffle spline order
  if (shuffleSplines) {
    srand(time(NULL));
    for (int j = numSplines - 1; j > 0; j--) {
      int k = rand() % (j + 1);
      // create temp
      int tempNumControlPoints = splines[j].numControlPoints;
      Point* tempControlPoints = splines[j].points;
      // set j = k
      splines[j].numControlPoints = splines[k].numControlPoints;
      splines[j].points = splines[k].points;
      // set k = temp
      splines[k].numControlPoints = tempNumControlPoints;
      splines[k].points = tempControlPoints;
    }
  }

  return 0;
}

void mouseMotionDragFunc(int x, int y)
{
  // mouse has moved and one of the mouse buttons is pressed (dragging)

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the landscape
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        landTranslate[0] += mousePosDelta[0] * 0.01f;
        landTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        landTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the landscape
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        landRotate[0] += mousePosDelta[1];
        landRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        landRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the landscape
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // mouse has moved
  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // a mouse button has has been pressed or depressed

  // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // take a screenshot
      saveScreenshot("screenshot.jpg");
      break;
  }
}

int main(int argc, char *argv[])
{
  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  // tells glut to use a particular display function to redraw
  glutDisplayFunc(display);
  // perform animation inside idleFunc
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);
  // callback for a timer to save screenshots
  glutTimerFunc(timer, screenshotTimer, 0);

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

  if (argc<2)
  {
    printf ("usage: %s <trackfile>\n", argv[0]);
    exit(0);
  } else if (argc == 3) {
    shuffleSplines = true;
  }

  // load the splines from the provided filename
  loadSplines(argv[1]);

  printf("Loaded %d spline(s).\n", numSplines);
  for(int i=0; i<numSplines; i++)
    printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

  // do initialization
  initScene(argc, argv);

  // sink forever into the glut loop
  glutMainLoop();
}
