#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_debug_printf : enable

#include "struct.glsl"
#include "rng.glsl"

#include "ndf/ggx.glsl"
#include "ndf/phong.glsl"
#include "ndf/beckmann.glsl"

#include "materials/rough.glsl"
#include "weight.glsl"

hitAttributeEXT vec3 attribs;

layout(location = 0) rayPayloadInEXT hitPayload prd;
layout(location = 1) rayPayloadEXT bool isShadowed;
layout(buffer_reference, scalar, std430) buffer Vertices {Vertex v[]; }; // Positions of an object
layout(buffer_reference, scalar) buffer Indices {uvec3 i[]; }; // Triangle indices
layout(buffer_reference, scalar, std430) buffer Materials {WaveFrontMaterial m[]; }; // Array of all materials on an object
layout(buffer_reference, scalar) buffer MatIndices {int i[]; }; // Material ID for each triangle
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
layout(set = 0, binding = 3)    buffer _scene_desc { ObjBuffers i[]; } scene_desc;

void main()
{
	// When contructing the TLAS, we stored the model id in InstanceCustomIndexEXT, so the
	// the instance can quickly have access to the data

	// Object data
	ObjBuffers objResource = scene_desc.i[gl_InstanceCustomIndexEXT];
	MatIndices matIndices  = MatIndices(objResource.materialIndices);
	Materials  materials   = Materials(objResource.materials);
	Indices    indices     = Indices(objResource.indices);
	Vertices   vertices    = Vertices(objResource.vertices);

	// Retrieve the material used on this triangle 'PrimitiveID'
	int               mat_idx = matIndices.i[gl_PrimitiveID];
	WaveFrontMaterial mat     = materials.m[mat_idx];        // Material for this triangle

	// Indices of the triangle
	uvec3 ind = indices.i[gl_PrimitiveID];

	// Vertex of the triangle
	Vertex v0 = vertices.v[ind.x];
	Vertex v1 = vertices.v[ind.y];
	Vertex v2 = vertices.v[ind.z];

	// Barycentric coordinates of the triangle
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

	// Computing the normal at hit position
	vec3 N = v0.nrm.xyz * barycentrics.x + v1.nrm.xyz * barycentrics.y + v2.nrm.xyz * barycentrics.z;
	N      = normalize(vec3(N.xyz * gl_WorldToObjectEXT));        // Transforming the normal to world space

	if (dot(N,gl_WorldRayDirectionEXT) > 0)
		N = -N;

	// Sample distribution
	float rand1 = stepAndOutputRNGFloat(prd.rngState);
	float rand2 = stepAndOutputRNGFloat(prd.rngState);
	
	vec3 m;
	if (mat.ndf == GGX)
		m = ggx_sampling(mat.roughness, N, rand1, rand2);
	if (mat.ndf == BECKMANN)
		m = beckmann_sampling(mat.roughness, N, rand1, rand2);

	m = normalize(m);

	// Computing the coordinates of the hit position
	vec3 P = v0.pos.xyz * barycentrics.x + v1.pos.xyz * barycentrics.y + v2.pos.xyz * barycentrics.z;
	P      = vec3(gl_ObjectToWorldEXT * vec4(P, 1.0));        // Transforming the position to world space

	// Hardcoded (to) light direction
	vec3 L = normalize(vec3(0, 5, 5) - P);
	float NdotL = dot(N, L);

	vec3 diffuse  = mat.diffuse;
	vec3 direct = vec3(0);

	// direct light
	vec3 Ld = vec3(0.0f);
	if (NdotL > 0)
	{
		float tMin   = 0.0001;
		float tMax   = 1e32;
		vec3  origin = P;
		vec3  rayDir = L;
		uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
		isShadowed   = true;

		traceRayEXT(topLevelAS,        // acceleration structure
		            flags,             // rayFlags
		            0xFF,              // cullMask
		            0,                 // sbtRecordOffset
		            0,                 // sbtRecordStride
		            1,                 // missIndex
		            origin,            // ray origin
		            tMin,              // ray min range
		            rayDir,            // ray direction
		            tMax,              // ray max range
		            1                  // payload (location = 1)
		);
		float Li = 0;
		// pdf = 1 for point lights
		float pdf = 1.0f;
		
		if (!isShadowed)
			Ld = rough_bsdf(mat, L,normalize(-gl_WorldRayDirectionEXT),N,m) * abs(NdotL) * Li / pdf; 
	}
	// TODO remove 0.
	mat.shininess = 0;
	
	// Reflect
	vec3 reflected = reflect(gl_WorldRayDirectionEXT, m);

	prd.radiance += prd.attenuation * Ld;
	prd.attenuation *= mat.diffuse * weight(mat, normalize(-gl_WorldRayDirectionEXT),normalize(reflected),m,N);
	/*
		By applying PBR formula (https://www.pbr-book.org/3ed-2018/Light_Transport_I_Surface_Reflection/Path_Tracing#fragment-SampleBSDFtogetnewpathdirection-0)
		attenuation values are to low, therefore we get dark objects.
	prd.attenuation *= getAttenuation(mat, reflected, -gl_WorldRayDirectionEXT, N, m);
	*/
	
	prd.rayOrigin = P + 0.001 * N;
	prd.rayDir    = reflected;
}
