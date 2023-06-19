#ifndef WEIGHT_GLSL
#define WEIGHT_GLSL

#include "ndf/ggx.glsl"

// Walter et. 2007 eq (41)
float weight(WaveFrontMaterial material, vec3 i, vec3 o, vec3 m, vec3 n) {
  float i_dot_m = abs(dot(i, m));
  float i_dot_n = abs(dot(i, n));
  float m_dot_n = abs(dot(m, n));
  float alpha   = material.roughness;

  return i_dot_m * ggx_G(alpha, i, o, m, n) / (i_dot_n * m_dot_n);
}

// "Importance Sampling Microfacet-Based BSDFs using the Distribution of Visible Normals".
float weight_heitz(WaveFrontMaterial material, vec3 i, vec3 o, vec3 m, vec3 n) {
  float alpha = material.roughness;
  return ggx_g1(alpha, o, m, n);
  /*
   Above formula should be equal to ggx_G(i,o,m,n) / ggx_g1(i,m,n), but for
   some reason doesn't work :(
  */
}

#endif // WEIGHT_GLSL