#version 450

// Note: 
// For now in PBR texture position is store (as 32-bit float). It is used to debug.

// ### Samplers: framebuffer attachment from previous deferred render pass
layout (binding = 0) uniform sampler2D samplerNormal;
layout (binding = 1) uniform sampler2D samplerAlbedo;
layout (binding = 2) uniform sampler2D samplerPbr;
layout (binding = 3) uniform sampler2D samplerDepth;

// ### Params passed from vertex shader
layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inViewRay;

// ### Fragment shader output
layout (location = 0) out vec4 outColor;

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

// ### Helper function: Decode packed normal vector
vec3 decode (vec2 enc)
{
    vec2 fenc = enc*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
	
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

// ### Helper function: Restore Z view space value restore from depth buffer
float restoreZ(float z)
{
	return ubo.m32/(ubo.m22+z);
}

// ###
void main() 
{
	// Get G-Buffer values
	vec3 normal = decode(texture(samplerNormal, inUV).rg);
	vec4 albedo = texture(samplerAlbedo, inUV);
	vec4 pbr = texture(samplerPbr, inUV);
	vec4 depth = texture(samplerDepth, inUV);
	vec4 fragcolor;
	
	float vZ = restoreZ(depth.r);
	vec3 fragPos = inViewRay * vZ + ubo.eye;
	vec3 ws_normal = ubo.view2world * normal;

	switch(ubo.debugSwitch)
	{
	case 0:
		fragcolor = vec4(ws_normal.rgb,1);
		break;
		
	case 1:
		fragcolor = vec4(albedo.rgb,1);
		break;
		
	case 2:
//		fragcolor = vec4(vec3(1)-fract(abs(pbr.rgb*0.1)),1);
		fragcolor = vec4(pbr.rgb,1);
		break;
		
	case 3:
		fragcolor = vec4(vec3(1)-fract(abs(fragPos.rgb*0.1)),1);
		break;
		
	case 4:
		fragcolor = vec4(abs(ws_normal.rgb-pbr.rgb)*3.921568627450980392156862745098 * 50 ,1);
		break;

	default:
// -------------------------------------------------------
// -------------------- light ----------------------------
	#define lightCount 6
	#define ambient 0.0
	
	// Ambient part
	fragcolor = vec4(0,0,0, 1);
	for(int i = 0; i < lightCount; ++i)
	{
		// Vector to light
		vec3 L = ubo.lights[i].position.xyz - fragPos;
		// Distance from light to fragment position
		float dist = length(L);

		// Viewer to fragment
		vec3 V = ubo.eye.xyz - fragPos;
		V = normalize(V);
		
		//if(dist < ubo.lights[i].radius)
		{
			// Light to fragment
			L = normalize(L);

			// Attenuation
			float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

			// Diffuse part
			vec3 N = normalize(ws_normal);
			float NdotL = max(0.0, dot(N, L));
			vec3 diff = ubo.lights[i].color * albedo.rgb * NdotL * atten;

			// Specular part
			// Specular map values are stored in alpha of albedo mrt
			vec3 R = reflect(-L, N);
			float NdotR = max(0.0, dot(R, V));
			vec3 spec = ubo.lights[i].color * albedo.a * pow(NdotR, 200.0) * atten;

			fragcolor += vec4(diff + spec,0);	
		}	
	}    	
// ------------------------------------------------------	
// ------------------------------------------------------
	}
	if (vZ > 999.07f) // paint background as grey
	{
		fragcolor = vec4(0.3, 0.3, 0.3, 1);
	}
	outColor = fragcolor;	
	

}