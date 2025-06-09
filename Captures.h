#pragma once
#include "Utils.h"
#include "BattleFireDirect3D12.h"
ID3D12Resource* LoadHDRICubeMapFromFile(ID3D12GraphicsCommandList *inCommandList, const char* inFilePath,int inCubeMapResolution,
	LPCTSTR inVSFilePath = L"Res/SkyBoxvs.hlsl", LPCTSTR inFSFilePath = L"Res/Texture2D2CubeMap.hlsl");
ID3D12Resource* CaptureDiffuseIrradiance(ID3D12GraphicsCommandList* inCommandList, ID3D12Resource* inSrcCubeMap,int inCubeMapResolution,
	LPCTSTR inVSFilePath = L"Res/SkyBoxvs.hlsl", LPCTSTR inFSFilePath = L"Res/CaptureDiffuseIrradiance.hlsl");
ID3D12Resource* CapturePrefilteredColor(ID3D12GraphicsCommandList* inCommandList, ID3D12Resource* inSrcCubeMap, int inCubeMapResolution,
	LPCTSTR inVSFilePath = L"Res/SkyBoxvs.hlsl", LPCTSTR inFSFilePath = L"Res/CapturePrefilteredColor.hlsl");
ID3D12Resource* GenerateBRDF(ID3D12GraphicsCommandList* inCommandList, int inResolution=512, LPCTSTR inVSFilePath = L"Res/fsqvs.hlsl", LPCTSTR inFSFilePath = L"Res/CaptureBRDF.hlsl");