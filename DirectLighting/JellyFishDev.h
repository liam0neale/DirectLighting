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

	void CreateRasterProgram(D3D12Global& d3d);
	void CreateRasterPSO(D3D12Global& d3d, Model _model);
	void UpdatePipeline(D3D12Global& d3d, D3D12Resources& resources);
	bool CreatePerObjectConstantBuffer(D3D12Global& d3d, D3D12Resources& resources);

private:
	HWND window;
	

	DXRGlobal dxr = {};
	D3D12Global d3d = {};
	D3D12Resources resources = {};
	D3D12ShaderCompilerInfo shaderCompiler;
	RasterProgram rasterProgram;
	D3D12_VIEWPORT m_viewport; // area that output from rasterizer will be stretched to.
	D3D12_RECT m_scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

};

