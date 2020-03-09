#include "stdafx.h"
#include "..\3rdParty\ImGui\imgui.h"


// ============================================================================ ImGui main methods ===

VkImGui::VkImGui(OutputWindow *_vk, float width, float height) : vk(_vk)
{
	ImGui::CreateContext();
	fontTexture = new CTexture2d(vk);	
	init(width, height);
	initResources();
	vertexBuf = VK_NULL_HANDLE;
	indexBuf = VK_NULL_HANDLE;
	vertexMem = VK_NULL_HANDLE;
	indexMem = VK_NULL_HANDLE;
	initGui();
};

VkImGui::~VkImGui()
{
	auto sh = vk->_GetShader("__imgui__");
	sh->SetDrawCallback(nullptr, nullptr);
	ImGui::DestroyContext();
	vk->vkDestroy(*fontTexture);

//	// Release all Vulkan resources required for rendering imGui
	vk->vkDestroy(vertexBuf);
	vk->vkDestroy(indexBuf);
	vk->vkFree(vertexMem);
	vk->vkFree(indexMem);

}

// Initialize styles, keys, etc.
void VkImGui::init(float width, float height)
{
	// Color scheme
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(width, height);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

template<typename F>
void CallbackProxy(void *f) { reinterpret_cast<F>(f)(); }

// Initialize all Vulkan resources used by the ui
void VkImGui::initResources()
{
	ImGuiIO& io = ImGui::GetIO();

	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
	io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;
	io.MouseDrawCursor = true;
	// --------------------------------------------------------------------------- Create font texture
	unsigned char* fontData;
	int texWidth, texHeight;
	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

	Image src(fontData, texWidth, texHeight, PF_R8G8B8A8_UNORM);
	fontTexture->_LoadTexture(&src);

	auto sh = vk->_GetShader("__imgui__");
	if (!sh)
		throw std::string("failed to create imgui");

	auto prop = sh->SetDrawCallback([](VkCommandBuffer cb, void *data) {
		auto self = reinterpret_cast<VkImGui*>(data);
		self->drawFrame(cb);
	}, this);

	if (prop)
	{
		auto p = prop->Find("fontSampler");
		if (p && p->val) 
			*reinterpret_cast<VkDescriptorImageInfo **>(p->val) = &fontTexture->descriptor;

		pScale = nullptr;
		pTranslate = nullptr;
		p = prop->Find("scale");
		if (p && p->val)
			pScale = reinterpret_cast<float *>(p->val);

		p = prop->Find("translate");
		if (p && p->val)
			pTranslate = reinterpret_cast<float *>(p->val);
	}

}

// Starts a new imGui frame and sets up windows and ui elements
void VkImGui::newFrame()
{
	if (pScale && pTranslate)
	{
		// we set hear pushconstant valus
		ImGuiIO& io = ImGui::GetIO();
		pScale[0] = 2.0f / io.DisplaySize.x;
		pScale[1] = 2.0f / io.DisplaySize.y;
		pTranslate[0] = -1;
		pTranslate[1] = -1;
	}
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(vk->viewport.width, vk->viewport.height);

	// empty demo window
	ImGui::NewFrame();
	drawGui();
	if (m_showImGuiDemoWindow)
		ImGui::ShowDemoWindow();
	ImGui::Render();

	updateBuffers(); 	// this will copy new data 

	vk->updateFlags.commandBuffers[FORWARD] = true; // vkImGui is using forward rendering render pass and command buffers should be updated
	return;
}

// Update vertex and index buffer containing the imGui elements when required

void VkImGui::updateBuffers()
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	// Note: Alignment is done inside buffer creation
	VkDeviceSize vertexBufferSize = imDrawData->TotalVtxCount * sizeof(ImDrawVert);
	VkDeviceSize indexBufferSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);

	if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
		return;
	}

	// Update buffers only if vertex or index count has been changed compared to current buffer size
	// -- Vertex buffer
	if ((vertexBuf == VK_NULL_HANDLE) || (vertexCount != imDrawData->TotalVtxCount)) 
	{
		if (vertexBuf && vertexMem)
		{
			vkUnmapMemory(vk->device, vertexMem);
			vk->vkDestroy(vertexBuf);
			vk->vkFree(vertexMem);
		}

		vk->_CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuf, vertexMem);
		vertexCount = imDrawData->TotalVtxCount;
		if (vkMapMemory(vk->device, vertexMem, 0, vertexBufferSize, 0, &vertexData) != VK_SUCCESS) {
			throw std::runtime_error("failed to map memory!");
		}
	}

	// -- Index buffer
	VkDeviceSize indexSize = imDrawData->TotalIdxCount * sizeof(ImDrawIdx);
	if ((indexBuf == VK_NULL_HANDLE) || (indexCount < imDrawData->TotalIdxCount)) 
	{
		if (indexBuf && indexMem)
		{
			vkUnmapMemory(vk->device, indexMem);
			vk->vkDestroy(indexBuf);
			vk->vkFree(indexMem);
		}
		vk->_CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indexBuf, indexMem);
		indexCount = imDrawData->TotalIdxCount;
		if (vkMapMemory(vk->device, indexMem, 0, indexBufferSize, 0, &indexData) != VK_SUCCESS) {
			throw std::runtime_error("failed to map memory!");
		}
	}

	// Upload data
	ImDrawVert* vtxDst = (ImDrawVert*)vertexData;
	ImDrawIdx* idxDst = (ImDrawIdx*)indexData;

	for (int n = 0; n < imDrawData->CmdListsCount; n++) {
		const ImDrawList* cmd_list = imDrawData->CmdLists[n];
		memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
		memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
		vtxDst += cmd_list->VtxBuffer.Size;
		idxDst += cmd_list->IdxBuffer.Size;
	}

	// Flush to make writes visible to GPU
	VkMappedMemoryRange mappedRange[] = {
		{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, vertexMem, 0, VK_WHOLE_SIZE /* vertexBufferSize */ },
		{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, indexMem, 0, VK_WHOLE_SIZE /* indexBufferSize */},
	};
		
	vkFlushMappedMemoryRanges(vk->device, 2, mappedRange);
}

// Draw current imGui frame into a command buffer

void VkImGui::drawFrame(VkCommandBuffer commandBuffer)
{
	// Render commands
	ImDrawData* imDrawData = ImGui::GetDrawData();
	int32_t vertexOffset = 0;
	int32_t indexOffset = 0;

	if (imDrawData && imDrawData->CmdListsCount > 0) {

		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuf, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuf, 0, VK_INDEX_TYPE_UINT16);

		for (int32_t i = 0; i < imDrawData->CmdListsCount; i++)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissorRect;
				scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
				scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
				scissorRect.extent.width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
				scissorRect.extent.height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
				if (j >= 0)
				{
					vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset, vertexOffset, 0);
				}
				indexOffset += pcmd->ElemCount;
			}
			vertexOffset += cmd_list->VtxBuffer.Size;
		}
	}
}

// ============================================================================ evens ===
// ... from glfw used to read mouse and keyboards
void VkImGui::_iglfw_cursorPosition(double x, double y) 
{
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(static_cast<float>(x), static_cast<float>(y));
}

void VkImGui::_iglfw_cursorEnter(bool Enter) 
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDrawCursor = Enter;

}

void VkImGui::_iglfw_mouseButton(int button, int action, int mods) 
{ 
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS && button >= 0 && button < COUNT_OF(io.MouseDown))
		io.MouseDown[button] = true;

	if (action == GLFW_RELEASE && button >= 0 && button < COUNT_OF(io.MouseDown))
		io.MouseDown[button] = false;
}

void VkImGui::_iglfw_scroll(double x, double y)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH = static_cast<float>(x);
	io.MouseWheel = static_cast<float>(y);
}

void VkImGui::_iglfw_key(int key, int scancode, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS)
		io.KeysDown[key] = true;
	if (action == GLFW_RELEASE)
		io.KeysDown[key] = false;

	io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
	io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
	io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void VkImGui::_iglfw_character(unsigned int c)
{
	ImGuiIO& io = ImGui::GetIO();
	if (c > 0 && c < 0x10000)
		io.AddInputCharacter((unsigned short)c);
}

// ==================================================================== GUI ===
// used in BPS

inline void VkImGui::initGui()
{
	m_showImGuiDemoWindow = false;
	m_hMesh = -1;
	m_propName = "";
}

void VkImGui::drawGui()
{
	ImGuiIO& io = ImGui::GetIO();
	// property window
		// left panel
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(350, io.DisplaySize.y));
	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysAutoResize |
//		ImGuiWindowFlags_NoResize |
		// ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		// ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize;
	if (!ImGui::Begin(m_propName.size() ? m_propName.c_str() : "Properties", NULL, flags))
	{
		ImGui::End();
		return;
	}
	ImGui::PushItemWidth(ImGui::GetFontSize() * -12);           // Use fixed width for labels (by passing a negative value), the rest goes to widgets. We choose a width proportional to our font size.

	if (ImGui::CollapsingHeader("Stats"))
	{
		if (vk->m_enablePiplineStatistic)
		{

			static const char *pipelineStats[] = {
			"Input assembly vertex count        ",
			"Input assembly primitives count    ",
			"Vertex shader invocations          ",
			"Clipping stage primitives processed",
			"Clipping stage primtives output    ",
			"Fragment shader invocations        " };
			for (int i = 0; i < COUNT_OF(pipelineStats); ++i)
			{
				ImGui::Text("%s : %d", pipelineStats[i], vk->m_pipelineStats[i]);
			}
		}
	}
	if (m_hMesh != -1)
	{
		if (ImGui::CollapsingHeader("Properties"))
			inputProperties(m_hMesh);
	}

	if (ImGui::CollapsingHeader("Tests"))
	{

		ImGui::Checkbox("Show ImGui Demo Window", &m_showImGuiDemoWindow);
	}
	ImGui::End();

}

void VkImGui::ShowProperties(HandleMesh hMesh, const char * name)
{ 
	//vk->AddModel
	m_hMesh = hMesh; 
	m_propName = name ? name : ""; 
}

void VkImGui::_updateTextureData()
{
	static int lastFrame = -2;
	if (lastFrame != ImGui::GetFrameCount())
	{
		lastFrame = ImGui::GetFrameCount();
	
		CResourceManager rm;

		// build list of textures
		m_textureResources.clear();
		rm.Filter(RESID_IMAGE, [&](IResource *res) {
			m_textureResources.emplace_back(res);
		});

		// sort
		std::sort(m_textureResources.begin(), m_textureResources.end(), [&](IResource *a, IResource *b) {
			auto na = rm.GetName(a);
			auto nb = rm.GetName(b);
			return _stricmp(na, nb) <= 0;
		});

		m_textureDescriptors.clear();
		for (auto res : m_textureResources)
		{
			auto pIdx = vk->textures.findIdx(rm.GetName(res));
			m_textureDescriptors.emplace_back(pIdx ? &vk->textures[*pIdx].descriptor : nullptr);
		}
	}
}

Properties * VkImGui::_propertiesForGui(Properties *prop)
{
	auto strcmpext = [] (const char *val, const char **patt, size_t count, bool caseInsesitive = true)
	{
		decltype(_stricmp) *cmp = caseInsesitive ? _stricmp : strcmp;
		for (size_t i = 0; i < count; ++i)
		{
			if (cmp(val, patt[i]) == 0)
				return true;
		}
		return false;
	}; 

	static const char *colors[] = {
		"baseColor", "color"
	};
	static const char *drag01[] = {
		"normalMapScale", "metalness", "roughtness"
	};

	for (auto &p : *prop)
	{
		switch (p.type)
		{
		case Property::PT_F32:
			if ((p.count == 3 || p.count == 4) && strcmpext(p.name, colors, COUNT_OF(colors)))
			{
				p.guiType = PTGUI_COLOR;
			}
			if (p.count <= 4 && strcmpext(p.name, drag01, COUNT_OF(drag01)))
			{
				p.guiType = PTGUI_DRAG;
				p.guiMin = 0.0f;
				p.guiMax = 1.0f;
				p.guiStep = 0.005f;
			}
			break;
		}
	}
	return prop;
}

bool VkImGui::inputProperties(HandleMesh hMesh)
{
	return inputProperties(_propertiesForGui(vk->GetMeshParams(hMesh)));
}

bool VkImGui::inputProperties(Properties *prop)
{
	bool change = false;
	for (auto &p : *prop)
	{
		switch (p.guiType)
		{
		case PTGUI_COLOR:
			if (p.count == 3)
				change = ImGui::ColorEdit3(p.name, reinterpret_cast<float *>(p.val));
			else
				change = ImGui::ColorEdit4(p.name, reinterpret_cast<float *>(p.val));
			break;

		case PTGUI_DRAG:
			switch (p.count)
			{
			case 1:
				change = ImGui::DragFloat(p.name, reinterpret_cast<float *>(p.val), p.guiStep, p.guiMin, p.guiMax); break;
			case 2:
				change = ImGui::DragFloat2(p.name, reinterpret_cast<float *>(p.val), p.guiStep, p.guiMin, p.guiMax); break;
			case 3:
				change = ImGui::DragFloat3(p.name, reinterpret_cast<float *>(p.val), p.guiStep, p.guiMin, p.guiMax); break;
			case 4:
				change = ImGui::DragFloat4(p.name, reinterpret_cast<float *>(p.val), p.guiStep, p.guiMin, p.guiMax); break;
			}
			break;
		default:
			switch (p.type)
			{
			case Property::PT_F32:
				switch (p.count) {
				case 1:
					change = ImGui::InputFloat(p.name, reinterpret_cast<float *>(p.val)); break;
				case 2:
					change = ImGui::InputFloat2(p.name, reinterpret_cast<float *>(p.val)); break;
				case 3:
					change = ImGui::InputFloat3(p.name, reinterpret_cast<float *>(p.val)); break;
				case 4:
					change = ImGui::InputFloat4(p.name, reinterpret_cast<float *>(p.val)); break;

				case 16:
				{
					bool Mchange = false;
					float *r, *s, *t;
					_decomposeM(reinterpret_cast<float *>(p.val), &r, &s, &t);

					Mchange = ImGui::DragFloat3("Positon", t, 1.0);
					Mchange |= ImGui::DragFloat3("Rotation", r, 1.0);
					Mchange |= ImGui::DragFloat3("Scale", s, 0.005f);

					if (Mchange)
					{
						float T[16];
						Utils::composeM(T, r, s, t);
						vk->SetModelMatrix(m_hMesh, T);
					}
				}
				break;

				default:
					ImGui::Text(".f32x%d?. %s", p.count, p.name);
				}
				break;

			case Property::PT_TEXTURE:
			{
				IResource *res = nullptr;
				auto pPropVal = reinterpret_cast<VkDescriptorImageInfo **>(p.val);
				change = selectTexture(p.name, pPropVal, &res);
				if (change && res && !*pPropVal)
				{
					re.GetImGui()->vk->AddTexture(p.val, res);
				}
			}
			break;
			default:
				ImGui::Text(".?. %s", p.name);
			}
		}
	}
	return change;
}

bool VkImGui::selectTexture(const char *label, VkDescriptorImageInfo **pPropVal, IResource **outRes)
{
	_updateTextureData();

	int idx = _getTexture(*pPropVal);

	bool change = ImGui::Combo(label, &idx, [](void *data, int idx, const char **out_text) {
		auto &textureResources = *reinterpret_cast<std::vector<IResource*>*>(data);
		if (idx >= 0 && idx < textureResources.size())
		{
			CResourceManager rm;
			*out_text = rm.GetName(textureResources[idx]);
		}
		else {
			*out_text = "[ NOT SET ]";
		}
		return true;
	}, &m_textureResources, static_cast<int>(m_textureResources.size()));

	if (change)
	{
		if (outRes)
			*outRes = m_textureResources[idx];

		*pPropVal = m_textureDescriptors[idx];
	}

	return change;
}

// ============================================================================ helper methods

void VkImGui::_decomposeM(const float *f, float **r, float **s, float **t)
{
	decomposedMatricesCacheEntry *slot = nullptr;
	int prevFrame = ImGui::GetFrameCount() - 1;
	decomposedMatricesCacheEntry *empty = nullptr;
	for (auto &prev : m_decomposedMatricesCache)
	{
		if (!empty && prev.frame != prevFrame)
			empty = &prev;

		if (prevFrame == prev.frame && f == prev.f)
		{
			slot = &prev;
			break;
		}
	}
	if (!slot)
	{
		if (!empty)
		{
			size_t s = m_decomposedMatricesCache.size();
			m_decomposedMatricesCache.resize(s + 4);
			empty = &m_decomposedMatricesCache[s];
		}
		slot = empty;
		slot->f = f;
		Utils::decomposeM(f, slot->r, slot->s, slot->t);
	}
	slot->frame = ImGui::GetFrameCount();
	*r = slot->r;
	*s = slot->s;
	*t = slot->t;
}

int VkImGui::_getTexture(VkDescriptorImageInfo *val)
{
	for (size_t i = 0; i < m_textureDescriptors.size(); ++i)
	{
		if (m_textureDescriptors[i] == val)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}


