#ifndef GGX_GLSL
#define GGX_GLSL

#include "../constants.glsl"
#include "../util.glsl"

vec3 ggx_sampling(float roughness, vec3 N, float rand1, float rand2) {
  float alpha = roughness;
  float theta = atan((alpha * sqrt(rand1)) / sqrt(1 - rand1));
  float phi   = 2 * PI * rand2;

  // T is perpendicular to N.
  vec3 T = cross(N, vec3(0.0, 0.0, 1.0));
  return rotate_point(rotate_point(N, T, theta), N, phi);
}

float ggx_g1(float alpha, vec3 v, vec3 m, vec3 n) {
  /* https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
          Parameters:
          - v: in ray.
          - m: micro-normal.
          - n: macro-normal.
  */

  if (dot(v, m) * dot(v, n) > 0) {
    float theta_v = acos(dot(v, n));
    return 2.0f / (1.0f + sqrt(1.0f + pow(alpha, 2) * pow(tan(theta_v), 2)));
  } else {
    return 0.0;
  }
}

float ggx_G(float alpha, vec3 wi, vec3 wo, vec3 m, vec3 n) {
  /*if (dot(wi,n)*dot(wi,m) <= 0 || dot(wo,n)*dot(wo,m) <= 0)
  {
      return 0.0f;
  }*/

  float g1_i = ggx_g1(alpha, wi, m, n);
  float g1_o = ggx_g1(alpha, wo, m, n);

  return g1_i * g1_o;
};

float ggx_D(float alpha, vec3 m, vec3 n) {
  float mDotn   = dot(m, n);
  float theta_m = acos(mDotn);
  return (mDotn > 0 ? pow(alpha, 2) / (PI * pow(cos(theta_m), 4) * pow(pow(alpha, 2) + pow(tan(theta_m), 2), 2)) : 0);
};

#endif // GGX_GLSL