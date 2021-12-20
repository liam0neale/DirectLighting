#pragma once
#pragma once
#include "DXDefines.h"

namespace GData
{
	
	// direct3d stuff
	static const int m_frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering
	
	static IDXGIFactory4* dxgiFactory;

	static ID3D12Device* m_pDevice; // direct3d device

	static IDXGISwapChain3* m_pSwapChain; // swapchain used to switch between render targets

	static ID3D12CommandQueue* m_pCommandQueue; // container for command lists

	static ID3D12DescriptorHeap* m_pRTVDescriptorHeap; // a descriptor heap to hold resources like the render targets

	static ID3D12Resource* m_pRenderTargets[m_frameBufferCount]; // number of render targets equal to buffer count

	static ID3D12CommandAllocator* m_pCommandAllocator[m_frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

	static ID3D12GraphicsCommandList* m_pCommandList; // a command list we can record commands into, then execute them to render the frame

	static ID3D12Fence* m_pFence[m_frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
																					 //as we have allocators (more if we want to know when the gpu is finished with an asset)

	static HANDLE m_fenceEvent; // a handle to an event when our fence is unlocked by the gpu

	static UINT64 m_fenceValue[m_frameBufferCount]; // this value is incremented each frame. each fence will have its own value

	static int m_frameIndex; // current rtv we are on

	static int m_rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

}