#version 450

// ### Output passed to fragment shader
layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outViewRay;

// ### Params UBO.  Sared with deffered.vert.glsl 
struct Light {
	vec4 position;
	vec3 color;
	float radius;
};

layout (binding = 5) uniform LUBO 
{
	Light lights[6];
	int debugSwitch;

	vec3 viewRays[3];
	vec3 eye;
	float m22;
	float m32;
	
	mat3 view2world;
} ubo; 

out gl_PerVertex
{
	vec4 gl_Position;
};

// ###
void main() 
{
	outUV = vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2);                 // UV values are in range [0 .. 2], but range [1..2] is outside screen
	outViewRay = ubo.viewRays[gl_VertexIndex%3];                                 // precalculated viewRays 
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0, 1.0f);
}
