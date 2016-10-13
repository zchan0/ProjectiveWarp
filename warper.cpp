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

static std::string input, output;
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
  switch (argc) {
  case 2: case 3:
  	input  = argv[1];
    output = argv[2] != NULL ? argv[2] : "output.png";
    return true; break;

  default:
  	std::cerr << "Usage: warper inimage.png [outimage.png]" << std:: endl;
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
      std::cout << "Projective!" << std::endl;
      break;
    default:
      std::cerr << "ERROR: undefined transform specification OR invalid params" << std::endl;
      return false;
      break;
  }

  std::cout << "sepcMatrix " << std::endl;
  sepcMatrix.print();

  multiMatrix = sepcMatrix * multiMatrix;
  std::cout << "multiMatrix" << std::endl;
  multiMatrix.print();

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

void inputOperations()
{
	std::string line;
	while(std::getline(std::cin, line)) {
		if (line == "d") {
			break;
    }
		
    if(!parseOperation(line)) {
      std::cerr << "ERROR: invalid operation" << std::endl;
      continue;
    }
	}
}

/** warp */

void X(double u, double v)
{
  double sum = 0.0;
  std::cout << multiMatrix[0] << std::endl; 
}

void Y(double u, double v)
{
  double sum = 0.0;
  std::cout << multiMatrix[1] << std::endl; 
}

void inverseMap()
{

}

int main(int argc, char *argv[])
{
	inputOperations();

	return 0;
}