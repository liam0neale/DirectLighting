#pragma once
#include "DXDefines.h"
#include "GraphicsData.h"
#include "LWindow.h"
#include "Status.h"

//using namespace GData;
using namespace EngineStatus;

class Graphics
{
public:
	Graphics() = default;
	~Graphics() = default;

	bool OnInit(LWindow& _window);

	void UpdatePipeline();
	void WaitForPreviousFrame();
	void CleanUp();

	//Gets
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

	//Objects
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

	
};

