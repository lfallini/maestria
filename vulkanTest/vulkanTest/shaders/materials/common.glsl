#define GGX 0
#define BECKMANN 1
#define PHONG 2

// Walter et. 2007 (https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf eq. 22)
float fresnel_factor(vec3 i, vec3 m, float eta_i, float eta_t) {
  float c         = abs(dot(i, m));
  float g_squared = pow(eta_t, 2) / pow(eta_i, 2) - 1 + pow(c, 2);
  if (g_squared < 0) {
    return 1.0f;
  } else {
    float g = sqrt(g_squared);
    return (pow(g - c, 2) / (2 * pow(g + c, 2))) * (1 + pow(c * (g + c) - 1, 2) / pow(c * (g - c) + 1, 2));
  }
}