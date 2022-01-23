#pragma once
#include "DXDefines.h"
#include "JFStructures.h"
#include "JFGraphics.h"
#include "LWindow.h"
class JellyFishDev
{
public: 
	JellyFishDev() = default;
	void Init(ConfigInfo& config);
	void Update();
	void Render();
	void Cleanup();

	DXRGlobal* getDXRGlobal() {return &dxr;}
	D3D12Global* getDXGlobal() {return &d3d;}
	D3D12Resources* getResources() { return &resources; }
	D3D12ShaderCompilerInfo* getShaderInfo() { return &shaderCompiler; }
private:
	HWND window;
	

	DXRGlobal dxr = {};
	D3D12Global d3d = {};
	D3D12Resources resources = {};
	D3D12ShaderCompilerInfo shaderCompiler;

	bool m_useRayTracing = false;
};

