#include "ndf/ggx.glsl"

#ifndef UTIL_GLSL
#define UTIL_GLSL

vec3 rotate_point(vec3 point, vec3 axis, float angle) {
    axis = normalize(axis);
    float c = cos(angle);
    float s = sin(angle);
    float t = 1.0 - c;

	// https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Rotate-About-Axis-Node.html
    // Construct the rotation matrix
    mat3 rotationMatrix = mat3(
        t * axis.x * axis.x + c,           t * axis.x * axis.y - s * axis.z,  t * axis.x * axis.z + s * axis.y,
        t * axis.x * axis.y + s * axis.z,  t * axis.y * axis.y + c,           t * axis.y * axis.z - s * axis.x,
        t * axis.x * axis.z - s * axis.y,  t * axis.y * axis.z + s * axis.x,  t * axis.z * axis.z + c
    );

    // Apply the rotation matrix to the point
    return rotationMatrix * point;
}

#endif // UTIL_GLSL