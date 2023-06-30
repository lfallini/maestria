
#ifndef PHONG_GLSL
#define PHONG_GLSL

#include "../constants.glsl"
#include "../util.glsl"

vec3 phong_sampling(float alpha, vec3 N, float rand1, float rand2) {
  
  float theta = acos(pow(rand1, 1 / (alpha + 2)));
  float phi   = 2 * PI * rand2;

  // T is perpendicular to N.
  vec3 T = cross(N, vec3(0.0, 0.0, 1.0));
  return rotate_point(rotate_point(N, T, theta), N, phi);
};

float phong_g1(float alpha, vec3 v, vec3 m, vec3 n) {
  if (dot(v, m) * dot(v, n) > 0) {
    float theta_v = acos(dot(v, n));
    float a       = sqrt(0.5 * alpha + 1) / abs(tan(theta_v));
    float ret     = a < 1.6 ? (3.535 * a + 2.181 * pow(a, 2)) / (1 + 2.276 * a + 2.577 * pow(a, 2)) : 1;
    return ret;
  } else {
    return 0;
  }
};

float phong_G(float alpha, vec3 wi, vec3 wo, vec3 m, vec3 n) {
  /*if (dot(wi,n)*dot(wi,m) <= 0 || dot(wo,n)*dot(wo,m) <= 0)
  {
      return 0.0f;
  }*/

  float g1_i = phong_g1(alpha, wi, m, n);
  float g1_o = phong_g1(alpha, wo, m, n);

  return g1_i * g1_o;
};

float phong_D(float alpha, vec3 m, vec3 n)
{
    float mDotn = dot(m, n);
    float theta_m = acos(mDotn);
    return (mDotn > 0 ? ((alpha + 2) * pow(cos(theta_m), alpha)) / (2 * PI) : 0);
};



#endif // PHONG_GLSL