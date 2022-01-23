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
private:
	HWND window;
	Model model;
	Material material;

	DXRGlobal dxr = {};
	D3D12Global d3d = {};
	D3D12Resources resources = {};
	D3D12ShaderCompilerInfo shaderCompiler;

	bool m_useRayTracing = false;
};

