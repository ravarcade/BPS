#version 450
#extension GL_ARB_separate_shader_objects : enable

// cubeShader
// ------------ info for BPS -------------
// ------- DON'T REMOVE LINES BELOW ------
const int CTRL_DEPTHWRITE = 1; 
// ---------------------------------------

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inUV;

layout(binding = 0) uniform SharedUBO {
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 baseColor;
} pc;

layout (location = 0) out vec3 outViewPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outTangent;
layout (location = 3) out vec2 outUV;
layout (location = 4) out vec4 outColor;
layout (location = 5) out vec3 outNormal2;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() 
{
	vec4 pos = vec4(inPos,1);
	
	gl_Position = ubo.proj * ubo.view * pc.model * pos;
	
	// View space
	outViewPos = vec3(ubo.view * pc.model * pos);  // ViewSpace
	mat3 mNormal = mat3(ubo.view) * transpose(inverse(mat3(pc.model))); // Transpose-Inverse is needed for objects with different scale for different axis

	// World space
//	outViewPos = vec3(pc.model * pos); // World space
//	mat3 mNormal = transpose(inverse(mat3(pc.model))); // Transpose-Inverse is needed for objects with different scale for different axis

	outUV = inUV;	
	outColor = pc.baseColor;
	outNormal = mNormal * normalize(inNormal);	
	outTangent = mNormal * normalize(inTangent);

	mat3 mNormal2 = transpose(inverse(mat3(pc.model)));
	outNormal2 = mNormal2 * normalize(inNormal);	
}
