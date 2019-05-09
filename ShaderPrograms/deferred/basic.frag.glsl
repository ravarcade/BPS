#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout (binding = 1) uniform sampler2D samplerColor;
//layout (binding = 2) uniform sampler2D samplerNormalMap;

layout (location = 0) in vec3 outViewPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inUV;
layout (location = 4) in vec4 inColor;
layout (location = 5) in vec3 inNormal2;


layout (location = 0) out vec2 outNormal;
layout (location = 1) out vec4 outAlbedo;
layout (location = 2) out vec4 outPbr;

// normal encode
vec2 encode (vec3 n)
{
    float p = sqrt(n.z*8+8);
    return n.xy/p + 0.5;
}

void main() 
{
	//outPosition = vec4(outViewPos, 1.0);

	// Calculate normal in tangent space
	vec3 N = normalize(inNormal);
	vec3 T = normalize(inTangent);
	vec3 N2 = normalize(inNormal2);
	
	// re-orthogonalize T with respect to N
	T = normalize(T - dot(T, N) * N);
	
	vec3 B = cross(N, T);
	mat3 TBN = mat3(T, B, N);
//	vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0)); // narazie bez probkowania textury z normalą
	vec3 tnorm = TBN * vec3(0.0, 0.0, 1.0);
//	outNormal = vec4(tnorm.rg * 0.5 + vec2(0.5), 0, 1);
	outNormal = encode(tnorm);
	outAlbedo = inColor; // texture(samplerColor, inUV);
//	outPbr    = vec4(0,0,0,1.0);
	outPbr    = vec4(N2, 1.0);
}