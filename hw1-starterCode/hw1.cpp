/*
  CSCI 420 Computer Graphics, USC
  Assignment 1: Height Fields with Shaders.
  C++ starter code

  Student username: bobtran
*/

#include "basicPipelineProgram.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "openGLHeader.h"
#include "glutHeader.h"

#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
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
    Point* points;
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
char windowTitle[512] = "CSCI 420 Homework II";

OpenGLMatrix matrix;
BasicPipelineProgram * pipelineProgram;
GLuint trackVBO, skyVBO, groundVBO, crossbarVBO;
GLuint trackVAO, skyVAO, groundVAO, crossbarVAO;
GLuint trackTexHandle, skyTexHandle, groundTexHandle, crossbarTexHandle;

vector<double> trackPos, skyPos, groundPos, crossbarPos;
vector<double> trackUVs, skyUVs, groundUVs, crossbarUVs;

bool takeScreenshot = false;
int screenshotCount = 0;

double s = 0.5;
double step = 0.001;
double M[4][4] = { {-s, 2-s, s-2, s} ,
        {2*s, s-3, 3-2*s, -s}, 
        {-s, 0, s, 0} ,
        {0, 1, 0, 0} };

// the spline array 
Spline* splines;
// total number of splines 
int numSplines;
vector<Point> splineCoords, tangentCoords;

int loadSplines(char* argv)
{
    char* cName = (char*)malloc(128 * sizeof(char));
    FILE* fileList;
    FILE* fileSpline;
    int iType, i = 0, j, iLength;

    // load the track file 
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf("can't open file\n");
        exit(1);
    }

    // stores the number of splines in a global variable 
    fscanf(fileList, "%d", &numSplines);

    splines = (Spline*)malloc(numSplines * sizeof(Spline));

    // reads through the spline files 
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");

        if (fileSpline == NULL)
        {
            printf("can't open file\n");
            exit(1);
        }

        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);

        // allocate memory for all the points
        splines[j].points = (Point*)malloc(iLength * sizeof(Point));
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

int initTexture(const char* imageFilename, GLuint textureHandle)
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
    unsigned char* pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

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
    delete[] pixelsRGBA;

    return 0;
}

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

void initVBO(vector<double> &pos, vector<double> &uvs, GLuint& vbo) {
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (pos.size() + uvs.size()) * sizeof(double), NULL, GL_STATIC_DRAW);
    // upload position data
    glBufferSubData(GL_ARRAY_BUFFER, 0, pos.size() * sizeof(double), pos.data());
    // upload uv data
    glBufferSubData(GL_ARRAY_BUFFER, pos.size() * sizeof(double), uvs.size() * sizeof(double), uvs.data());
}

void initVAO(vector<double>& pos, GLuint& vbo, GLuint &vao, GLuint &texHandle) {
    glGenVertexArrays(1, &vao);
    GLuint program = pipelineProgram->GetProgramHandle();

    glActiveTexture(GL_TEXTURE0);
    GLint h_textureImage = glGetUniformLocation(program, "textureImage");
    glUniform1i(h_textureImage, 0);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    const void* offset = (const void*)0;
    glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, 0, offset);

    GLuint loc2 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc2);
    const void* offset2 = (const void*)(size_t)(pos.size() * sizeof(double));
    glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, 0, offset2);
    glBindVertexArray(0);
}

void display(GLuint &vao, vector<double>& pos) {
    glBindVertexArray(vao);
    GLint first = 0;
    GLsizei numberOfVertices = (pos.size() / 3);
    glDrawArrays(GL_LINES, first, numberOfVertices);
    glBindVertexArray(0);
}

void initSplineCoords() {
    // the brute force way first
    double C[4][3];
    Point coord, tangent;
    int counter = 0;
    for (int i = 0; i < numSplines; i++) { // for each spline
        for (int j = 0; j < splines[i].numControlPoints - 3; j++) { // for each point in that spline
            for (int k = 0; k < 4; k++) { // fill out control matrix with P_k (k = 0 to 3) coefficients info of current spline (splines[i]), current point j
                C[k][0] = splines[i].points[j + k].x;
                C[k][1] = splines[i].points[j + k].y;
                C[k][2] = splines[i].points[j + k].z;
            }

            double B[4][3];
            for (int x = 0; x < 4; x++) { // each row of M1
                for (int y = 0; y < 3; y++) { // each column of M2
                    B[x][y] = 0;
                    for (int z = 0; z < 4; z++) { // each row of M2 - matches col of M1
                        B[x][y] += M[x][z] * C[z][y];
                    }
                }
            }

            coord.x = coord.y = coord.z = 0.0;
            tangent.x = tangent.y = tangent.z = 0.0;
            for (double u = 0; u <= 1.0; u += step) {
                coord.x = pow(u, 3) * B[0][0] + pow(u, 2) * B[1][0] + pow(u, 1) * B[2][0] + B[3][0];
                coord.y = pow(u, 3) * B[0][1] + pow(u, 2) * B[1][1] + pow(u, 1) * B[2][1] + B[3][1];
                coord.z = pow(u, 3) * B[0][2] + pow(u, 2) * B[1][2] + pow(u, 1) * B[2][2] + B[3][2];

                cout << "count: " << counter++ << " x: " << coord.x << " y: " << coord.y << " z: " << coord.z << endl;
                // double temp2 = (3 * pow(u, 2) * M[0][l]) + (2 * u * M[1][l]) + M[2][l];
                // tangent.x += temp2 * C[l][0];
                // tangent.y += temp2 * C[l][1];
                // tangent.z += temp2 * C[l][2];

                splineCoords.push_back(coord);
                // normalize tangent
                // tangentCoords.push_back(tangent);
            }
        }
    }

    // TODO: use this vector of Points[x, y, z] to draw with GL_LINES for now
    for (unsigned int i = 0; i < splineCoords.size() - 1; i++) {
        trackPos.insert(trackPos.end(), { splineCoords.at(i).x,  splineCoords.at(i).y, splineCoords.at(i).z });
        trackUVs.insert(trackUVs.end(), { 0.0, 0.0, 0.0, 0.0});
        trackPos.insert(trackPos.end(), { splineCoords.at(i+1).x,  splineCoords.at(i+1).y, splineCoords.at(i+1).z });
        trackUVs.insert(trackUVs.end(), { 0.0, 0.0, 0.0, 0.0 });
    }
}

void displayFunc() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();

  matrix.LookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);

  matrix.Translate(landTranslate[0], landTranslate[1], landTranslate[2]);
  matrix.Rotate(landRotate[0], 1.0, 0.0, 0.0);
  matrix.Rotate(landRotate[1], 0.0, 1.0, 0.0);
  matrix.Rotate(landRotate[2], 0.0, 0.0, 1.0);
  matrix.Scale(landScale[0], landScale[1], landScale[2]);

  float m[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(m);

  float p[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(p);

  pipelineProgram->Bind();
  pipelineProgram->SetModelViewMatrix(m);
  pipelineProgram->SetProjectionMatrix(p);

  display(trackVAO, trackPos);

  glutSwapBuffers();
}

void idleFunc() {
    // for example, here, you can save the screenshots to disk (to make the animation)
    if (screenshotCount >= 0 && screenshotCount <= 300 && takeScreenshot) {
        char num[4];
        sprintf(num, "%03d", screenshotCount);
        string screenshotName("./Animations/" + string(num) + ".jpg");
        saveScreenshot(screenshotName.c_str());
    }
    else if (screenshotCount >= 300) {
        takeScreenshot = false;
    }

    // make the screen update 
    glutPostRedisplay();
}

void reshapeFunc(int w, int h) {
  glViewport(0, 0, w, h);
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  matrix.Perspective(60.0, (float)w / (float)h, 0.01f, 1000.0f);
  //matrix.SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y) {
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

void mouseButtonFunc(int button, int state, int x, int y) {
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

  // keep track of whether CTRL and SHIFT keys are pressed
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // if CTRL and SHIFT are not pressed, we are in rotate mode
    default:
      controlState = ROTATE;
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y) {
    GLint loc = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "mode");
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
    case 's':
        cout << "Toggle taking screenshots" << endl;
        takeScreenshot = !takeScreenshot;
        break;
    }
}

void initScene(int argc, char *argv[]) {
  glClearColor(1.0f, 1.0f, 0.8f, 0.0f);

  // init buffer read image
  initSplineCoords();
  
  // set up pipeline
  pipelineProgram = new BasicPipelineProgram();
  pipelineProgram->Init("../openGLHelper-starterCode");
  pipelineProgram->Bind();
  
  // set up VBOs
  initVBO(trackPos, trackUVs, trackVBO);

  // set up VAOs
  initVAO(trackPos, trackVBO, trackVAO, trackTexHandle);


  glEnable(GL_DEPTH_TEST);
}

// main does not have order of operations but has callbacks to use async
int main(int argc, char *argv[]) {
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

    #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
        glutReshapeWindow(windowWidth - 1, windowHeight - 1);
    #endif

    // tells glut to use a particular display function to redraw 
    glutDisplayFunc(displayFunc);
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

    if (argc < 2)
    {
        printf("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }

    // load the splines from the provided filename
    loadSplines(argv[1]);

    printf("Loaded %d spline(s).\n", numSplines);
    for (int i = 0; i < numSplines; i++)
        printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);

    // do initialization
    initScene(argc, argv);

    // sink forever into the glut loop
    glutMainLoop();
}
