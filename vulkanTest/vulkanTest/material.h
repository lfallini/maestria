#pragma once
#include "glm/glm.hpp"
#include "glm/ext.hpp"
enum MaterialType { SPECULAR = 1, ROUGH = 2, DIFFUSE = 3 };

class Material {
public:
  glm::vec3    diffuse{0.7};
  glm::vec3    specular{0.0};
  glm::vec3    emission{0.0};
  float        shininess{0.0};
  float        roughness = 0.01;
  /*
	Attributes order matters! 
	if materialType is on top, struct size is 80 bytes. However, placing it 
	here makes struct 64 bytes (Padding and alignment stuff). 
  */
  MaterialType materialType = ROUGH;
};
