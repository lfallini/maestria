#define PI 3.14159265359

layout(push_constant) uniform Constants
{
  int frameNumber;
  int maxDepth;
  int numSamples;
  int time;
} constants;