#include "JellyFishDev.h"

void JellyFishDev::Init(ConfigInfo& config)
{
	// Create a new window
	HRESULT hr = Window::Create(config.width, config.height, config.instance, window, "Introduction to DirectX Raytracing (DXR)");
	Utils::Validate(hr, "Error: failed to create window!");
	
	if (m_useRayTracing)
	{
		d3d.width = config.width;
		d3d.height = config.height;
		d3d.vsync = config.vsync;

		// Load a model
		Utils::LoadModel(config.model, model, material);

		// Initialize the shader compiler
		D3DShaders::Init_Shader_Compiler(shaderCompiler);

		// Initialize D3D12
		D3D12::Create_Device(d3d);
		D3D12::Create_Command_Queue(d3d);
		D3D12::Create_Command_Allocator(d3d);
		D3D12::Create_Fence(d3d);
		D3D12::Create_SwapChain(d3d, window);
		D3D12::Create_CommandList(d3d);
		D3D12::Reset_CommandList(d3d);

		// Create common resources
		D3DResources::Create_Descriptor_Heaps(d3d, resources);
		D3DResources::Create_BackBuffer_RTV(d3d, resources);
		D3DResources::Create_Vertex_Buffer(d3d, resources, model);
		D3DResources::Create_Index_Buffer(d3d, resources, model);
		D3DResources::Create_Texture(d3d, resources, material);
		D3DResources::Create_View_CB(d3d, resources);
		D3DResources::Create_Material_CB(d3d, resources, material);

		// Create DXR specific resources
		DXR::Create_Bottom_Level_AS(d3d, dxr, resources, model);
		DXR::Create_Top_Level_AS(d3d, dxr, resources);
		DXR::Create_DXR_Output(d3d, resources);
		DXR::Create_Descriptor_Heaps(d3d, dxr, resources, model);
		DXR::Create_RayGen_Program(d3d, dxr, shaderCompiler);
		DXR::Create_Miss_Program(d3d, dxr, shaderCompiler);
		DXR::Create_Closest_Hit_Program(d3d, dxr, shaderCompiler);
		DXR::Create_Pipeline_State_Object(d3d, dxr);
		DXR::Create_Shader_Table(d3d, dxr, resources);

		d3d.cmdList->Close();
		ID3D12CommandList* pGraphicsList = { d3d.cmdList };
		d3d.cmdQueue->ExecuteCommandLists(1, &pGraphicsList);

		D3D12::WaitForGPU(d3d);
		D3D12::Reset_CommandList(d3d);
	}
	else
	{


	}
	
}

void JellyFishDev::Update()
{
	if (m_useRayTracing)
	{
		D3DResources::Update_View_CB(d3d, resources);
	}
	else
	{
		
	}

}

void JellyFishDev::Render()
{
	if (m_useRayTracing)
	{
		DXR::Build_Command_List(d3d, dxr, resources);
		D3D12::Present(d3d);
		D3D12::MoveToNextFrame(d3d);
		D3D12::Reset_CommandList(d3d);
	}
	else
	{

	}

}

void JellyFishDev::Cleanup()
{
	if (m_useRayTracing)
	{
		D3D12::WaitForGPU(d3d);
		CloseHandle(d3d.fenceEvent);

		DXR::Destroy(dxr);
		D3DResources::Destroy(resources);
		D3DShaders::Destroy(shaderCompiler);
		D3D12::Destroy(d3d);

		DestroyWindow(window);
	}
	else
	{

	}

}
