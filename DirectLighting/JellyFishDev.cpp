#include "JellyFishDev.h"

void JellyFishDev::Init(ConfigInfo& config)
{
	// Create a new window
	//HRESULT hr = Window::Create(config.width, config.height, config.instance, window, "Introduction to DirectX Raytracing (DXR)");
	//Utils::Validate(hr, "Error: failed to create window!");
	
	if (m_useRayTracing)
	{
		d3d.width = config.width;
		d3d.height = config.height;
		d3d.vsync = config.vsync;

		

	
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
