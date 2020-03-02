#include "stdafx.h"

namespace BAMS {
namespace CORE {

bool ResShader::_ParseFilename(const WSTR & prg, U32 * pStage, bool * pIsSrc)
{
	bool isSrc = prg.endsWith(L".glsl");
	bool isBin = isSrc ? false : prg.endsWith(L".spv"); // we don't check for ".spv" if it ends ".glsl"

	if (!isSrc && !isBin)
		return false;

	if (pIsSrc)
		*pIsSrc = isSrc;

	auto ends = isSrc ? src : bin;

	U32 stage = -1;
	bool ret = false;
	for (auto f = ends; *f && !ret; ++f)
	{
		for (; *f; ++f)
		{
			if (prg.endsWith(*f))
			{
				ret = true;
				break;
			}
		}
		++stage;
	}
	if (ret && pStage)
		*pStage = stage;

	return ret;
}

void ResShader::_LoadXML()
{
	using tinyxml2::XMLPrinter;
	using tinyxml2::XMLDocument;

	// memory is allocated with ResRawData MemoryAllocator, so we don't have to copy it.
	auto Data = static_cast<const char *>(rb->GetData());
	auto Size = rb->GetSize();
	auto &rm = globalResourceManager;
	WSTR currPath(rm->GetDirPath(rb->Path));
	WSTR cvt;
	if (Data)
	{
		XMLDocument xml;
		xml.Parse(Data, Size);

		auto root = xml.FirstChildElement("Shader");
		for (auto r = root->FirstChildElement("Program"); r; r = r->NextSiblingElement())
		{
			auto fileName = r->GetText();
			if (!fileName)
				continue;

			cvt.UTF8(fileName);
			rm->AbsolutePath(cvt, &currPath);

			if (auto p = AddProgram(cvt))
			{
				p->Timestamp = r->Int64Attribute("Update", 0);
				p->Size = r->Int64Attribute("Size", 0);
			}
		}
	}
}

void ResShader::_SaveXML()
{
	using tinyxml2::XMLPrinter;
	using tinyxml2::XMLDocument;

	auto &rm = globalResourceManager;

	if (rb->Path.size() == 0)
	{
		// build new file name
		int fidx = 1;
		WSTR fn = rb->Name;
		rm->AbsolutePath(fn);
		WSTR fnWithExt = fn + L".shader";
		while (Tools::Exist(fnWithExt))
		{
			++fidx;
			fnWithExt = fn + fidx + L".shader";
		}
		rb->Path = fnWithExt;
	}

	XMLDocument out;
	WSTR currPath(rm->GetDirPath(rb->Path));
	auto *root = out.NewElement("Shader");
	STR cvt;
	auto writeToXML = [&](File &prg) {
		if (prg.IsEmpty()) // don't add to XML empty entries
			return;

		auto *entry = out.NewElement("Program");

		// we need relative path and Filename is absolute
		WSTR fpath = prg.Filename;
		rm->RelativePath(fpath, &currPath);

		// we need UTF8 (to put in XML)
		cvt.UTF8(fpath);

		entry->SetAttribute("Update", prg.Timestamp);
		entry->SetAttribute("Size", static_cast<I64>(prg.Size));
		entry->SetText(cvt.c_str());
		root->InsertEndChild(entry);
	};

	for (auto &prg : Source)
		writeToXML(prg);
	for (auto &prg : Binary)
		writeToXML(prg);


	out.InsertFirstChild(root);
	XMLPrinter prn;
	out.Print(&prn);
	rb->UpdateDataMemory(prn.CStr(), prn.CStrSize() - 1);
}

ResShader::File * ResShader::AddProgram(WSTR filename)
{
	auto rm = globalResourceManager;
	if (!rb->isLoaded() && !rb->isDeleted() && rb->Path.size()>0)
	{
		rm->LoadSync(rb);
	}

	ResBase *res = nullptr;
	U32 stage = 0;
	bool isSrc = true;
	if (_ParseFilename(filename, &stage, &isSrc))
	{
		auto p = isSrc ? &Source[stage] : &Binary[stage];
		rm->AbsolutePath(filename);
		if (p->Filename != filename)
		{
			isModified = true;
			res = rm->FindOrCreate(filename.c_str(), isSrc ? ResShaderSrc::GetTypeId() : ResShaderBin::GetTypeId()); // create resource (if not exist)
			p->Filename = filename;
			p->SetResource(res, this);
			// Timestamp = 0 will trigger compilation of source code or build of final shader program
			p->Size = 0;
			p->Timestamp = 0;
		}
		return p;
	}

	return nullptr;
}

WSTR & ResShader::GetSourceFilename(U32 stage)
{
	assert(stage >= NumSubprogramTypes);
	return Source[stage].Filename;
}

WSTR & ResShader::GetBinaryFilename(U32 stage)
{
	assert(stage >= NumSubprogramTypes);
	return Binary[stage].Filename;
}

void ResShader::Update(ResBase * res)
{
	isUpdateRecived = true;
	_LoadXML();
	isModified = false;

	// step 2: make sure, if we have source, we need compiled program too.
	for (U32 stage = 0; stage < NumSubprogramTypes; ++stage)
	{
		auto &src = Source[stage];
		auto &bin = Binary[stage];
		if (src.IsEmpty() && bin.IsEmpty())
			continue; // nothing to do with empty source and binary at i-stage

		if (!src.IsEmpty() && (bin.IsEmpty() || src.Timestamp > bin.Timestamp) )
		{
			Compile(&src);
		}

		if (!src.IsEmpty() && !bin.IsEmpty() && src.IsModified())
		{
			isModified = true;
			if (src.Timestamp == 0 && src.Size == 0)
			{
				Delete(&src);
			}
			else
			{
				Compile(&src);
			}
		}
	}
}

void ResShader::Save(ResBase * res)
{
	if (!isModified && isUpdateRecived)
		return;
	
	_SaveXML();
	Tools::SaveFile(res->Path, GetData(), GetSize());

	// file saved... but res may be marked as deleted (not file on disk befor save) or with old file time stamp
	globalResourceManager->RefreshResorceFileInfo(res);
}

U32 ResShader::GetBinaryCount()
{
	U32 cnt = 0;
	for (auto &p : Binary)
		if (!p.IsEmpty())
			++cnt;

	return cnt;
}

U32 ResShader::GetSourceCount()
{
	U32 cnt = 0;
	for (auto &p : Binary)
	{
		if (!p.IsEmpty())
			++cnt;
	}

	return cnt;
}

ResBase * ResShader::GetBinary(U32 idx)
{
	for (auto &p : Binary)
	{
		if (!p.IsEmpty())
		{
			if (idx == 0)
				return p.UpdateNotifyReciver.GetResource();
			--idx;
		}
	}

	return nullptr;
}

void ResShader::Compile(File * pSrcFile)
{
	// find matching stage;
	U32 stage = 0;
	File *pBinFile = nullptr;
	for (auto &src : Source)
	{
		if (&src == pSrcFile)
		{
			pBinFile = &Binary[stage];
			break;
		}
		++stage;
	}

	assert(pBinFile);

	if (pBinFile)
	{
		TRACEW(L"compile: " << pSrcFile->Filename.c_str() << L"\n");
		WSTR binFilename = pBinFile->Filename;
		if (binFilename.size() == 0)
		{
			binFilename = pSrcFile->Filename;
			binFilename.resize(binFilename.size() - 4);
			binFilename += L"spv";
		}

		auto pBinRes = pBinFile->GetResource();
		if (pBinRes) {
			pBinRes->ModificationBegin();
		}


		WSTR cmd = L"glslangValidator.exe -V ";
		cmd += pSrcFile->Filename + L" -o " + binFilename;
		if (Tools::WinExec(cmd) == 0)
		{
			AddProgram(binFilename);
		}
		else
		{
			TRACEW(L"compile error\n");
		}
		if (pBinRes)
			pBinRes->ModificationEnd();
	}
}

void ResShader::Link(File * p)
{
	TRACEW(L"link: " << p->Filename.c_str() << L"\n");
	reloadShaderMessageData.wnd = -1;
	reloadShaderMessageData.shader = rb->Name.c_str();
	Message msg = { RELOAD_SHADER, RENDERING_ENGINE, 0, &reloadShaderMessageData };
	IEngine::PostMsg(&msg, 100ms);
}

void ResShader::Delete(File * p)
{
	TRACEW(L"deleted: " << p->Filename.c_str() << L"\n");
	p->Filename = "";
}

/// <summary>
/// Recive notification about source or compiled shader programs.
/// </summary>
/// <param name="res">The resource (with shader program).</param>
/// <param name="type">The type.</param>
void ResShader::Notify(ResBase * res, U32 type)
{
	bool isResModified = false;
	auto getFile = [&](auto &files) {
		for (auto &f : files)
		{
			if (!f.IsEmpty() && f.GetResource() == res)
			{
				isResModified = f.IsModified();
				return &f;
			}
		}
		return (File *)nullptr;
	};

	auto src = getFile(Source);
	auto bin = getFile(Binary);

	assert(src || bin);
	if (!src && !bin)
		return;

	if (isResModified)
	{
		if (res->isDeleted())
			Delete(src ? src : bin);

		if (src)
			Compile(src);

		// we don't need to link after Notify, only source compilation may trigger link... hmm
		if (bin)
			Link(bin);

		isModified = true;
	}
}

void ResShader::IdentifyResourceType(ResBase *res)
{
	if (res->Type != RESID_UNKNOWN)
		return;

	if (res->Path.endsWith(L".glsl"))
		res->Type = RESID_SHADER_SRC;
	else if (res->Path.endsWith(L".spv"))
		res->Type = RESID_SHADER_BIN;
	else if (res->Path.endsWith(L".shader"))
		res->Type = RESID_SHADER;
}

}; // CORE namespace
}; // BAMS namespace