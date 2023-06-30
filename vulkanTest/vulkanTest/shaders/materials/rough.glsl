#include "common.glsl"

vec3 rough_bsdf(WaveFrontMaterial material, vec3 w_i, vec3 w_o, vec3 n, vec3 m) {
  // m = micronormal
  // n = macronormal
  // https://www.pbr-book.org/3ed-2018/Reflection_Models/Microfacet_Models
  vec3  R      = material.diffuse;
  float ni     = 1.0;
  float no     = 6;
  float wiDotn = abs(dot(w_i, n));
  float woDotn = abs(dot(w_o, n));
  vec3  hr     = vec3((wiDotn / abs(wiDotn)) * (w_i + w_o));

  if (wiDotn == 0 || woDotn == 0 || length(hr) == 0)
    return vec3(0.0);

  hr            = normalize(hr);
  float fresnel = fresnel_factor(w_i, hr, ni, no);
  float alpha   = material.roughness;
  float D, G;

  if (material.ndf == GGX) {
    G = ggx_G(alpha, w_i, w_o, hr, n);
    D = ggx_D(alpha, hr, n);
  }
  if (material.ndf == BECKMANN) {
    G = beckmann_G(alpha, w_i, w_o, hr, n);
    D = beckmann_D(alpha, hr, n);
  }
  if (material.ndf == PHONG) {
    G = phong_G(alpha, w_i, w_o, hr, n);
    D = phong_D(alpha, hr, n);
  }

  vec3 f = (R * fresnel * G * D) / (4.0f * abs(wiDotn) * abs(woDotn));

  return f;
}

vec3 getAttenuation(WaveFrontMaterial material, vec3 w_i, vec3 w_o, vec3 n, vec3 m) {

  float alpha     = material.roughness;
  vec3  f         = rough_bsdf(material, w_i, w_o, n, m);
  float absWidotN = abs(dot(w_i, n));
  float pdf       = ggx_D(alpha, m, n) * ggx_g1(alpha, w_o, m, n) * abs(dot(w_o, m)) /
              abs(dot(w_o, n)); // ggx_D(alpha,m,n) * abs(dot(n, m));

  if (pdf == 0)
    return vec3(0);
  pdf = 1;
  return f * absWidotN / pdf;

}