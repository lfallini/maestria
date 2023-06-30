#ifndef STRUCT_GLSL
#define STRUCT_GLSL

#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct hitPayload {
  vec3 radiance;
  vec3 attenuation;
  int  done;
  vec3 rayOrigin;
  vec3 rayDir;
  uint rngState;
};

struct WaveFrontMaterial {
  vec3  diffuse;
  float pad;
  vec3  specular;
  float padding;
  vec3  emission;
  float padding_emission;
  float shininess;
  float roughness;
  int   materialType;
  int   ndf;
};

struct HitInfo {
  vec3              normal;
  vec3              position;
  WaveFrontMaterial material;
};

struct Vertex {
  vec3 pos;
  vec3 nrm;
  vec3 color;
  vec2 texCoord;
};

struct ObjBuffers {
  uint64_t vertices;
  uint64_t indices;
  uint64_t materials;
  uint64_t materialIndices;
};

#endif // STRUCT_GLSL