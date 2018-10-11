#include "stdafx.h"

#include "..\3rdParty\SPRIV-Cross\spirv_glsl.hpp"






void doTests()
{
	BAMS::CResourceManager rm;
	auto code = rm.GetRawDataByName("vert");
	if (!code.IsLoaded())
		rm.LoadSync();

	spirv_cross::CompilerGLSL glsl((uint32_t*)code.GetData(), (code.GetSize()+sizeof(uint32_t)-1)/sizeof(uint32_t));
	auto resources = glsl.get_shader_resources();
	for (auto &resource : resources.sampled_images)
	{
		unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
		unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
		printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

		// Modify the decoration to prepare it for GLSL.
		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

		// Some arbitrary remapping if we want.
		glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
	}

}