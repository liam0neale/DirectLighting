#include "Graphics.h"

bool Graphics::OnInit(LWindow &_window)
{
	HRESULT hr;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		return false;
	}

	// Fill out the Viewport
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = _window.getWidth();
	m_viewport.Height = _window.getHeight();
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = _window.getWidth();
	m_scissorRect.bottom = _window.getHeight();

	bool setup = InitDevice() && InitCommandQueue() && InitSwapchain(_window) && InitRenderTargets() && InitCommandAllocators() && InitCommandList() && InitFence();
	setup = CreateDepthBuffer(_window);
	if (isRayTracing)
	{
		InitShaderCompiler(shaderCompiler);
		CreateViewCB();
		CreateMateriaCB();
		LoadModel();
		CreateVertexBuffer(m_model);
		CreateIndexBuffer(m_model);
		
		CreateBottomLevelAS(m_dxr);
		CreateTopLevelAS(m_dxr);
		CreateDXROutPut(_window.getWidth(), _window.getHeight());
		CreateDescriptorHeap(m_dxr);
		CreateRayGenProgram(m_dxr);
		CreateMissProgram(m_dxr, shaderCompiler);
		CreateClosetHitProgram(m_dxr, shaderCompiler);
		CreateRayPSO(m_dxr);
		CreateShaderTable(m_dxr);
	}
	else
	{
		setup = InitRootSignature();
		
		setup = CreatePerObjectConstantBuffer();

		setup = CreatePSO(m_psoData);
		//setup = Test(_window.getWidth(), _window.getHeight(), _window.getWindow());
		setup = CreateTexture();
	}
	
	
	//setup = InitRootSignature() && CompileMyShaders() && CreateInputLayout()   CreateConstantBuffer();
	if (setup)
	{
		
		// Now we execute the command list to upload the initial assets (triangle data)
		m_pCommandList->Close();
		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
		m_fenceValue[m_frameIndex]++;
		hr = m_pCommandQueue->Signal(m_pFence[m_frameIndex], m_fenceValue[m_frameIndex]);
		if (FAILED(hr))
		{
			return false;
		}
	}
	setup = InitScene(_window.getWidth(), _window.getHeight());
		


	return setup;
}

void Graphics::Update()
{
	if (isRayTracing)
	{

	}
	else
	{
		// update app logic, such as moving the camera or figuring out what objects are in view

		// create rotation matrices
		XMMATRIX rotXMat = XMMatrixRotationX(0.0001f);
		XMMATRIX rotYMat = XMMatrixRotationY(0.0002f);
		XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f);

		// add rotation to cube1's rotation matrix and store it
		XMMATRIX rotMat = XMLoadFloat4x4(&m_cube1RotMat) * rotXMat * rotYMat * rotZMat;
		XMStoreFloat4x4(&m_cube1RotMat, rotMat);

		// create translation matrix for cube 1 from cube 1's position vector
		XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&m_cube1Position));

		// create cube1's world matrix by first rotating the cube, then positioning the rotated cube
		XMMATRIX worldMat = rotMat * translationMat;

		// store cube1's world matrix
		XMStoreFloat4x4(&m_cube1WorldMat, worldMat);

		// update constant buffer for cube1
		// create the wvp matrix and store in constant buffer
		XMMATRIX viewMat = XMLoadFloat4x4(&m_cameraViewMat); // load view matrix
		XMMATRIX projMat = XMLoadFloat4x4(&m_cameraProjMat); // load projection matrix
		XMMATRIX wvpMat = XMLoadFloat4x4(&m_cube1WorldMat) * viewMat * projMat; // create wvp matrix
		XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
		XMStoreFloat4x4(&m_cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

		// copy our ConstantBuffer instance to the mapped constant buffer resource
		memcpy(m_pCBVGPUAddress[m_frameIndex], &m_cbPerObject, sizeof(m_cbPerObject));

		// now do cube2's world matrix
		// create rotation matrices for cube2
		rotXMat = XMMatrixRotationX(0.0003f);
		rotYMat = XMMatrixRotationY(0.0002f);
		rotZMat = XMMatrixRotationZ(0.0001f);

		// add rotation to cube2's rotation matrix and store it
		rotMat = rotZMat * (XMLoadFloat4x4(&m_cube2RotMat) * (rotXMat * rotYMat));
		XMStoreFloat4x4(&m_cube2RotMat, rotMat);

		// create translation matrix for cube 2 to offset it from cube 1 (its position relative to cube1
		XMMATRIX translationOffsetMat = XMMatrixTranslationFromVector(XMLoadFloat4(&m_cube2PositionOffset));

		// we want cube 2 to be half the size of cube 1, so we scale it by .5 in all dimensions
		XMMATRIX scaleMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);

		// reuse worldMat. 
		// first we scale cube2. scaling happens relative to point 0,0,0, so you will almost always want to scale first
		// then we translate it. 
		// then we rotate it. rotation always rotates around point 0,0,0
		// finally we move it to cube 1's position, which will cause it to rotate around cube 1
		worldMat = scaleMat * translationOffsetMat * rotMat * translationMat;

		wvpMat = XMLoadFloat4x4(&m_cube2WorldMat) * viewMat * projMat; // create wvp matrix
		transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
		XMStoreFloat4x4(&m_cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

		// copy our ConstantBuffer instance to the mapped constant buffer resource
		memcpy(m_pCBVGPUAddress[m_frameIndex] + m_ConstantBufferPerObjectAlignedSize, &m_cbPerObject, sizeof(m_cbPerObject));

		// store cube2's world matrix
		XMStoreFloat4x4(&m_cube2WorldMat, worldMat);
	}
    
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
		return;//Running = false;
	}

	// reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// the command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter,
	// but in this tutorial we are only clearing the rtv, and do not actually need
	// anything but an initial default pipeline, which is what we get by setting
	// the second parameter to NULL
	hr = m_pCommandList->Reset(m_pCommandAllocator[m_frameIndex], m_pPipelineStateObject);
	if (FAILED(hr))
	{
		return;//Running = false;
	}

	// here we start recording commands into the commandList (which all the commands will be stored in the commandAllocator)

// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

	// get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// set the render target for the output merger stage (the output of the pipeline)
	m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	if (isRayTracing)
	{
		// Clear the render target by using the ClearRenderTargetView command
		const float clearColor[] = { 0.7f, 0.2f, 0.4f, 1.0f };
		m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	}
	else
	{
		// Clear the render target by using the ClearRenderTargetView command
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		m_pCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		// clear the depth/stencil buffer
		m_pCommandList->ClearDepthStencilView(m_pDSDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		// set root signature
		m_pCommandList->SetGraphicsRootSignature(m_pRootSignature); // set the root signature

		// set the descriptor heap
		ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap };
		m_pCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		// set the descriptor table to the descriptor heap (parameter 1, as constant buffer root descriptor is parameter index 0)
		m_pCommandList->SetGraphicsRootDescriptorTable(1, mainDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		// draw triangle
		m_pCommandList->RSSetViewports(1, &m_viewport); // set the viewports
		m_pCommandList->RSSetScissorRects(1, &m_scissorRect); // set the scissor rects
		m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // set the primitive topology
		m_pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView); // set the vertex buffer (using the vertex buffer view)
		m_pCommandList->IASetIndexBuffer(&m_indexBufferView);

		// first cube

		// set cube1's constant buffer
		m_pCommandList->SetGraphicsRootConstantBufferView(0, m_pConstantBufferUploadHeaps[m_frameIndex]->GetGPUVirtualAddress());

		// draw first cube
		m_pCommandList->DrawIndexedInstanced(m_numCubeIndices, 1, 0, 0, 0);

		// second cube

		// set cube2's constant buffer. You can see we are adding the size of ConstantBufferPerObject to the constant buffer
		// resource heaps address. This is because cube1's constant buffer is stored at the beginning of the resource heap, while
		// cube2's constant buffer data is stored after (256 bits from the start of the heap).
		m_pCommandList->SetGraphicsRootConstantBufferView(0, m_pConstantBufferUploadHeaps[m_frameIndex]->GetGPUVirtualAddress() + m_ConstantBufferPerObjectAlignedSize);

		// draw second cube
		m_pCommandList->DrawIndexedInstanced(m_numCubeIndices, 1, 0, 0, 0);
	}
	

	// transition the "frameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = m_pCommandList->Close();
	if (FAILED(hr))
	{
		return;
	}
}

void Graphics::Render()
{
	UpdatePipeline(); // update the pipeline by sending commands to the commandqueue

	// create an array of command lists (only one command list here)
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList };

	// execute the array of command lists
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// this command goes in at the end of our command queue. we will know when our command queue 
	// has finished because the fence value will be set to "fenceValue" from the GPU since the command
	// queue is being executed on the GPU
	HRESULT hr = m_pCommandQueue->Signal(m_pFence[m_frameIndex], m_fenceValue[m_frameIndex]);
	if (FAILED(hr))
	{
		return;
	}

	// present the current backbuffer
	hr = m_pSwapChain->Present(0, 0);
	if (FAILED(hr))
	{
		return;
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
			return;
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

	m_pPipelineStateObject->Release();
	m_pRootSignature->Release();
	m_pVertexBuffer->Release();
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
		ID3D12Device* pDevice;
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice));

		D3D12_FEATURE_DATA_D3D12_OPTIONS5 features5 = {};
		HRESULT hr = pDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
		if (SUCCEEDED(hr) && features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
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
	backBufferDesc.Width = _window.getWidth(); // buffer width
	backBufferDesc.Height = _window.getHeight(); // buffer height
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

	dxgiFactory->CreateSwapChain(
		m_pCommandQueue, // the queue will be flushed once the swap chain is created
		&swapChainDesc, // give it the swap chain description we created above
		&tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
	);

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


bool Graphics::InitRootSignature()
{
	// create a root descriptor, which explains where to find the data for this root parameter
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// create a descriptor range (descriptor table) and fill it out
	// this is a range of descriptors inside a descriptor heap
	D3D12_DESCRIPTOR_RANGE  descriptorTableRanges[1]; // only one range right now
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // this is a range of shader resource views (descriptors)
	descriptorTableRanges[0].NumDescriptors = 1; // we only have one texture right now, so the range is only 1
	descriptorTableRanges[0].BaseShaderRegister = 0; // start index of the shader registers in the range
	descriptorTableRanges[0].RegisterSpace = 0; // space 0. can usually be zero
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // this appends the range to the end of the root signature descriptor tables

	// create a descriptor table
	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges); // we only have one range
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0]; // the pointer to the beginning of our ranges array

	// create a root parameter for the root descriptor and fill it out
	D3D12_ROOT_PARAMETER  rootParameters[2]; // only one parameter right now
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	// fill out the parameter for our descriptor table. Remember it's a good idea to sort parameters by frequency of change. Our constant
	// buffer will be changed multiple times per frame, while our descriptor table will not be changed at all (in this tutorial)
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // this is a descriptor table
	rootParameters[1].DescriptorTable = descriptorTable; // this is our descriptor table for this root parameter
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // our pixel shader will be the only shader accessing this parameter for now

	// create a static sampler
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), // we have 2 root parameters
		rootParameters, // a pointer to the beginning of our root parameters array
		1, // we have one static sampler
		&sampler, // a pointer to our static sampler (array)
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

	ID3DBlob* errorBuff; // a buffer holding the error data if any
	ID3DBlob* signature;
	HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errorBuff);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	if (FAILED(hr))
	{
		return false;
	}
	return true;
}


bool Graphics::CreatePSO(PSOData& _psoData)
{

	// create root signature
	HRESULT hr;
	//CreateConstantBuffer();

	// create vertex and pixel shaders

	// when debugging, we can compile the shader files at runtime.
	// but for release versions, we can compile the hlsl shaders
	// with fxc.exe to create .cso files, which contain the shader
	// bytecode. We can load the .cso files at runtime to get the
	// shader bytecode, which of course is faster than compiling
	// them at runtime

	// compile vertex shader
	ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
	ID3DBlob* errorBuff; // a buffer holding the error data if any
	hr = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vertexShader,
		&errorBuff);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// fill out a shader bytecode structure, which is basically just a pointer
	// to the shader bytecode and the size of the shader bytecode
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* pixelShader;
	hr = D3DCompileFromFile(L"PixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pixelShader,
		&errorBuff);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	// create input layout

	// The input layout is used by the Input Assembler so that it knows
	// how to read the vertex data bound to it.


	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// create a pipeline state object (PSO)

	// In a real application, you will have many pso's. for each different shader
	// or different combinations of shaders, different blend states or different rasterizer states,
	// different topology types (point, line, triangle, patch), or a different number
	// of render targets you will need a pso

	// VS is the only required shader for a pso. You might be wondering when a case would be where
	// you only set the VS. It's possible that you have a pso that only outputs data with the stream
	// output, and not on a render target, which means you would not need anything after the stream
	// output.

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
	psoDesc.InputLayout = inputLayoutDesc; // the structure describing our input layout
	psoDesc.pRootSignature = m_pRootSignature; // the root signature that describes the input data this pso needs
	psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
	psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
	psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
	psoDesc.SampleMask = 0xffffffff; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blent state.
	psoDesc.NumRenderTargets = 1; // we are only binding one render target
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state
	// create the pso
	hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineStateObject));
	if (FAILED(hr))
	{
		return false;
	}
	
	Vertex vList[] = {
		// front face
		Vertex({ -0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}),
		Vertex({  0.5f, -0.5f, -0.5f}, {1.0f, 1.0f }),
		Vertex({ -0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f }),
		Vertex({  0.5f,  0.5f, -0.5f}, { 1.0f, 0.0f}),

		// right side face
		Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f }),
		Vertex({  0.5f,  0.5f,  0.5f}, { 1.0f, 0.0 }),
		Vertex({  0.5f, -0.5f,  0.5f}, { 1.0f, 1.0f }),
		Vertex({  0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f }),

		// left side face
		Vertex({ -0.5f,  0.5f,  0.5f}, {  0.0f, 0.0f}),
		Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f}),
		Vertex({ -0.5f, -0.5f,  0.5f}, { 0.0f, 1.0f }),
		Vertex({ -0.5f,  0.5f, -0.5f}, {  1.0f, 0.0f }),

		// back face
		Vertex({  0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f }),
		Vertex({ -0.5f, -0.5f,  0.5f}, { 1.0f, 1.0f }),
		Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 1.0f }),
		Vertex({ -0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f }),

		// top face
		Vertex({ -0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f }),
		Vertex({ 0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f }),
		Vertex({ 0.5f,  0.5f, -0.5f}, {  1.0f, 1.0f }),
		Vertex({ -0.5f,  0.5f,  0.5f}, { 0.0f,0.0f }),

		// bottom face
		Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f}),
		Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f }),
		Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f,  1.0f }),
		Vertex({ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f })
	};
	for (int i = 0; i < ARRAYSIZE(vList); i++)
	{
		m_model.vertices.push_back(vList[i]);
	}
	int vBufferSize = sizeof(vList);

	// create default heap
	// default heap is memory on the GPU. Only the GPU has access to this memory
	// To get data into this heap, we will have to upload the data using
	// an upload heap
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
																		// from the upload heap to this heap
		nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&m_pVertexBuffer));

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_pVertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// create upload heap
	// upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
	// We will upload the vertex buffer using this heap to the default heap
	ID3D12Resource* vBufferUploadHeap;
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));
	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList); // pointer to our vertex array
	vertexData.RowPitch = vBufferSize; // size of all our triangle vertex data
	vertexData.SlicePitch = vBufferSize; // also the size of our triangle vertex data

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources(m_pCommandList, m_pVertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

	
	if(!CreateIndexBuffer(vBufferSize, vBufferUploadHeap))
		return false;

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));


	// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
	//m_fenceValue[m_frameIndex]++;
	//hr = m_pCommandQueue->Signal(m_pFence[m_frameIndex], m_fenceValue[m_frameIndex]);
	if (FAILED(hr))
	{
		//Running = false;
	}

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vBufferSize;
	return true;
}

bool Graphics::CreateVertexBuffer()
{
	// a triangle
	

	return true;
}

bool Graphics::CreateIndexBuffer(int _vBufferSize, ID3D12Resource* _pVBufferUploadHeap)
{
	DWORD iList[] = {
		// ffront face
		0, 1, 2, // first triangle
		0, 3, 1, // second triangle

		// left face
		4, 5, 6, // first triangle
		4, 7, 5, // second triangle

		// right face
		8, 9, 10, // first triangle
		8, 11, 9, // second triangle

		// back face
		12, 13, 14, // first triangle
		12, 15, 13, // second triangle

		// top face
		16, 17, 18, // first triangle
		16, 19, 17, // second triangle

		// bottom face
		20, 21, 22, // first triangle
		20, 23, 21, // second triangle
	};
	for (int i = 0; i < ARRAYSIZE(iList); i++)
	{
		m_model.indices.push_back(iList[i]);
	}
	int iBufferSize = sizeof(iList);

	m_numCubeIndices = sizeof(iList) / sizeof(DWORD);


	// create default heap to hold index buffer
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
		nullptr, // optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&m_pIndexBuffer));

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_pIndexBuffer->SetName(L"Index Buffer Resource Heap");

	ID3D12Resource* iBufferUploadHeap;
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(_vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));
	_pVBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
	indexData.RowPitch = iBufferSize; // size of all our index buffer
	indexData.SlicePitch = iBufferSize; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources(m_pCommandList, m_pIndexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	m_indexBufferView.SizeInBytes = iBufferSize;
	return true;
}

bool Graphics::CreateDepthBuffer(LWindow& _window)
{
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSDescriptorHeap));
	if (FAILED(hr))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;
	
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, _window.getWidth(), _window.getHeight(), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_pDepthStencilBuffer)
	);
	m_pDSDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilDesc, m_pDSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool Graphics::CreatePerObjectConstantBuffer()
{
	HRESULT hr;
	for (int i = 0; i < m_frameBufferCount; ++i)
	{
		// create resource for cube 1
		hr = m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&m_pConstantBufferUploadHeaps[i]));
		m_pConstantBufferUploadHeaps[i]->SetName(L"Constant Buffer Upload Resource Heap");

		ZeroMemory(&m_cbPerObject, sizeof(m_cbPerObject));

		CD3DX12_RANGE readRange(0, 0);    // We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)

		// map the resource heap to get a gpu virtual address to the beginning of the heap
		hr = m_pConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_pCBVGPUAddress[i]));

		// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
		// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
		memcpy(m_pCBVGPUAddress[i], &m_cbPerObject, sizeof(m_cbPerObject)); // cube1's constant buffer data
		memcpy(m_pCBVGPUAddress[i] + m_ConstantBufferPerObjectAlignedSize, &m_cbPerObject, sizeof(m_cbPerObject)); // cube2's constant buffer data
	}
	return true;
}

bool Graphics::InitScene(int _width, int _height)
{
	// build projection and view matrix
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)_width / (float)_height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&m_cameraProjMat, tmpMat);

	// set starting camera state
	m_cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
	m_cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// build view matrix
	XMVECTOR cPos = XMLoadFloat4(&m_cameraPosition);
	XMVECTOR cTarg = XMLoadFloat4(&m_cameraTarget);
	XMVECTOR cUp = XMLoadFloat4(&m_cameraUp);
	tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&m_cameraViewMat, tmpMat);

	// set starting cubes position
	// first cube
	m_cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
	XMVECTOR posVec = XMLoadFloat4(&m_cube1Position); // create xmvector for cube1's position

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
	XMStoreFloat4x4(&m_cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
	XMStoreFloat4x4(&m_cube1WorldMat, tmpMat); // store cube1's world matrix

	// second cube
	m_cube2PositionOffset = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
	posVec = XMLoadFloat4(&m_cube2PositionOffset) + XMLoadFloat4(&m_cube1Position); // create xmvector for cube2's position
																																							// we are rotating around cube1 here, so add cube2's position to cube1

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
	XMStoreFloat4x4(&m_cube2RotMat, XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
	XMStoreFloat4x4(&m_cube2WorldMat, tmpMat); // store cube2's world matrix
	return true;
}

bool Graphics::Test(int _width, int _height, HWND _hwnd)
{
 return false;
	/*
	HRESULT hr;	

	// -- Create the Device -- //

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		return false;
	}

	IDXGIAdapter1* adapter; // adapters are the graphics card (this includes the embedded graphics on the motherboard)

	int adapterIndex = 0; // we'll start looking for directx 12  compatible graphics devices starting at index 0

	bool adapterFound = false; // set this to true when a good one was found

								 // find first hardware gpu that supports d3d 12
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// we dont want a software device
			continue;
		}

		// we want a device that is compatible with direct3d 12 (feature level 11 or higher)
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

	// -- Create a direct command queue -- //

	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT; // direct means the gpu can directly execute this command queue

	hr = m_pDevice->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_pCommandQueue)); // create the command queue
	if (FAILED(hr))
	{
		return false;
	}

	// -- Create the Swap Chain (double/tripple buffering) -- //

	DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
	backBufferDesc.Width = _width; // buffer width
	backBufferDesc.Height = _height; // buffer height
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
	swapChainDesc.OutputWindow = _hwnd; // handle to our window
	swapChainDesc.SampleDesc = sampleDesc; // our multi-sampling description
	swapChainDesc.Windowed = true; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

	IDXGISwapChain* tempSwapChain;

	dxgiFactory->CreateSwapChain(
		m_pCommandQueue, // the queue will be flushed once the swap chain is created
		&swapChainDesc, // give it the swap chain description we created above
		&tempSwapChain // store the created swap chain in a temp IDXGISwapChain interface
	);

	m_pSwapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	m_frameIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// -- Create the Back Buffers (render target views) Descriptor Heap -- //

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

	// -- Create the Command Allocators -- //

	for (int i = 0; i < m_frameBufferCount; i++)
	{
		hr = m_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// -- Create a Command List -- //

	// create the command list with the first allocator
	hr = m_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator[m_frameIndex], NULL, IID_PPV_ARGS(&m_pCommandList));
	if (FAILED(hr))
	{
		return false;
	}

	// -- Create a Fence & Fence Event -- //

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


	// create root signature

	// create a root descriptor, which explains where to find the data for this root parameter
	D3D12_ROOT_DESCRIPTOR rootCBVDescriptor;
	rootCBVDescriptor.RegisterSpace = 0;
	rootCBVDescriptor.ShaderRegister = 0;

	// create a root parameter and fill it out
	D3D12_ROOT_PARAMETER  rootParameters[1]; // only one parameter right now
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; // this is a constant buffer view root descriptor
	rootParameters[0].Descriptor = rootCBVDescriptor; // this is the root descriptor for this root parameter
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // our pixel shader will be the only shader accessing this parameter for now

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), // we have 1 root parameter
		rootParameters, // a pointer to the beginning of our root parameters array
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);

	ID3DBlob* signature;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	// create vertex and pixel shaders

	// when debugging, we can compile the shader files at runtime.
	// but for release versions, we can compile the hlsl shaders
	// with fxc.exe to create .cso files, which contain the shader
	// bytecode. We can load the .cso files at runtime to get the
	// shader bytecode, which of course is faster than compiling
	// them at runtime

	// compile vertex shader
	ID3DBlob* vertexShader; // d3d blob for holding vertex shader bytecode
	ID3DBlob* errorBuff; // a buffer holding the error data if any
	hr = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&vertexShader,
		&errorBuff);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// fill out a shader bytecode structure, which is basically just a pointer
	// to the shader bytecode and the size of the shader bytecode
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* pixelShader;
	hr = D3DCompileFromFile(L"PixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&pixelShader,
		&errorBuff);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuff->GetBufferPointer());
		return false;
	}

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	// create input layout

	// The input layout is used by the Input Assembler so that it knows
	// how to read the vertex data bound to it.

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};

	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	// create a pipeline state object (PSO)

	// In a real application, you will have many pso's. for each different shader
	// or different combinations of shaders, different blend states or different rasterizer states,
	// different topology types (point, line, triangle, patch), or a different number
	// of render targets you will need a pso

	// VS is the only required shader for a pso. You might be wondering when a case would be where
	// you only set the VS. It's possible that you have a pso that only outputs data with the stream
	// output, and not on a render target, which means you would not need anything after the stream
	// output.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {}; // a structure to define a pso
	psoDesc.InputLayout = inputLayoutDesc; // the structure describing our input layout
	psoDesc.pRootSignature = m_pRootSignature; // the root signature that describes the input data this pso needs
	psoDesc.VS = vertexShaderBytecode; // structure describing where to find the vertex shader bytecode and how large it is
	psoDesc.PS = pixelShaderBytecode; // same as VS but for pixel shader
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // type of topology we are drawing
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the render target
	psoDesc.SampleDesc = sampleDesc; // must be the same sample description as the swapchain and depth/stencil buffer
	psoDesc.SampleMask = 0xffffffff; // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // a default rasterizer state.
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT); // a default blent state.
	psoDesc.NumRenderTargets = 1; // we are only binding one render target
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT); // a default depth stencil state

	// create the pso
	hr = m_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineStateObject));
	if (FAILED(hr))
	{
		return false;
	}

	// Create vertex buffer

	/*
	// a quad
	Vertex vList[] = {
		// front face
	Vertex({ -0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f }),
	Vertex({  0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ -0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({  0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f, 1.0f }),

	// right side face
	Vertex({  0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f }),
	Vertex({  0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({  0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f, 1.0f }),

	// left side face
	Vertex({ -0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f }),
	Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ -0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ -0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f, 0.0f, 1.0f }),

	// back face
	Vertex({  0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f }),
	Vertex({ -0.5f, -0.5f,  0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ -0.5f,  0.5f,  0.5f}, { 0.0f, 1.0f, 0.0f, 1.0f }),

	// top face
	Vertex({ -0.5f,  0.5f, -0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f }),
	Vertex({ 0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ 0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ -0.5f,  0.5f,  0.5f}, { 0.0f, 1.0f, 0.0f, 1.0f }),

	// bottom face
	Vertex({  0.5f, -0.5f,  0.5f}, { 1.0f, 0.0f, 0.0f, 1.0f }),
	Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f, 0.0f, 1.0f, 1.0f }),
	Vertex({ -0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f, 1.0f })
	};

	int vBufferSize =0;// sizeof(vList);

	// create default heap
	// default heap is memory on the GPU. Only the GPU has access to this memory
	// To get data into this heap, we will have to upload the data using
	// an upload heap
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data
										// from the upload heap to this heap
		nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&m_pVertexBuffer));

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_pVertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// create upload heap
	// upload heaps are used to upload data to the GPU. CPU can write to it, GPU can read from it
	// We will upload the vertex buffer using this heap to the default heap
	ID3D12Resource* vBufferUploadHeap;
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));
	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList); // pointer to our vertex array
	vertexData.RowPitch = vBufferSize; // size of all our triangle vertex data
	vertexData.SlicePitch = vBufferSize; // also the size of our triangle vertex data

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources(m_pCommandList, m_pVertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pVertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Create index buffer

	// a quad (2 triangles)
	DWORD iList[] = {
		// ffront face
		0, 1, 2, // first triangle
		0, 3, 1, // second triangle

		// left face
		4, 5, 6, // first triangle
		4, 7, 5, // second triangle

		// right face
		8, 9, 10, // first triangle
		8, 11, 9, // second triangle

		// back face
		12, 13, 14, // first triangle
		12, 15, 13, // second triangle

		// top face
		16, 17, 18, // first triangle
		16, 19, 17, // second triangle

		// bottom face
		20, 21, 22, // first triangle
		20, 23, 21, // second triangle
	};

	int iBufferSize = sizeof(iList);

	m_numCubeIndices = sizeof(iList) / sizeof(DWORD);

	// create default heap to hold index buffer
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(iBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_COPY_DEST, // start in the copy destination state
		nullptr, // optimized clear value must be null for this type of resource
		IID_PPV_ARGS(&m_pIndexBuffer));

	// we can give resource heaps a name so when we debug with the graphics debugger we know what resource we are looking at
	m_pVertexBuffer->SetName(L"Index Buffer Resource Heap");

	// create upload heap to upload index buffer
	ID3D12Resource* iBufferUploadHeap;
	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap));
	vBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList); // pointer to our index array
	indexData.RowPitch = iBufferSize; // size of all our index buffer
	indexData.SlicePitch = iBufferSize; // also the size of our index buffer

	// we are now creating a command with the command list to copy the data from
	// the upload heap to the default heap
	UpdateSubresources(m_pCommandList, m_pIndexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	// Create the depth/stencil buffer

	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_pDSDescriptorHeap));
	if (FAILED(hr))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, _width, _height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_pDepthStencilBuffer)
	);
	m_pDSDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

	m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilDesc, m_pDSDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// create the constant buffer resource heap
	// We will update the constant buffer one or more times per frame, so we will use only an upload heap
	// unlike previously we used an upload heap to upload the vertex and index data, and then copied over
	// to a default heap. If you plan to use a resource for more than a couple frames, it is usually more
	// efficient to copy to a default heap where it stays on the gpu. In this case, our constant buffer
	// will be modified and uploaded at least once per frame, so we only use an upload heap

	// first we will create a resource heap (upload heap) for each frame for the cubes constant buffers
	// As you can see, we are allocating 64KB for each resource we create. Buffer resource heaps must be
	// an alignment of 64KB. We are creating 3 resources, one for each frame. Each constant buffer is 
	// only a 4x4 matrix of floats in this tutorial. So with a float being 4 bytes, we have 
	// 16 floats in one constant buffer, and we will store 2 constant buffers in each
	// heap, one for each cube, thats only 64x2 bits, or 128 bits we are using for each
	// resource, and each resource must be at least 64KB (65536 bits)
	for (int i = 0; i < m_frameBufferCount; ++i)
	{
		// create resource for cube 1
		hr = m_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // this heap will be used to upload the constant buffer data
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // size of the resource heap. Must be a multiple of 64KB for single-textures and constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ, // will be data that is read from so we keep it in the generic read state
			nullptr, // we do not have use an optimized clear value for constant buffers
			IID_PPV_ARGS(&m_pConstantBufferUploadHeaps[i]));
		m_pConstantBufferUploadHeaps[i]->SetName(L"Constant Buffer Upload Resource Heap");

		ZeroMemory(&m_cbPerObject, sizeof(m_cbPerObject));

		CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)

		// map the resource heap to get a gpu virtual address to the beginning of the heap
		hr = m_pConstantBufferUploadHeaps[i]->Map(0, &readRange, reinterpret_cast<void**>(&m_pCBVGPUAddress[i]));

		// Because of the constant read alignment requirements, constant buffer views must be 256 bit aligned. Our buffers are smaller than 256 bits,
		// so we need to add spacing between the two buffers, so that the second buffer starts at 256 bits from the beginning of the resource heap.
		memcpy(m_pCBVGPUAddress[i], &m_cbPerObject, sizeof(m_cbPerObject)); // cube1's constant buffer data
		memcpy(m_pCBVGPUAddress[i] + m_ConstantBufferPerObjectAlignedSize, &m_cbPerObject, sizeof(m_cbPerObject)); // cube2's constant buffer data
	}

	// Now we execute the command list to upload the initial assets (triangle data)
	m_pCommandList->Close();
	ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// increment the fence value now, otherwise the buffer might not be uploaded by the time we start drawing
	m_fenceValue[m_frameIndex]++;
	hr = m_pCommandQueue->Signal(m_pFence[m_frameIndex], m_fenceValue[m_frameIndex]);
	if (FAILED(hr))
	{
		return false;
	}

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vBufferSize;

	// create a vertex buffer view for the triangle. We get the GPU memory address to the vertex pointer using the GetGPUVirtualAddress() method
	m_indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT; // 32-bit unsigned integer (this is what a dword is, double word, a word is 2 bytes)
	m_indexBufferView.SizeInBytes = iBufferSize;

	// Fill out the Viewport
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = _width;
	m_viewport.Height = _height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = _width;
	m_scissorRect.bottom = _height;

	// build projection and view matrix
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)_width / (float)_height, 0.1f, 1000.0f);
	XMStoreFloat4x4(&m_cameraProjMat, tmpMat);

	// set starting camera state
	m_cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
	m_cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// build view matrix
	XMVECTOR cPos = XMLoadFloat4(&m_cameraPosition);
	XMVECTOR cTarg = XMLoadFloat4(&m_cameraTarget);
	XMVECTOR cUp = XMLoadFloat4(&m_cameraUp);
	tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&m_cameraViewMat, tmpMat);

	// set starting cubes position
	// first cube
	m_cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
	XMVECTOR posVec = XMLoadFloat4(&m_cube1Position); // create xmvector for cube1's position

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
	XMStoreFloat4x4(&m_cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
	XMStoreFloat4x4(&m_cube1WorldMat, tmpMat); // store cube1's world matrix

	// second cube
	m_cube2PositionOffset = XMFLOAT4(1.5f, 0.0f, 0.0f, 0.0f);
	posVec = XMLoadFloat4(&m_cube2PositionOffset) + XMLoadFloat4(&m_cube1Position); // create xmvector for cube2's position
																				// we are rotating around cube1 here, so add cube2's position to cube1

	tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube2's position offset vector
	XMStoreFloat4x4(&m_cube2RotMat, XMMatrixIdentity()); // initialize cube2's rotation matrix to identity matrix
	XMStoreFloat4x4(&m_cube2WorldMat, tmpMat); // store cube2's world matrix
	return true;*/
}

bool Graphics::CreateTexture()
{
	// create the descriptor heap that will store our srv
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HRESULT hr = m_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap));
	if (FAILED(hr))
	{
		return false;
	}
	// Load the image from file
	BYTE* imageData;
	D3D12_RESOURCE_DESC textureDesc;
	int imageBytesPerRow;
	int imageSize = LoadImageDataFromFile(&imageData, textureDesc, L"braynzar.jpg", imageBytesPerRow);

	// make sure we have data
	if (imageSize <= 0)
	{
		return false;
	}

	// create a default heap where the upload heap will copy its contents into (contents being the texture)
  hr = m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&textureDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&textureBuffer));
	if (FAILED(hr))
	{
		return false;
	}
	textureBuffer->SetName(L"Texture Buffer Resource Heap");
	
	UINT64 textureUploadBufferSize;
	// this function gets the size an upload buffer needs to be to upload a texture to the gpu.
	// each row must be 256 byte aligned except for the last row, which can just be the size in bytes of the row
	// eg. textureUploadBufferSize = ((((width * numBytesPerPixel) + 255) & ~255) * (height - 1)) + (width * numBytesPerPixel);
	//textureUploadBufferSize = (((imageBytesPerRow + 255) & ~255) * (textureDesc.Height - 1)) + imageBytesPerRow;
	m_pDevice->GetCopyableFootprints(&textureDesc, 0, 1, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// now we create an upload heap to upload our texture to the GPU
	hr = m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&textureBufferUploadHeap));
	if (FAILED(hr))
	{
		return false;
	}
	textureBufferUploadHeap->SetName(L"Texture Buffer Upload Resource Heap");

	// store vertex buffer in upload heap
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &imageData[0]; // pointer to our image data
	textureData.RowPitch = imageBytesPerRow; // size of all our triangle vertex data
	textureData.SlicePitch = imageBytesPerRow * textureDesc.Height; // also the size of our triangle vertex data

	// Now we copy the upload buffer contents to the default heap
	UpdateSubresources(m_pCommandList, textureBuffer, textureBufferUploadHeap, 0, 0, 1, &textureData);

	// transition the texture default heap to a pixel shader resource (we will be sampling from this heap in the pixel shader to get the color of pixels)
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

	// now we create a shader resource view (descriptor that points to the texture and describes it)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_pDevice->CreateShaderResourceView(textureBuffer, &srvDesc, mainDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	return true;
}

int Graphics::LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow)
{
	HRESULT hr;

	// need one instance of the imaging factory to create decoders and frames
	static IWICImagingFactory* wicFactory;

	// reset decoder, frame and converter since these will be different for each image we load
	IWICBitmapDecoder* wicDecoder = NULL;
	IWICBitmapFrameDecode* wicFrame = NULL;
	IWICFormatConverter* wicConverter = NULL;

	bool imageConverted = false;

	if (wicFactory == NULL)
	{
		// Initialize the COM library
		CoInitialize(NULL);

		// create the WIC factory
		hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&wicFactory)
		);
		if (FAILED(hr)) return 0;
	}
	// load a decoder for the image
	hr = wicFactory->CreateDecoderFromFilename(
		filename,                        // Image we want to load in
		NULL,                            // This is a vendor ID, we do not prefer a specific one so set to null
		GENERIC_READ,                    // We want to read from this file
		WICDecodeMetadataCacheOnLoad,    // We will cache the metadata right away, rather than when needed, which might be unknown
		&wicDecoder                      // the wic decoder to be created
	);
	if (FAILED(hr)) return 0;

	// get image from decoder (this will decode the "frame")
	hr = wicDecoder->GetFrame(0, &wicFrame);
	if (FAILED(hr)) return 0;

	// get wic pixel format of image
	WICPixelFormatGUID pixelFormat;
	hr = wicFrame->GetPixelFormat(&pixelFormat);
	if (FAILED(hr)) return 0;

	// get size of image
	UINT textureWidth, textureHeight;
	hr = wicFrame->GetSize(&textureWidth, &textureHeight);
	if (FAILED(hr)) return 0;
	
	// convert wic pixel format to dxgi pixel format
	DXGI_FORMAT dxgiFormat = GetDXGIFormatFromWICFormat(pixelFormat);

	// if the format of the image is not a supported dxgi format, try to convert it
	if (dxgiFormat == DXGI_FORMAT_UNKNOWN)
	{
		// get a dxgi compatible wic format from the current image format
		WICPixelFormatGUID convertToPixelFormat = GetConvertToWICFormat(pixelFormat);

		// return if no dxgi compatible format was found
		if (convertToPixelFormat == GUID_WICPixelFormatDontCare) return 0;

		// set the dxgi format
		dxgiFormat = GetDXGIFormatFromWICFormat(convertToPixelFormat);

		// create the format converter
		hr = wicFactory->CreateFormatConverter(&wicConverter);
		if (FAILED(hr)) return 0;

		// make sure we can convert to the dxgi compatible format
		BOOL canConvert = FALSE;
		hr = wicConverter->CanConvert(pixelFormat, convertToPixelFormat, &canConvert);
		if (FAILED(hr) || !canConvert) return 0;

		// do the conversion (wicConverter will contain the converted image)
		hr = wicConverter->Initialize(wicFrame, convertToPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) return 0;

		// this is so we know to get the image data from the wicConverter (otherwise we will get from wicFrame)
		imageConverted = true;
	}

	int bitsPerPixel = GetDXGIFormatBitsPerPixel(dxgiFormat); // number of bits per pixel
	bytesPerRow = (textureWidth * bitsPerPixel) / 8; // number of bytes in each row of the image data
	int imageSize = bytesPerRow * textureHeight; // total image size in bytes

	// allocate enough memory for the raw image data, and set imageData to point to that memory
	*imageData = (BYTE*)malloc(imageSize);

	// copy (decoded) raw image data into the newly allocated memory (imageData)
	if (imageConverted)
	{
		// if image format needed to be converted, the wic converter will contain the converted image
		hr = wicConverter->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (FAILED(hr)) return 0;
	}
	else
	{
		// no need to convert, just copy data from the wic frame
		hr = wicFrame->CopyPixels(0, bytesPerRow, imageSize, *imageData);
		if (FAILED(hr)) return 0;
	}

	// now describe the texture with the information we have obtained from the image
	resourceDescription = {};
	resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDescription.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	resourceDescription.Width = textureWidth; // width of the texture
	resourceDescription.Height = textureHeight; // height of the texture
	resourceDescription.DepthOrArraySize = 1; // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	resourceDescription.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level
	resourceDescription.Format = dxgiFormat; // This is the dxgi format of the image (format of the pixels)
	resourceDescription.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	resourceDescription.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags
	
	return imageSize;
}

DXGI_FORMAT Graphics::GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat) return DXGI_FORMAT_R32G32B32A32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf) return DXGI_FORMAT_R16G16B16A16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA) return DXGI_FORMAT_R16G16B16A16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA) return DXGI_FORMAT_R8G8B8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA) return DXGI_FORMAT_B8G8R8A8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR) return DXGI_FORMAT_B8G8R8X8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR) return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;

	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102) return DXGI_FORMAT_R10G10B10A2_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551) return DXGI_FORMAT_B5G5R5A1_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565) return DXGI_FORMAT_B5G6R5_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat) return DXGI_FORMAT_R32_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf) return DXGI_FORMAT_R16_FLOAT;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGray) return DXGI_FORMAT_R16_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppGray) return DXGI_FORMAT_R8_UNORM;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha) return DXGI_FORMAT_A8_UNORM;

	else return DXGI_FORMAT_UNKNOWN;
}

WICPixelFormatGUID Graphics::GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID)
{
	if (wicFormatGUID == GUID_WICPixelFormatBlackWhite) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat2bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat4bppGray) return GUID_WICPixelFormat8bppGray;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint) return GUID_WICPixelFormat16bppGrayHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint) return GUID_WICPixelFormat32bppGrayFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555) return GUID_WICPixelFormat16bppBGRA5551;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010) return GUID_WICPixelFormat32bppRGBA1010102;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf) return GUID_WICPixelFormat64bppRGBAHalf;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE) return GUID_WICPixelFormat128bppRGBAFloat;
	else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha) return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
	else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB) return GUID_WICPixelFormat32bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB) return GUID_WICPixelFormat64bppRGBA;
	else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf) return GUID_WICPixelFormat64bppRGBAHalf;
#endif

	else return GUID_WICPixelFormatDontCare;
}

int Graphics::GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat)
{
	if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT) return 128;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM) return 64;
	else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM) return 32;

	else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM) return 32;
	else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT) return 32;
	else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R16_UNORM) return 16;
	else if (dxgiFormat == DXGI_FORMAT_R8_UNORM) return 8;
	else if (dxgiFormat == DXGI_FORMAT_A8_UNORM) return 8;
}

ID3D12Resource* Graphics::CreateBuffer(D3D12BufferCreateInfo& info)
{
	D3D12_HEAP_PROPERTIES heapDesc = {};
	heapDesc.Type = info.heapType;
	heapDesc.CreationNodeMask = 1;
	heapDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = info.alignment;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Width = info.size;
	resourceDesc.Flags = info.flags;

	// Create the GPU resource
	ID3D12Resource* pResource;
	HRESULT hr = m_pDevice->CreateCommittedResource(&heapDesc, D3D12_HEAP_FLAG_NONE, &resourceDesc, info.state, nullptr, IID_PPV_ARGS(&pResource));
	
	return pResource;
}

void Graphics::CompileShader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob)
{
	HRESULT hr;
	UINT32 code(0);
	IDxcBlobEncoding* pShaderText(nullptr);

	// Load and encode the shader file
	hr = compilerInfo.library->CreateBlobFromFile(info.filename, &code, &pShaderText);
	

	// Create the compiler include handler
	IDxcIncludeHandler* dxcIncludeHandler;
	hr = compilerInfo.library->CreateIncludeHandler(&dxcIncludeHandler);


	// Compile the shader
	IDxcOperationResult* result;
	hr = compilerInfo.compiler->Compile(
		pShaderText,
		info.filename,
		info.entryPoint,
		info.targetProfile,
		info.arguments,
		info.argCount,
		info.defines,
		info.defineCount,
		dxcIncludeHandler,
		&result);

	// Verify the result
	result->GetStatus(&hr);
	if (FAILED(hr))
	{
		IDxcBlobEncoding* error;
		hr = result->GetErrorBuffer(&error);

		// Convert error blob to a string
		std::vector<char> infoLog(error->GetBufferSize() + 1);
		memcpy(infoLog.data(), error->GetBufferPointer(), error->GetBufferSize());
		infoLog[error->GetBufferSize()] = 0;

		std::string errorMsg = "Shader Compiler Error:\n";
		errorMsg.append(infoLog.data());

		MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
		return;
	}

	hr = result->GetResult(blob);
	
}

void Graphics::CompileRayShader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program)
{
	CompileShader(compilerInfo, program.info, &program.blob);
	program.SetBytecode();
}

ID3D12Resource* Graphics::CreateConstantBuffer(UINT64 size)
{
	D3D12BufferCreateInfo bufferInfo((size + 255) & ~255, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	return CreateBuffer(bufferInfo);
}

void Graphics::CreateViewCB()
{
	viewCB = CreateConstantBuffer(sizeof(ViewCB));

	HRESULT hr = viewCB->Map(0, nullptr, reinterpret_cast<void**>(&viewCBStart));

	memcpy(viewCBStart, &viewCBData, sizeof(viewCBData));
}

void Graphics::CreateMateriaCB()
{
	materialCB = CreateConstantBuffer(sizeof(MaterialCB));


	materialCBData.resolution = XMFLOAT4(512, 0.f, 0.f, 0.f);

	HRESULT hr = materialCB->Map(0, nullptr, reinterpret_cast<void**>(&materialCBStart));
	
	memcpy(materialCBStart, &materialCBData, sizeof(materialCBData));
}

void Graphics::InitShaderCompiler(D3D12ShaderCompilerInfo& _shaderCompilerInfo)
{
	HRESULT hr = _shaderCompilerInfo.DxcDllHelper.Initialize();

	hr = _shaderCompilerInfo.DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &shaderCompiler.compiler);

	hr = _shaderCompilerInfo.DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &shaderCompiler.library);
}

void Graphics::LoadModel()
{
	Vertex vList[] = {
		// front face
		Vertex({ -0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}),
		Vertex({  0.5f, -0.5f, -0.5f}, {1.0f, 1.0f }),
		Vertex({ -0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f }),
		Vertex({  0.5f,  0.5f, -0.5f}, { 1.0f, 0.0f}),

		// right side face
		Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f }),
		Vertex({  0.5f,  0.5f,  0.5f}, { 1.0f, 0.0 }),
		Vertex({  0.5f, -0.5f,  0.5f}, { 1.0f, 1.0f }),
		Vertex({  0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f }),

		// left side face
		Vertex({ -0.5f,  0.5f,  0.5f}, {  0.0f, 0.0f}),
		Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f}),
		Vertex({ -0.5f, -0.5f,  0.5f}, { 0.0f, 1.0f }),
		Vertex({ -0.5f,  0.5f, -0.5f}, {  1.0f, 0.0f }),

		// back face
		Vertex({  0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f }),
		Vertex({ -0.5f, -0.5f,  0.5f}, { 1.0f, 1.0f }),
		Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 1.0f }),
		Vertex({ -0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f }),

		// top face
		Vertex({ -0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f }),
		Vertex({ 0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f }),
		Vertex({ 0.5f,  0.5f, -0.5f}, {  1.0f, 1.0f }),
		Vertex({ -0.5f,  0.5f,  0.5f}, { 0.0f,0.0f }),

		// bottom face
		Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f}),
		Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f }),
		Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f,  1.0f }),
		Vertex({ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f })
	};
	for (int i = 0; i < ARRAYSIZE(vList); i++)
	{
		m_model.vertices.push_back(vList[i]);
	}
	DWORD iList[] = {
		// ffront face
		0, 1, 2, // first triangle
		0, 3, 1, // second triangle

		// left face
		4, 5, 6, // first triangle
		4, 7, 5, // second triangle

		// right face
		8, 9, 10, // first triangle
		8, 11, 9, // second triangle

		// back face
		12, 13, 14, // first triangle
		12, 15, 13, // second triangle

		// top face
		16, 17, 18, // first triangle
		16, 19, 17, // second triangle

		// bottom face
		20, 21, 22, // first triangle
		20, 23, 21, // second triangle
	};
	for (int i = 0; i < ARRAYSIZE(iList); i++)
	{
		m_model.indices.push_back(iList[i]);
	}
	int iBufferSize = sizeof(iList);
}

void Graphics::CreateVertexBuffer(Model& model)
{
	// Create the vertex buffer resource
	D3D12BufferCreateInfo info(((UINT)model.vertices.size() * sizeof(Vertex)), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
  m_pVertexBuffer = CreateBuffer(info);

	// Copy the vertex data to the vertex buffer
	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange = {};
	HRESULT hr = m_pVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));


	memcpy(pVertexDataBegin, model.vertices.data(), info.size);
	m_pVertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view
	m_vertexBufferView.BufferLocation = m_pVertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = static_cast<UINT>(info.size);
}

void Graphics::CreateIndexBuffer(Model& model)
{
	// Create the index buffer resource
	D3D12BufferCreateInfo info((UINT)model.indices.size() * sizeof(UINT), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_pIndexBuffer = CreateBuffer(info);

	// Copy the index data to the index buffer
	UINT8* pIndexDataBegin;
	D3D12_RANGE readRange = {};
	HRESULT hr = m_pIndexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));
	

	memcpy(pIndexDataBegin, model.indices.data(), info.size);
	m_pIndexBuffer->Unmap(0, nullptr);

	// Initialize the index buffer view
	m_indexBufferView.BufferLocation = m_pIndexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = static_cast<UINT>(info.size);
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void Graphics::CreateBottomLevelAS(DXRGlobal& _dxr)
{
	// Describe the geometry that goes in the bottom acceleration structure(s)
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc;
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = m_pVertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = m_vertexBufferView.StrideInBytes;
	geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_model.vertices.size());
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geometryDesc.Triangles.IndexBuffer = m_pIndexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexFormat = m_indexBufferView.Format;
	geometryDesc.Triangles.IndexCount = static_cast<UINT>(m_model.indices.size());
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get the size requirements for the BLAS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.pGeometryDescs = &geometryDesc;
	ASInputs.NumDescs = 1;
	ASInputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

	ASPreBuildInfo.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);
	ASPreBuildInfo.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);

	// Create the BLAS scratch buffer
	D3D12BufferCreateInfo bufferInfo(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	bufferInfo.alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	_dxr.BLAS.pScratch = CreateBuffer(bufferInfo);


	// Create the BLAS buffer
	bufferInfo.size = ASPreBuildInfo.ResultDataMaxSizeInBytes;
	bufferInfo.state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	_dxr.BLAS.pResult = CreateBuffer(bufferInfo);

	// Describe and build the bottom level acceleration structure
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = ASInputs;
	buildDesc.ScratchAccelerationStructureData = _dxr.BLAS.pScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = _dxr.BLAS.pResult->GetGPUVirtualAddress();

	m_pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the BLAS build to complete
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = _dxr.BLAS.pResult;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	m_pCommandList->ResourceBarrier(1, &uavBarrier);
}

void Graphics::CreateTopLevelAS(DXRGlobal& _dxr)
{
	// Describe the TLAS geometry instance(s)
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.InstanceMask = 0xFF;
	instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
	instanceDesc.AccelerationStructure = _dxr.BLAS.pResult->GetGPUVirtualAddress();
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

	// Create the TLAS instance buffer
	D3D12BufferCreateInfo instanceBufferInfo;
	instanceBufferInfo.size = sizeof(instanceDesc);
	instanceBufferInfo.heapType = D3D12_HEAP_TYPE_UPLOAD;
	instanceBufferInfo.flags = D3D12_RESOURCE_FLAG_NONE;
	instanceBufferInfo.state = D3D12_RESOURCE_STATE_GENERIC_READ;
	_dxr.TLAS.pInstanceDesc = CreateBuffer(instanceBufferInfo);
	

	// Copy the instance data to the buffer
	UINT8* pData;
	_dxr.TLAS.pInstanceDesc->Map(0, nullptr, (void**)&pData);
	memcpy(pData, &instanceDesc, sizeof(instanceDesc));
	_dxr.TLAS.pInstanceDesc->Unmap(0, nullptr);

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get the size requirements for the TLAS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS ASInputs = {};
	ASInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	ASInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	ASInputs.InstanceDescs = _dxr.TLAS.pInstanceDesc->GetGPUVirtualAddress();
	ASInputs.NumDescs = 1;
	ASInputs.Flags = buildFlags;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO ASPreBuildInfo = {};
	m_pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&ASInputs, &ASPreBuildInfo);

	ASPreBuildInfo.ResultDataMaxSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ResultDataMaxSizeInBytes);
	ASPreBuildInfo.ScratchDataSizeInBytes = ALIGN(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, ASPreBuildInfo.ScratchDataSizeInBytes);

	// Set TLAS size
	_dxr.tlasSize = ASPreBuildInfo.ResultDataMaxSizeInBytes;

	// Create TLAS scratch buffer
	D3D12BufferCreateInfo bufferInfo(ASPreBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	bufferInfo.alignment = max(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
	_dxr.TLAS.pScratch = CreateBuffer(bufferInfo);

	// Create the TLAS buffer
	bufferInfo.size = ASPreBuildInfo.ResultDataMaxSizeInBytes;
	bufferInfo.state = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	_dxr.TLAS.pResult = CreateBuffer(bufferInfo);

	// Describe and build the TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = ASInputs;
	buildDesc.ScratchAccelerationStructureData = _dxr.TLAS.pScratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = _dxr.TLAS.pResult->GetGPUVirtualAddress();

	m_pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// Wait for the TLAS build to complete
	D3D12_RESOURCE_BARRIER uavBarrier;
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = _dxr.TLAS.pResult;
	uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	m_pCommandList->ResourceBarrier(1, &uavBarrier);
}

void Graphics::CreateDXROutPut(int _width, int _height)
{
	// Describe the DXR output resource (texture)
	// Dimensions and format should match the swapchain
	// Initialize as a copy source, since we will copy this buffer's contents to the swapchain
	D3D12_RESOURCE_DESC desc = {};
	desc.DepthOrArraySize = 1;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	desc.Width = _width;
	desc.Height = _height;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;

	// Create the buffer resource
	HRESULT hr = m_pDevice->CreateCommittedResource(&DefaultHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&m_pDXROutput));
	
}

void Graphics::CreateDescriptorHeap(DXRGlobal& _dxr)
{
	// Describe the CBV/SRV/UAV heap
	// Need 7 entries:
	// 1 CBV for the ViewCB
	// 1 CBV for the MaterialCB
	// 1 UAV for the RT output
	// 1 SRV for the Scene BVH
	// 1 SRV for the index buffer
	// 1 SRV for the vertex buffer
	// 1 SRV for the texture
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = 7;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// Create the descriptor heap
	HRESULT hr = m_pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pRayDescriptorHeap));


	// Get the descriptor heap handle and increment size
	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_pRayDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	UINT handleIncrement = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);


	// Create the ViewCB CBV
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = ALIGN(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(viewCBData));
	cbvDesc.BufferLocation = viewCB->GetGPUVirtualAddress();

	m_pDevice->CreateConstantBufferView(&cbvDesc, handle);

	// Create the MaterialCB CBV
	cbvDesc.SizeInBytes = ALIGN(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, sizeof(materialCBData));
	cbvDesc.BufferLocation = materialCB->GetGPUVirtualAddress();

	handle.ptr += handleIncrement;
	m_pDevice->CreateConstantBufferView(&cbvDesc, handle);

	// Create the DXR output buffer UAV
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	handle.ptr += handleIncrement;
	m_pDevice->CreateUnorderedAccessView(m_pDXROutput, nullptr, &uavDesc, handle);

	// Create the DXR Top Level Acceleration Structure SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = _dxr.TLAS.pResult->GetGPUVirtualAddress();

	handle.ptr += handleIncrement;
	m_pDevice->CreateShaderResourceView(nullptr, &srvDesc, handle);

	// Create the index buffer SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC indexSRVDesc;
	indexSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	indexSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	indexSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	indexSRVDesc.Buffer.StructureByteStride = 0;
	indexSRVDesc.Buffer.FirstElement = 0;
	indexSRVDesc.Buffer.NumElements = (static_cast<UINT>(m_model.indices.size()) * sizeof(UINT)) / sizeof(float);
	indexSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle.ptr += handleIncrement;
	m_pDevice->CreateShaderResourceView(m_pIndexBuffer, &indexSRVDesc, handle);

	// Create the vertex buffer SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC vertexSRVDesc;
	vertexSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	vertexSRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	vertexSRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	vertexSRVDesc.Buffer.StructureByteStride = 0;
	vertexSRVDesc.Buffer.FirstElement = 0;
	vertexSRVDesc.Buffer.NumElements = (static_cast<UINT>(m_model.vertices.size()) * sizeof(Vertex)) / sizeof(float);
	vertexSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle.ptr += handleIncrement;
	m_pDevice->CreateShaderResourceView(m_pVertexBuffer, &vertexSRVDesc, handle);

	// Create the material texture SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVDesc = {};
	textureSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	textureSRVDesc.Texture2D.MipLevels = 1;
	textureSRVDesc.Texture2D.MostDetailedMip = 0;
	textureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	handle.ptr += handleIncrement;
	m_pDevice->CreateShaderResourceView(texture, &textureSRVDesc, handle);
}

void Graphics::CreateRayGenProgram(DXRGlobal& _dxr)
{
	// Load and compile the ray generation shader
	_dxr.rgs = RtProgram(D3D12ShaderInfo(L"shaders\\RayGen.hlsl", L"", L"lib_6_3"));
	CompileRayShader(shaderCompiler, _dxr.rgs);

	// Describe the ray generation root signature
	D3D12_DESCRIPTOR_RANGE ranges[3];

	ranges[0].BaseShaderRegister = 0;
	ranges[0].NumDescriptors = 2;
	ranges[0].RegisterSpace = 0;
	ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	ranges[0].OffsetInDescriptorsFromTableStart = 0;

	ranges[1].BaseShaderRegister = 0;
	ranges[1].NumDescriptors = 1;
	ranges[1].RegisterSpace = 0;
	ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	ranges[1].OffsetInDescriptorsFromTableStart = 2;

	ranges[2].BaseShaderRegister = 0;
	ranges[2].NumDescriptors = 4;
	ranges[2].RegisterSpace = 0;
	ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ranges[2].OffsetInDescriptorsFromTableStart = 3;

	D3D12_ROOT_PARAMETER param0 = {};
	param0.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param0.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	param0.DescriptorTable.NumDescriptorRanges = _countof(ranges);
	param0.DescriptorTable.pDescriptorRanges = ranges;

	D3D12_ROOT_PARAMETER rootParams[1] = { param0 };

	D3D12_ROOT_SIGNATURE_DESC rootDesc = {};
	rootDesc.NumParameters = _countof(rootParams);
	rootDesc.pParameters = rootParams;
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	// Create the root signature
	ID3DBlob* sig;
	ID3DBlob* error;
	HRESULT hr = D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sig, &error);
	

	ID3D12RootSignature* pRootSig;
	hr = m_pDevice->CreateRootSignature(0, sig->GetBufferPointer(), sig->GetBufferSize(), IID_PPV_ARGS(&pRootSig));

}

void Graphics::CreateMissProgram(DXRGlobal& _dxr, D3D12ShaderCompilerInfo& compilerInfo)
{
	// Load and compile the miss shader
	_dxr.miss = RtProgram(D3D12ShaderInfo(L"shaders\\Miss.hlsl", L"", L"lib_6_3"));
	CompileRayShader(compilerInfo, _dxr.miss);
}

void Graphics::CreateClosetHitProgram(DXRGlobal& _dxr, D3D12ShaderCompilerInfo& compilerInfo)
{
	// Load and compile the Closest Hit shader
	_dxr.hit = HitProgram(L"Hit");
	_dxr.hit.chs = RtProgram(D3D12ShaderInfo(L"shaders\\ClosestHit.hlsl", L"", L"lib_6_3"));
	CompileRayShader(compilerInfo, _dxr.hit.chs);
}

void Graphics::CreateRayPSO(DXRGlobal& _dxr)
{
	// Need 10 subobjects:
	// 1 for RGS program
	// 1 for Miss program
	// 1 for CHS program
	// 1 for Hit Group
	// 2 for RayGen Root Signature (root-signature and association)
	// 2 for Shader Config (config and association)
	// 1 for Global Root Signature
	// 1 for Pipeline Config	
	UINT index = 0;
	std::vector<D3D12_STATE_SUBOBJECT> subobjects;
	subobjects.resize(10);

	// Add state subobject for the RGS
	D3D12_EXPORT_DESC rgsExportDesc = {};
	rgsExportDesc.Name = L"RayGen_12";
	rgsExportDesc.ExportToRename = L"RayGen";
	rgsExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC	rgsLibDesc = {};
	rgsLibDesc.DXILLibrary.BytecodeLength = _dxr.rgs.blob->GetBufferSize();
	rgsLibDesc.DXILLibrary.pShaderBytecode = _dxr.rgs.blob->GetBufferPointer();
	rgsLibDesc.NumExports = 1;
	rgsLibDesc.pExports = &rgsExportDesc;

	D3D12_STATE_SUBOBJECT rgs = {};
	rgs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	rgs.pDesc = &rgsLibDesc;

	subobjects[index++] = rgs;

	// Add state subobject for the Miss shader
	D3D12_EXPORT_DESC msExportDesc = {};
	msExportDesc.Name = L"Miss_5";
	msExportDesc.ExportToRename = L"Miss";
	msExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC	msLibDesc = {};
	msLibDesc.DXILLibrary.BytecodeLength = _dxr.miss.blob->GetBufferSize();
	msLibDesc.DXILLibrary.pShaderBytecode = _dxr.miss.blob->GetBufferPointer();
	msLibDesc.NumExports = 1;
	msLibDesc.pExports = &msExportDesc;

	D3D12_STATE_SUBOBJECT ms = {};
	ms.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	ms.pDesc = &msLibDesc;

	subobjects[index++] = ms;

	// Add state subobject for the Closest Hit shader
	D3D12_EXPORT_DESC chsExportDesc = {};
	chsExportDesc.Name = L"ClosestHit_76";
	chsExportDesc.ExportToRename = L"ClosestHit";
	chsExportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

	D3D12_DXIL_LIBRARY_DESC	chsLibDesc = {};
	chsLibDesc.DXILLibrary.BytecodeLength = _dxr.hit.chs.blob->GetBufferSize();
	chsLibDesc.DXILLibrary.pShaderBytecode = _dxr.hit.chs.blob->GetBufferPointer();
	chsLibDesc.NumExports = 1;
	chsLibDesc.pExports = &chsExportDesc;

	D3D12_STATE_SUBOBJECT chs = {};
	chs.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	chs.pDesc = &chsLibDesc;

	subobjects[index++] = chs;

	// Add a state subobject for the hit group
	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHit_76";
	hitGroupDesc.HitGroupExport = L"HitGroup";

	D3D12_STATE_SUBOBJECT hitGroup = {};
	hitGroup.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroup.pDesc = &hitGroupDesc;

	subobjects[index++] = hitGroup;

	// Add a state subobject for the shader payload configuration
	D3D12_RAYTRACING_SHADER_CONFIG shaderDesc = {};
	shaderDesc.MaxPayloadSizeInBytes = sizeof(XMFLOAT4);	// RGB and HitT
	shaderDesc.MaxAttributeSizeInBytes = D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES;

	D3D12_STATE_SUBOBJECT shaderConfigObject = {};
	shaderConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shaderConfigObject.pDesc = &shaderDesc;

	subobjects[index++] = shaderConfigObject;

	// Create a list of the shader export names that use the payload
	const WCHAR* shaderExports[] = { L"RayGen_12", L"Miss_5", L"HitGroup" };

	// Add a state subobject for the association between shaders and the payload
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderPayloadAssociation = {};
	shaderPayloadAssociation.NumExports = _countof(shaderExports);
	shaderPayloadAssociation.pExports = shaderExports;
	shaderPayloadAssociation.pSubobjectToAssociate = &subobjects[(index - 1)];

	D3D12_STATE_SUBOBJECT shaderPayloadAssociationObject = {};
	shaderPayloadAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shaderPayloadAssociationObject.pDesc = &shaderPayloadAssociation;

	subobjects[index++] = shaderPayloadAssociationObject;

	// Add a state subobject for the shared root signature 
	D3D12_STATE_SUBOBJECT rayGenRootSigObject = {};
	rayGenRootSigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	rayGenRootSigObject.pDesc = &_dxr.rgs.pRootSignature;

	subobjects[index++] = rayGenRootSigObject;

	// Create a list of the shader export names that use the root signature
	const WCHAR* rootSigExports[] = { L"RayGen_12", L"HitGroup", L"Miss_5" };

	// Add a state subobject for the association between the RayGen shader and the local root signature
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION rayGenShaderRootSigAssociation = {};
	rayGenShaderRootSigAssociation.NumExports = _countof(rootSigExports);
	rayGenShaderRootSigAssociation.pExports = rootSigExports;
	rayGenShaderRootSigAssociation.pSubobjectToAssociate = &subobjects[(index - 1)];

	D3D12_STATE_SUBOBJECT rayGenShaderRootSigAssociationObject = {};
	rayGenShaderRootSigAssociationObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	rayGenShaderRootSigAssociationObject.pDesc = &rayGenShaderRootSigAssociation;

	subobjects[index++] = rayGenShaderRootSigAssociationObject;

	D3D12_STATE_SUBOBJECT globalRootSig;
	globalRootSig.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	globalRootSig.pDesc = &_dxr.miss.pRootSignature;

	subobjects[index++] = globalRootSig;

	// Add a state subobject for the ray tracing pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
	pipelineConfig.MaxTraceRecursionDepth = 1;

	D3D12_STATE_SUBOBJECT pipelineConfigObject = {};
	pipelineConfigObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipelineConfigObject.pDesc = &pipelineConfig;

	subobjects[index++] = pipelineConfigObject;

	// Describe the Ray Tracing Pipeline State Object
	D3D12_STATE_OBJECT_DESC pipelineDesc = {};
	
	pipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipelineDesc.NumSubobjects = static_cast<UINT>(subobjects.size());
	pipelineDesc.pSubobjects = subobjects.data();

	// Create the RT Pipeline State Object (RTPSO)
	HRESULT hr = m_pDevice->CreateStateObject(&pipelineDesc, IID_PPV_ARGS(&_dxr.rtpso));
	

	// Get the RTPSO properties
	hr = _dxr.rtpso->QueryInterface(IID_PPV_ARGS(&_dxr.rtpsoInfo));
	
}

void Graphics::CreateShaderTable(DXRGlobal& dxr)
{
	/*
	The Shader Table layout is as follows:
		Entry 0 - Ray Generation shader
		Entry 1 - Miss shader
		Entry 2 - Closest Hit shader
	All shader records in the Shader Table must have the same size, so shader record size will be based on the largest required entry.
	The ray generation program requires the largest entry:
		32 bytes - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES
		+  8 bytes - a CBV/SRV/UAV descriptor table pointer (64-bits)
		= 40 bytes ->> aligns to 64 bytes
	The entry size must be aligned up to D3D12_RAYTRACING_SHADER_BINDING_TABLE_RECORD_BYTE_ALIGNMENT
	*/

	uint32_t shaderIdSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	uint32_t shaderTableSize = 0;

	dxr.shaderTableRecordSize = shaderIdSize;
	dxr.shaderTableRecordSize += 8;							// CBV/SRV/UAV descriptor table
	dxr.shaderTableRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, dxr.shaderTableRecordSize);

	shaderTableSize = (dxr.shaderTableRecordSize * 3);		// 3 shader records in the table
	shaderTableSize = ALIGN(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT, shaderTableSize);

	// Create the shader table buffer
	D3D12BufferCreateInfo bufferInfo(shaderTableSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	dxr.shaderTable = CreateBuffer(bufferInfo);

	// Map the buffer
	uint8_t* pData;
	HRESULT hr = dxr.shaderTable->Map(0, nullptr, (void**)&pData);

	// Shader Record 0 - Ray Generation program and local root parameter data (descriptor table with constant buffer and IB/VB pointers)
	memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"RayGen_12"), shaderIdSize);

	// Set the root parameter data. Point to start of descriptor heap.
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = m_pRayDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// Shader Record 1 - Miss program (no local root arguments to set)
	pData += dxr.shaderTableRecordSize;
	memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"Miss_5"), shaderIdSize);

	// Shader Record 2 - Closest Hit program and local root parameter data (descriptor table with constant buffer and IB/VB pointers)
	pData += dxr.shaderTableRecordSize;
	memcpy(pData, dxr.rtpsoInfo->GetShaderIdentifier(L"HitGroup"), shaderIdSize);

	// Set the root parameter data. Point to start of descriptor heap.
	*reinterpret_cast<D3D12_GPU_DESCRIPTOR_HANDLE*>(pData + shaderIdSize) = m_pRayDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// Unmap
	dxr.shaderTable->Unmap(0, nullptr);
}
