#define PI 3.14159265359

#ifndef CONSTANTS_GLSL
#define CONSTANTS_GLSL

layout(push_constant) uniform Constants
{
  int frameNumber;
  int maxDepth;
  int numSamples;
  int time;
} constants;

#endif // CONSTANTS_GLSL