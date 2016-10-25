#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <limits>

#include "ImageIO/ImageIO.h"
#include "Matrix/matrix.h"

typedef enum 
{
  Flip   = 'f',
  Scale  = 's',
  Shear  = 'h',
  Rotate = 'r',
  Translate  = 't',
  Projective = 'p'
} Transform;

enum WarpMode
{
  BilinearWarp = 0,
  ProjectiveWarp = 1
};

static const unsigned char ESC = 27;

static int inW, inH, outW, outH;
static unsigned char *outPixmap;
static std::string input, output;
static WarpMode mode;

static ImageIO ioOrigin = ImageIO();
static ImageIO ioWarped = ImageIO();

static Vector2D xycorners[4];
static Matrix3D sepcMatrix  = Matrix3D(); // Single transform matrix
static Matrix3D multiMatrix = Matrix3D(); // Prodcut of multiple matrices

/** Helpers */

/**
 * split string by delimiter
 * credits to http://stackoverflow.com/a/236803
 * @param s     string to be splited
 * @param delim delimiter
 * @param elems splited values
 */
void split(const std::string &s, const char delim, std::vector<std::string> &elems) 
{
  std::stringstream ss;
  ss.str(s);
  std::string item;
    
  while (getline(ss, item, delim)) {
    if (!item.empty()) 
      elems.push_back(item);
  }
}

bool parseCommandLine(int argc, char* argv[]) 
{
  // default set mode to be projective
  mode = ProjectiveWarp;
  
  switch (argc) {
    case 2:
      if (argv[1][0] != '-') {
        input  = argv[1];
        output = "output.png";
        return true;
      }
      std::cerr << "Usage: warper [-b | -i] inimage.png [outimage.png]" << std:: endl;
      exit(1);
      return false; break; 
    case 3: case 4:
      // has option
      if (argv[1][0] == '-') {
        if (argv[1][1] == 'b') {
          mode = BilinearWarp;
        }
        if (argv[1][1] == 'i') {
          // interactive warp
        }
        input  = argv[2];
        output = argv[3] != NULL ? argv[3] : "output.png";
        return true; break;
      }
      // no option 
      else {
        input  = argv[1];
        output = argv[2] != NULL ? argv[2] : "output.png";
        return true; break;
      }

    default:
    	std::cerr << "Usage: warper [-b | -i] inimage.png [outimage.png]" << std:: endl;
      exit(1);
    	return false; break;
  }
}

/**  */

bool executeOperation(const char spec, double *params)
{
    double theta = 0.0;
    sepcMatrix.setidentity(); // reset

    switch(spec) {
    case Rotate:
      theta = params[0] * PI / 180.0;
      sepcMatrix.setelement(1, 1,  cos(theta));
      sepcMatrix.setelement(1, 2, -sin(theta));
      sepcMatrix.setelement(2, 1,  sin(theta));
      sepcMatrix.setelement(2, 2,  cos(theta));
      break;
    case Scale:
      if (params[0] == 0 || params[1] == 0) {
        std::cerr << "ERROR: Sx " << params[0] << " or Sy " << params[1] << " is invalid" << std::endl;
        return false; 
      }
      sepcMatrix.setelement(1, 1, params[0]);
      sepcMatrix.setelement(2, 2, params[1]);
      break;
    case Translate:
      sepcMatrix.setelement(1, 3, params[0]);
      sepcMatrix.setelement(2, 3, params[1]);
      break;
    case Flip:
      // flip horizontal
      if (params[0] == 1) {
        sepcMatrix.setelement(1, 1, -1);  
      }
      // flip vertical
      if (params[1] == 1) {
        sepcMatrix.setelement(2, 2, -1); 
      }
      break;
    case Shear:
      sepcMatrix.setelement(1, 2, params[0]);
      sepcMatrix.setelement(2, 1, params[1]);
      break;
    case Projective:
      sepcMatrix.setelement(3, 1, params[0]);
      sepcMatrix.setelement(3, 2, params[1]);
      break;
    default:
      std::cerr << "ERROR: undefined transform specification OR invalid params" << std::endl;
      return false;
      break;
  }

  multiMatrix = sepcMatrix * multiMatrix;

  return true; 
}

bool parseOperation(std::string operation)
{
  std::vector<std::string> v;
  split(operation, ' ', v);

  if (v.size() < 2) {
    return false;  
  }

  double *params = new double[v.size() - 1];
  for (int i = 1; i < v.size(); ++i) {
    params[i - 1] = stod(v[i]);
  }
  return executeOperation(v[0][0], params);
}

bool inputOperations()
{
	std::string line;
	while(std::getline(std::cin, line)) {
		if (line == "d") {
			return true;
    }
		
    if(!parseOperation(line)) {
      std::cerr << "ERROR: invalid operation" << std::endl;
      continue;
    }
	}
  return false;
}

/** warp */

void forwardMap(float u, float v, float &x, float &y)
{
  Vector2D inVector, outVector;
  inVector.x = u;
  inVector.y = v;
  outVector = multiMatrix * inVector;
  x = outVector.x;
  y = outVector.y;
}

void inverseMap(float x, float y, float &u, float &v)
{
  Vector2D inVector, outVector;
  inVector.x = x;
  inVector.y = y;
  outVector = multiMatrix.inverse() * inVector;  
  u = outVector.x;
  v = outVector.y;
}

/**
 * init outpixmap with out width and out height
 * set alpha channel to 0, makes pixel without color to be transparent
 */
void setupOutPixmap(int outW, int outH)
{
  outPixmap = new unsigned char[RGBA * outW * outH];
  for (int i = 0; i < outH; ++i) 
    for (int j = 0; j < outW; ++j) 
      for (int channel = 0; channel < RGBA; ++channel)
        outPixmap[(i * outW + j) * RGBA + channel] = 0;
}

/** 
 * calculate output image size, with 4 corners of input image
 */
void setupOutSize(int &outW, int &outH)
{
  float minX = std::numeric_limits<float>::max();
  float minY = minX;
  float maxX = std::numeric_limits<float>::min();
  float maxY = maxX;

  int *us = new int[2]; // u coordinates
  int *vs = new int[2]; // v coordinates
  float *xs = new float[2]; // x coordinates
  float *ys = new float[2]; // y coordinates

  us[0] = 0;  us[1] = inW; 
  vs[0] = 0;  vs[1] = inH;
  xs[0] = 0;  xs[1] = 0;
  ys[0] = 0;  ys[1] = 0;

  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      forwardMap(us[i], vs[j], xs[i], ys[j]);
      minX = fmin(minX, xs[i]);
      minY = fmin(minY, ys[i]);
      maxX = fmax(maxX, xs[i]);
      maxY = fmax(maxY, ys[i]);
    }
  }

  std::cout << "\ntranslation (" << -minX << ", " << -minY << ")" << std::endl;
  
  //  adjust coordinates by translation
  sepcMatrix.setidentity();
  if (minX < 0) {
    sepcMatrix.setelement(1, 3, -minX);
  } 
  if (minY < 0) {
    sepcMatrix.setelement(2, 3, -minY);
  }

  multiMatrix = sepcMatrix * multiMatrix;
  std::cout << "after translation, multiMatrix " << std::endl;
  multiMatrix.print();

  // calculate coordinates after translating
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 2; ++j) {
      forwardMap(us[i], vs[j], xs[i], ys[j]);
      // store 4 corners in xycorners
      xycorners[i * 2 + j].x = xs[i];
      xycorners[i * 2 + j].y = ys[j];
    }
  }

  // calculate output size
  // need round up max/min value before substraction
  maxX = std::ceil(maxX);  maxY = std::ceil(maxY);
  minX = std::floor(minX); minY = std::floor(minY);
  outW = maxX - minX;
  outH = maxY - minY;
  std::cout << "outW " << outW << "\toutH " << outH << std::endl;  
  
  // init outPixmap with calculated w / h
  setupOutPixmap(outW, outH);
}

void warp(const unsigned char *inPixmap)
{
  setupOutSize(outW, outH);

  int k, l;
  float u, v;
  for (int y = 0; y < outH; ++y) {
    for (int x = 0; x < outW; ++x) {
      inverseMap(x, y, u, v);
      k = (int)std::floor(v);
      l = (int)std::floor(u);

      if (k < 0 || k >= inH || l < 0 || l >= inW) 
        continue;

      for (int channel = 0; channel < RGBA; ++channel) 
        outPixmap[(y * outW + x) * RGBA + channel] = inPixmap[(k * inW + l) * RGBA + channel];
    }
  }

}

void bilinearWarp(const unsigned char *inPixmap)
{
  setupOutSize(outW, outH);

  int k, l;
  float u, v;
  Vector2D xyVector, uvVector;
  BilinearCoeffs coeff;
  setbilinear(outW, outH, xycorners, coeff);

  for (int y = 0; y < outH; ++y) {
    for (int x = 0; x < outW; ++x) {
      xyVector.x = x;
      xyVector.y = y;
      invbilinear(coeff, xyVector, uvVector);
      k = (int)std::floor(uvVector.y);
      l = (int)std::floor(uvVector.x);

      if (k < 0 || k > inH || l < 0 || l > inW) 
        continue;

      for (int channel = 0; channel < RGBA; ++channel)
        outPixmap[(y * outW + x) * RGBA + channel] = inPixmap[(k * inW + l) * RGBA + channel];
    }
  }
}

void loadImage()
{
  ioOrigin.loadFile(input);
  // setup input image's width and height
  inW = ioOrigin.getWidth();
  inH = ioOrigin.getHeight();

  switch(mode) {
    case ProjectiveWarp:
      warp(ioOrigin.pixmap);
      break;
    case BilinearWarp:
      bilinearWarp(ioOrigin.pixmap);
      break;
    default:
      break;
  }

  ioWarped.setPixmap(outW, outH, outPixmap);
}

void displayOriginWindow() 
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ioOrigin.draw();
}

void displayWarpedWindow()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  ioWarped.draw();
}

void handleReshape(int width, int height) 
{
  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, width, 0, height);
  glMatrixMode(GL_MODELVIEW); 
}

void handleKeyboard(unsigned char key, int x, int y) 
{
  switch(key) {
    case 'w': case 'W': 
      ioWarped.saveImage(output); break;
    case 'q': case 'Q': case ESC: 
      exit(0); break;
  } 
}

void promptInstruction()
{
  std::cout << "--------------------------------------------------------------" << std::endl;
  std::cout << "default warp mode: projective" << std::endl;
  std::cout << "options: " << std::endl;
  std::cout << "\t-b          bilinear switch - do bilinear warp" << std::endl;
  std::cout << "transform specifications: " << std::endl;
  std::cout << "\tr theta     counter clockwise rotation about image origin, theta in degrees" << std::endl
            << "\ts sx sy     scale (watch out for scale by 0!)" << std::endl
            << "\tt dx dy     translate" << std::endl
            << "\tf xf yf     flip - if xf = 1 flip horizontal, yf = 1 flip vertical" << std::endl
            << "\th hx hy     shear" << std::endl
            << "\tp px py     perspective" << std::endl
            << "\td           done" << std::endl;
  std::cout << "--------------------------------------------------------------" << std::endl;
}

int main(int argc, char *argv[])
{
  promptInstruction();

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	
  if (parseCommandLine(argc, argv) && inputOperations()) {
      loadImage();  
  }

  // Origin image window
  glutInitWindowSize(inW, inH);
  glutCreateWindow("Original Image");
  glutDisplayFunc(displayOriginWindow);
  glutKeyboardFunc(handleKeyboard);
  glutReshapeFunc(handleReshape);

  // Warped image window
  glutInitWindowSize(outW, outH);
  glutInitWindowPosition(glutGet(GLUT_WINDOW_X) + inW, 0);
  glutCreateWindow("Warped Image");
  glutDisplayFunc(displayWarpedWindow);
  glutKeyboardFunc(handleKeyboard);
  glutReshapeFunc(handleReshape);

  glutMainLoop();

	return 0;
}