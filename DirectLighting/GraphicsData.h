#pragma once
#pragma once
#include "DXDefines.h"

namespace GraphicsData
{
	// direct3d stuff
	const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering
	
	IDXGIFactory4* dxgiFactory;

	ID3D12Device* m_pDevice; // direct3d device

	IDXGISwapChain3* m_pSwapChain; // swapchain used to switch between render targets

	ID3D12CommandQueue* m_pCommandQueue; // container for command lists

	ID3D12DescriptorHeap* m_pRTVDescriptorHeap; // a descriptor heap to hold resources like the render targets

	ID3D12Resource* m_pRenderTargets[frameBufferCount]; // number of render targets equal to buffer count

	ID3D12CommandAllocator* m_pCommandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

	ID3D12GraphicsCommandList* m_pCommandList; // a command list we can record commands into, then execute them to render the frame

	ID3D12Fence* m_pFence[frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
																					 //as we have allocators (more if we want to know when the gpu is finished with an asset)

	HANDLE m_fenceEvent; // a handle to an event when our fence is unlocked by the gpu

	UINT64 m_fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value

	int m_frameIndex; // current rtv we are on

	int m_rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)

}