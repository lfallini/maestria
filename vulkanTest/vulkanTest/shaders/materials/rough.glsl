#include "common.glsl"

vec3 rough_bsdf(WaveFrontMaterial material, vec3 w_i, vec3 w_o, vec3 n, vec3 m) {
  // m = micronormal
  // n = macronormal

  float ni      = 1.0;
  float no      = 1.33;
  float wiDotn  = dot(w_i, n);
  float woDotn  = dot(w_o, n);
  vec3  hr      = normalize(vec3((wiDotn / abs(wiDotn)) * (w_i + w_o)));
  float fresnel = fresnel_factor(w_i, hr, ni, no);
  float alpha   = material.roughness;

  float G = ggx_G(alpha, w_i, w_o, hr, n);
  float D = ggx_D(alpha, hr, n);

  float f_r = (fresnel * G * D) / (4.0f * abs(wiDotn) * abs(woDotn));

  return vec3(f_r, f_r, f_r);
}