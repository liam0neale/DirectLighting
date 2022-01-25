#pragma once
#include <Windows.h>


#include "DXDefines.h"
#include "D3dx12.h"
#include "JFStructures.h"
#include "Utils.h"
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

using namespace DirectX;
using namespace Microsoft::WRL;

static const D3D12_HEAP_PROPERTIES UploadHeapProperties =
{
	D3D12_HEAP_TYPE_UPLOAD,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0, 0
};

static const D3D12_HEAP_PROPERTIES DefaultHeapProperties =
{
	D3D12_HEAP_TYPE_DEFAULT,
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
	D3D12_MEMORY_POOL_UNKNOWN,
	0, 0
};

namespace Window
{
	HRESULT Create(LONG width, LONG height, HINSTANCE& instance, HWND& window, LPCSTR title);
}

namespace D3DResources
{
	void Create_Buffer(D3D12Global& d3d, D3D12BufferCreateInfo& info, ID3D12Resource** ppResource);
	void Create_Texture(D3D12Global& d3d, D3D12Resources& resources, Material& material);
	void Create_Vertex_Buffer(D3D12Global& d3d, D3D12Resources& resources, Model& model);
	void Create_Index_Buffer(D3D12Global& d3d, D3D12Resources& resources, Model& model);
	void Create_Constant_Buffer(D3D12Global& d3d, ID3D12Resource** buffer, UINT64 size);
	void Create_BackBuffer_RTV(D3D12Global& d3d, D3D12Resources& resources);
	void Create_View_CB(D3D12Global& d3d, D3D12Resources& resources);
	void Create_Material_CB(D3D12Global& d3d, D3D12Resources& resources, const Material& material);
	void Create_Descriptor_Heaps(D3D12Global& d3d, D3D12Resources& resources);

	void Update_View_CB(D3D12Global& d3d, D3D12Resources& resources);

	void Upload_Texture(D3D12Global& d3d, ID3D12Resource* destResource, ID3D12Resource* srcResource, const TextureInfo& texture);

	void Destroy(D3D12Resources& resources);
}

namespace D3DShaders
{
	void Init_Shader_Compiler(D3D12ShaderCompilerInfo& shaderCompiler);
	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program);
	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob);
	void Destroy(D3D12ShaderCompilerInfo& shaderCompiler);
}

namespace D3D12
{
	void Create_Device(D3D12Global& d3d);
	void Create_CommandList(D3D12Global& d3d);
	void Create_Command_Queue(D3D12Global& d3d);
	void Create_Command_Allocator(D3D12Global& d3d);
	void Create_CommandList(D3D12Global& d3d);
	void Create_Fence(D3D12Global& d3d);
	void Create_SwapChain(D3D12Global& d3d, HWND& window);

	ID3D12RootSignature* Create_Root_Signature(D3D12Global& d3d, const D3D12_ROOT_SIGNATURE_DESC& desc);
	

	void Reset_CommandList(D3D12Global& d3d);
	void Submit_CmdList(D3D12Global& d3d);
	void Present(D3D12Global& d3d);
	void WaitForGPU(D3D12Global& d3d);
	void MoveToNextFrame(D3D12Global& d3d);

	void Destroy(D3D12Global& d3d);
}

namespace Rasteriser
{
	ID3D12RootSignature* CreateRasteriserProgram(D3D12Global& d3d);
	
	void LoadAssets(D3D12Global& d3d);
}

namespace DXR
{
	void Create_Bottom_Level_AS(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources, Model& model);
	void Create_Top_Level_AS(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources);
	void Create_RayGen_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler);
	void Create_Miss_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler);
	void Create_Closest_Hit_Program(D3D12Global& d3d, DXRGlobal& dxr, D3D12ShaderCompilerInfo& shaderCompiler);
	void Create_Pipeline_State_Object(D3D12Global& d3d, DXRGlobal& dxr);
	void Create_Shader_Table(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources);
	void Create_Descriptor_Heaps(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources, const Model& model);
	void Create_DXR_Output(D3D12Global& d3d, D3D12Resources& resources);

	void Build_Command_List(D3D12Global& d3d, DXRGlobal& dxr, D3D12Resources& resources);

	void Destroy(DXRGlobal& dxr);
}