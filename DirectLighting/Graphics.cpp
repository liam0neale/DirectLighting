#include "Graphics.h"

bool Graphics::OnInit(LWindow &_window)
{
	HRESULT hr;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		return false;
	}

	bool setup = InitDevice() && InitCommandQueue() && InitSwapchain(_window) && InitRenderTargets() && InitCommandAllocators() && InitCommandList() && InitFence();
	
	return setup;
}

void Graphics::UpdatePipeline()
{
	HRESULT hr;
	
	// We have to wait for the gpu to finish with the command allocator before we reset it
	WaitForPreviousFrame();

	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	hr = m_pCommandAllocator[m_frameIndex]->Reset();
	if (FAILED(hr))
	{
		m_status = Status::sERRORED;
	}

	/*by resetting the command list we are putting it into
	 a recording state so we can start recording commands into the command allocator.
	 the command allocator that we reference here may have multiple command lists
	 associated with it, but only one can be recording at any time. Make sure
	 that any other command lists associated to this command allocator are in
	 the closed state (not recording).
	 Here you will pass an initial pipeline state object as the second parameter,
	 but in this tutorial we are only clearing the rtv, and do not actually need
	 anything but an initial default pipeline, which is what we get by setting
	 the second parameter to NULL*/
	hr = m_pCommandList->Reset(m_pCommandAllocator[m_frameIndex], NULL);
	if (FAILED(hr))
	{
		m_status = Status::sERRORED;
	}
	
	// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

	// set the render target for the output merger stage (the output of the pipeline)
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear the render target by using the ClearRenderTargetView command
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = m_pCommandList->Close();
	if (FAILED(hr))
	{
		m_status = Status::sERRORED;
	}
}

void Graphics::WaitForPreviousFrame()
{
	HRESULT hr;

	// swap the current rtv buffer index so we draw on the correct buffer
	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// if the current fence value is still less than "fenceValue", then we know the GPU has not finished executing
	// the command queue since it has not reached the "commandQueue->Signal(fence, fenceValue)" command
	if (m_pFence[m_frameIndex]->GetCompletedValue() < m_fenceValue[m_frameIndex])
	{
		// we have the fence create an event which is signaled once the fence's current value is "fenceValue"
		hr = m_pFence[m_frameIndex]->SetEventOnCompletion(m_fenceValue[m_frameIndex], m_fenceEvent);
		if (FAILED(hr))
		{
			m_status = Status::sERRORED;
		}

		// We will wait until the fence has triggered the event that it's current value has reached "fenceValue". once it's value
		// has reached "fenceValue", we know the command queue has finished executing
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	// increment fenceValue for next frame
	m_fenceValue[m_frameIndex]++;
}

void Graphics::CleanUp()
{
	// wait for the gpu to finish all frames
	for (int i = 0; i < m_frameBufferCount; ++i)
	{
		m_frameIndex = i;
		WaitForPreviousFrame();
	}

	// get swapchain out of full screen before exiting
	BOOL fs = false;
	if (m_pSwapChain->GetFullscreenState(&fs, NULL))
		m_pSwapChain->SetFullscreenState(false, NULL);

	m_pDevice->Release();
	m_pSwapChain->Release();
	m_pCommandQueue->Release();
	m_pRTVDescriptorHeap->Release();
	m_pCommandList->Release();

	for (int i = 0; i < m_frameBufferCount; ++i)
	{
		m_pRenderTargets[i]->Release();
		m_pCommandAllocator[i]->Release();
		m_pFence[i]->Release();
	};
}

bool Graphics::InitDevice()
{
	HRESULT hr;
	if (m_pDevice != nullptr)
		m_pDevice->Release();

	IDXGIAdapter1* adapter; // adapters are the graphics hardware
	int adapterIndex = 0; // we'll start looking for directx 12  compatible graphics devices starting at index 0
	bool adapterFound = false; // set this to true when a good one was found

	// find first hardware gpu that supports d3d 12
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			//dont want a software adapter
			adapterIndex++;
			continue;
		}
		
		// we want a device that is compatible with direct3d 12 
		// NOTE: call the create device with nullptr in last param to check if this hardware adpater supports directx12
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}
	if (!adapterFound)
	{
		return false;
	}

	// Create the device
	hr = D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&m_pDevice)
	);

	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

bool Graphics::InitCommandQueue()
{
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC cqDesc = {}; // use all defualt values

	hr = m_pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_pCommandQueue)); 
	if (FAILED(hr))
	{
		return false;
	}
	return true;
}

bool Graphics::InitSwapchain(LWindow& _window)
{
	HRESULT hr;

	DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
	backBufferDesc.Width = 1;//D12Core::getWidth(); // buffer width
	backBufferDesc.Height = 1;//D12Core::getHeight(); // buffer height
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each chanel)

	// describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = m_frameBufferCount; // number of buffers we have
	swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
	swapChainDesc.OutputWindow = _window.getWindow(); // handle to our window
	swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
	swapChainDesc.Windowed = !_window.isFullscreen(); // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

	IDXGISwapChain* tempSwapChain;

	hr = dxgiFactory->CreateSwapChain(
		m_pCommandQueue, // the queue will be flushed once the swap chain is created
		&swapChainDesc, // give it the swap chain description we created above
		&tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
	);

	if (FAILED(hr))
	{
		return false;
	}

	m_pSwapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	return true;
}

bool Graphics::InitRenderTargets()
{
	HRESULT hr;
	// describe an rtv descriptor heap and create
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = m_frameBufferCount; // number of descriptors for this heap.
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap

	// This heap will not be directly referenced by the shaders (not shader visible), as this will store the output from the pipeline
	// otherwise we would set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_pRTVDescriptorHeap));
	if (FAILED(hr))
	{
		return false;
	}

	// get the size of a descriptor in this heap (this is a rtv heap, so only rtv descriptors should be stored in it.
	// descriptor sizes may vary from device to device, which is why there is no set size and we must ask the 
	// device to give us the size. we will use this size to increment a descriptor handle offset
	m_rtvDescriptorSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// get a handle to the first descriptor in the descriptor heap. a handle is basically a pointer,
	// but we cannot literally use it like a c++ pointer.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	// Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
	for (int i = 0; i < m_frameBufferCount; i++)
	{
		// first we get the n'th buffer in the swap chain and store it in the n'th
		// position of our ID3D12Resource array
		hr = m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&m_pRenderTargets[i]));
		if (FAILED(hr))
		{
			return false;
		}

		// the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
		m_pDevice->CreateRenderTargetView(m_pRenderTargets[i], nullptr, rtvHandle);

		// we increment the rtv handle by the rtv descriptor size we got above
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}

	return true;
}

//command allocator is used to allocate memory on the GPU for the commands we want to execute 
bool Graphics::InitCommandAllocators()
{
	HRESULT hr;
	for (int i = 0; i < m_frameBufferCount; i++)
	{
		hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}
	return true;
}

//you will want as many command lists as you have threads recording commands
bool Graphics::InitCommandList()
{
	HRESULT hr;
	// create the command list with the first allocator
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator[0], NULL, IID_PPV_ARGS(&m_pCommandList));
	if (FAILED(hr))
	{
		return false;
	}

	// command lists are created in the recording state. the main loop will set it up for recording again so close it now
	m_pCommandList->Close();
	return true;
}

//only using a single thread, so we only need one fence event, but since we are tripple buffering, we have three fences, one for each frame buffer
bool Graphics::InitFence()
{
	HRESULT hr;
	// create the fences
	for (int i = 0; i < m_frameBufferCount; i++)
	{
		hr = m_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence[i]));
		if (FAILED(hr))
		{
			return false;
		}
		m_fenceValue[i] = 0; // set the initial fence value to 0
	}

	// create a handle to a fence event
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		return false;
	}

	return true;
}

