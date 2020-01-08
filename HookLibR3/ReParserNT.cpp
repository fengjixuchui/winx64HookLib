#include "stdafx.h"
#include "ReParserNT.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include "MemoryModule.h"

using namespace std;
ReParserNT * ReParserNT::instance =nullptr;

ReParserNT::ReParserNT()
{
	char  lpBuffer[MAX_PATH] = {0};
	GetSystemDirectoryA(lpBuffer, MAX_PATH - 1);
	string path = lpBuffer;
	path += "\\ntdll.dll";

	fstream file(path, ios::binary | ios::in);
	if (!file.is_open()) 
	{
		MessageBoxA(NULL, path.c_str(), "文件读取失败", 0);
		return;
	}

	file.seekg(0, file.end);
	size_t srcSize = file.tellg();
	
	if (!srcSize) 
	{
		MessageBoxA(NULL, path.c_str(), "文件大小失败", 0);
		file.close();
		return;
	}

	file.seekg(ios::beg);

	char * ntdll = (char *)malloc(srcSize);
	
	file.read(ntdll, srcSize);
	file.clear();
	file.close();

	char * image = 0;
	int imageSize = 0;

	HMEMORYMODULE h = MemoryLoadLibrary1(ntdll, srcSize, &image, &imageSize);
	free(ntdll);

	this->ntdllImageBase = image;
	this->size = imageSize;


}


ReParserNT::~ReParserNT()
{
	 free(this->ntdllImageBase);
}

ReParserNT * ReParserNT::GetInstance()
{
	if (instance) return instance;
	instance = new ReParserNT();
	return instance;
}

void ReParserNT::DestoryInstance()
{
	if (instance) delete instance;
	instance = nullptr;
}

int ReParserNT::GetZwFunctionIndex(char * funcName)
{
	ULONG64 function = this->GetZwFunctionAddr(funcName);
	if (function != 0) 
	{
		function = *(PULONG)(function + 4);
		
	}
	return function;
}


ULONG64 ReParserNT::GetZwFunctionAddr(char * funcName)
{
	PIMAGE_DOS_HEADER pHead = (PIMAGE_DOS_HEADER)this->ntdllImageBase;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(this->ntdllImageBase + pHead->e_lfanew);
	int numberRvaAndSize = pNt->OptionalHeader.NumberOfRvaAndSizes;
	PIMAGE_DATA_DIRECTORY pDir = (PIMAGE_DATA_DIRECTORY)&pNt->OptionalHeader.DataDirectory[0];


	PIMAGE_EXPORT_DIRECTORY pExport = (PIMAGE_EXPORT_DIRECTORY)(this->ntdllImageBase + pDir->VirtualAddress);
	ULONG64 funcAddr = 0;

	for (int i = 0; i<pExport->NumberOfNames; i++)
	{
		int * funcAddress = (int *)(this->ntdllImageBase + pExport->AddressOfFunctions);
		int * names = (int *)(this->ntdllImageBase + pExport->AddressOfNames);
		short * fh = (short *)(this->ntdllImageBase + pExport->AddressOfNameOrdinals);
		int index = -1;
		char * name = this->ntdllImageBase + names[i];

		if (strcmp(name, funcName) == 0)
		{
			index = fh[i];
		}

		if (index != -1)
		{
			funcAddr = (ULONG64)(this->ntdllImageBase + funcAddress[index]);
			break;
		}


	}

	if (!funcAddr)
	{
		printf(("没有找到函数%s\r\n", funcName));

	}
	else
	{
		printf("找到函数%s addr %p\r\n", funcName, funcAddr);
	}


	return funcAddr;
}