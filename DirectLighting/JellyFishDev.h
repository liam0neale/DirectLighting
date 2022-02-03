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
	bool CreatePerObjectConstantBuffer(D3D12Global& d3d, D3D12Resources& resources, Model& _model);

	void WaitForPreviousFrame();
	//TEMP @@@
	//Contant Buffer Raz
	int m_ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;
	// this is the constant buffer data we will send to the gpu 
																				 // (which will be placed in the resource we created above)
	ID3D12Resource* m_pConstantBufferUploadHeaps[3]; // this is the memory on the gpu where constant buffers for each frame will be placed
	UINT8* m_pCBVGPUAddress[3]; // this is a pointer to each of the constant buffer resource heaps

	bool InitRasterTest(LWindow* _window);
	void UpdatePipelineTest();
	void RenderTest();
	void WaitForPreviousFrameTest();
	
	
private:
	HWND window;
	

	DXRGlobal dxr = {};
	D3D12Global d3d = {};
	D3D12Resources resources = {};
	D3D12ShaderCompilerInfo shaderCompiler;
	RasterProgram rasterProgram;
	D3D12_VIEWPORT m_viewport; // area that output from rasterizer will be stretched to.
	D3D12_RECT m_scissorRect; // the area to draw in. pixels outside that area will not be drawn onto


	static const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

	//ID3D12Device5* device; // direct3d device

	//IDXGISwapChain3* swapChain; // swapchain used to switch between render targets

	//ID3D12CommandQueue* commandQueue; // container for command lists

	ID3D12DescriptorHeap* rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets

	ID3D12Resource* renderTargets[frameBufferCount]; // number of render targets equal to buffer count

	//ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

	//ID3D12GraphicsCommandList* commandList; // a command list we can record commands into, then execute them to render the frame

	ID3D12Fence* fence[frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
											 //as we have allocators (more if we want to know when the gpu is finished with an asset)

	HANDLE fenceEvent; // a handle to an event when our fence is unlocked by the gpu

	UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value

	//int frameIndex; // current rtv we are on

	int rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

	
	ID3D12PipelineState* pipelineStateObject; // pso containing a pipeline state

	ID3D12RootSignature* rootSignature; // root signature defines data shaders will access

	D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to.

	D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

	ID3D12Resource* vertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
												 // the total size of the buffer, and the size of each element (vertex)
};

