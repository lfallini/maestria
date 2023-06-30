#ifndef BECKMANN_GLSL
#define BECKMANN_GLSL

#include "../util.glsl"

vec3 beckmann_sampling(float alpha, vec3 N, float rand1, float rand2) {
  float alphaSqr = alpha * alpha;
  float theta    = atan(sqrt(-alphaSqr * log(1 - rand1)));
  float phi      = 2 * PI * rand2;
  // T is perpendicular to N.
  vec3 T = cross(N, vec3(0.0, 0.0, 1.0));
  return rotate_point(rotate_point(N, T, theta), N, phi);
}

float beckmann_g1(float alpha, vec3 v, vec3 m, vec3 n) {
  /* https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
          Parameters:
          - v: in ray.
          - m: micro-normal.
          - n: macro-normal.
  */

  if (dot(v, m) * dot(v, n) > 0) {
    float theta_v = acos(dot(v, n));
    float a       = 1 / (alpha * abs(tan(theta_v)));
    float result  = a < 1.6 ? (3.535 * a + 2.181 * a * a) / (1 + 2.276 * a + 2.577 * a * a) : 1;
    return result;
  } else {
    return 0;
  }
}

float beckmann_G(float alpha, vec3 wi, vec3 wo, vec3 m, vec3 n) {
  /*if (dot(wi,n)*dot(wi,m) <= 0 || dot(wo,n)*dot(wo,m) <= 0)
  {
      return 0.0f;
  }*/

  float g1_i = beckmann_g1(alpha, wi, m, n);
  float g1_o = beckmann_g1(alpha, wo, m, n);

  return g1_i * g1_o;
};

float beckmann_D(float alpha, vec3 m, vec3 n) {
  float alphaSqr    = alpha * alpha;
  float mDotn       = dot(m, n);
  float mDotnSqr    = mDotn * mDotn;
  float tanThetaSqr = (1.0f - mDotnSqr) / mDotnSqr;
  return (mDotn > 0 ? exp(-tanThetaSqr / alphaSqr) / (PI * alphaSqr * mDotnSqr * mDotnSqr) : 0);
};

#endif // BECKMANN_GLSL