// TestingTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BAMEngine.h"


int main()
{
//	S1 st1;
//	S1 st1;
	/*
	S2 st2("hello world");
	S3 st3(st2);
	foo(std::move(st3));
//	S2 sst1;
//	sst1 = st2;

	st1 += " and you";
	st2 += S3("!tada!");
	st1 = (st2 + "haha" + st1);
	st3 += st3;
	st3 = st3 + st3 + st3;
	*/
	
	BAMS::IResourceManager_AddResource("C:\\Work\\BAM Pinball Sim\\BPS\\BAMEngine\\ReadMe.txt");
	BAMS::Resource res(BAMS::IResourceManager_FindByName("ReadMe"));
	{
		auto uid = res.GetUID();
		auto path =res.GetPath();
		auto name = res.GetName();
	}

//	SIZE_T s = ResourceBase::REQ_SIZE;
//	ResourceManifest resman;
//	resman.Create("C:\\Work\\BAM Pinball Sim\\BPS\\BAMEngine\\ReadMe.txt");

	//	rm.Import(&resman);
	/*
	RESID id = rm.Find("hello");
	auto v = rm.Get<TexTest>("hello");
	v->ResourceLoad();
	*/

//	rm.Add("C:\\Work\\BAM Pinball Sim\\BPS\\BAMEngine\\ReadMe.txt");
	return 0;
}

