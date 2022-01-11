#pragma once

/*
#include <Windows.h>
#include "DXDefines.h"
#include "LWindow.h"
#include "Status.h"
#include "d3dx12.h"
#include "GraphicsData.h"
*/
#include <Windows.h>
#include <D3d12.h>
#include <D3d12SDKLayers.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")


#include "D3dx12.h"
#include "LWindow.h"

#include "GraphicsData.h"


//using namespace GData;
using namespace DirectX;
using namespace Microsoft::WRL;
struct Vertex
{
	XMFLOAT3 pos;
};
class Graphics
{
public:
	Graphics() = default;
	~Graphics() = default;

	bool OnInit(LWindow& _window);

	void UpdatePipeline();
	void Render();
	void WaitForPreviousFrame();
	void CleanUp();

	//Gets
	ID3D12Device* Device(){return m_pDevice;}
	IDXGISwapChain3* SwapChain(){return m_pSwapChain;}
	ID3D12CommandQueue* CommandQueue() {return m_pCommandQueue;}
	ID3D12DescriptorHeap* RTVDescriptorHeap() {return m_pRTVDescriptorHeap;}
	ID3D12Resource** RenderTargets(){return m_pRenderTargets;}
	ID3D12CommandAllocator** CommandAllocator(){return m_pCommandAllocator;}
	ID3D12GraphicsCommandList* CommandList(){ return m_pCommandList;}
	ID3D12Fence** Fence(){return m_pFence;}
	HANDLE FenceEvent(){return m_fenceEvent;}
	int FrameIndex(){return m_frameIndex;}
	UINT64* FenceValue(){return m_fenceValue;}
private:

	//Pipeline 
	bool InitDevice();
	bool InitCommandQueue();
	bool InitSwapchain(LWindow& _window);
	bool InitRenderTargets();
	bool InitCommandAllocators();
	bool InitCommandList();
	bool InitFence();

	//Drawing
	bool InitRootSignature();
	bool CompileMyShaders();
	bool CreateInputLayout();
	bool CreatePSO(PSOData& _psoData);
	bool CreateVertexBuffer();
	bool CreateIndexBuffer(int _vBufferSize, ID3D12Resource* _pVBufferUploadHeap);
  bool CreateDepthBuffer(LWindow& _window);

	//-------

	//For Setting Up The Pipeline
	static const int m_frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

	IDXGIFactory4* dxgiFactory = nullptr;

	ID3D12Device* m_pDevice = nullptr; // direct3d device

	IDXGISwapChain3* m_pSwapChain = nullptr; // swapchain used to switch between render targets

	ID3D12CommandQueue* m_pCommandQueue = nullptr; // container for command lists

	ID3D12DescriptorHeap* m_pRTVDescriptorHeap = nullptr; // a descriptor heap to hold resources like the render targets

	ID3D12Resource* m_pRenderTargets[m_frameBufferCount]; // number of render targets equal to buffer count

	ID3D12CommandAllocator* m_pCommandAllocator[m_frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

	ID3D12GraphicsCommandList* m_pCommandList = nullptr; // a command list we can record commands into, then execute them to render the frame

	ID3D12Fence* m_pFence[m_frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
																					 //as we have allocators (more if we want to know when the gpu is finished with an asset)

	HANDLE m_fenceEvent; // a handle to an event when our fence is unlocked by the gpu

	UINT64 m_fenceValue[m_frameBufferCount]; // this value is incremented each frame. each fence will have its own value

	int m_frameIndex; // current rtv we are on

	int m_rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

	//For Drawing
	PSOData m_psoData;
	ID3D12PipelineState* m_pPipelineStateObject; // pso containing a pipeline state

	ID3D12RootSignature* m_pRootSignature; // root signature defines data shaders will access

	D3D12_VIEWPORT m_viewport; // area that output from rasterizer will be stretched to.

	D3D12_RECT m_scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

	ID3D12Resource* m_pVertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
																						   // the total size of the buffer, and the size of each element (vertex)

	ID3D12Resource* m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView; 

	ID3D12Resource* m_pDepthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
	ID3D12DescriptorHeap* m_pDSDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor
	
};

