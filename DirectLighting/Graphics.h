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
#include <wincodec.h>

#include <vector>
#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream
#include <string>
#include <sstream>
#include "dxcapi.use.h"
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
	Vertex(XMFLOAT3 _pos, XMFLOAT2 _texCoord)
	{
		texCoord = _texCoord;
		pos = _pos;
	}
	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
};

namespace GlobalRootSignatureParams 
{
	enum Value 
	{
		OutputViewSlot = 0,
		AccelerationStructureSlot,
		SceneConstantSlot,
		VertexBuffersSlot,
		Count
	};
}

namespace LocalRootSignatureParams 
{
	enum Value 
	{
		CubeConstantSlot = 0,
		Count
	};
}

enum class RenderMode
{
	rmRAZTERISER,
	rmRAY_TRACE
};

struct AccelerationStructureBuffers
{
	ID3D12Resource* pScratch;
	ID3D12Resource* pResult;
	ID3D12Resource* pInstanceDesc;    // Used only for top-level AS
};
struct DxilLibrary
{
	DxilLibrary(ID3DBlob* pBlob, const WCHAR* entryPoint[], uint32_t entryPointCount)
	{
		pShaderBlob = pBlob;
		stateSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		stateSubobject.pDesc = &dxilLibDesc;

		dxilLibDesc = {};
		exportDesc.resize(entryPointCount);
		exportName.resize(entryPointCount);
		if (pBlob)
		{
			dxilLibDesc.DXILLibrary.pShaderBytecode = pBlob->GetBufferPointer();
			dxilLibDesc.DXILLibrary.BytecodeLength = pBlob->GetBufferSize();
			dxilLibDesc.NumExports = entryPointCount;
			dxilLibDesc.pExports = exportDesc.data();

			for (uint32_t i = 0; i < entryPointCount; i++)
			{
				exportName[i] = entryPoint[i];
				exportDesc[i].Name = exportName[i].c_str();
				exportDesc[i].Flags = D3D12_EXPORT_FLAG_NONE;
				exportDesc[i].ExportToRename = nullptr;
			}
		}
	};

	DxilLibrary() : DxilLibrary(nullptr, nullptr, 0) {}

	D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
	D3D12_STATE_SUBOBJECT stateSubobject{};
	ID3DBlob* pShaderBlob;
	std::vector<D3D12_EXPORT_DESC> exportDesc;
	std::vector<std::wstring> exportName;
};
// this is the structure of our constant buffer.
struct ConstantBufferPerObject 
{
	XMFLOAT4X4 wvpMat;
};
class Graphics
{
public:
	Graphics() = default;
	~Graphics() = default;

	bool OnInit(LWindow& _window);

	void Update();
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
	bool CreatePSO(PSOData& _psoData);
	bool CreateVertexBuffer();
	bool CreateIndexBuffer(int _vBufferSize, ID3D12Resource* _pVBufferUploadHeap);
  bool CreateDepthBuffer(LWindow& _window);

	bool CreatePerObjectConstantBuffer();

	bool InitScene(int _width, int _height);

	bool Test(int _width, int _height, HWND _hwnd);
	//-------

	//For Setting Up The Pipeline
	RenderMode m_renderMode = RenderMode::rmRAZTERISER;
	static const int m_frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

	IDXGIFactory4* dxgiFactory = nullptr;

	ID3D12Device5* m_pDevice = nullptr; // direct3d device

	IDXGISwapChain3* m_pSwapChain = nullptr; // swapchain used to switch between render targets

	ID3D12CommandQueue* m_pCommandQueue = nullptr; // container for command lists

	ID3D12DescriptorHeap* m_pRTVDescriptorHeap = nullptr; // a descriptor heap to hold resources like the render targets

	ID3D12Resource* m_pRenderTargets[m_frameBufferCount]; // number of render targets equal to buffer count

	ID3D12CommandAllocator* m_pCommandAllocator[m_frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

	ID3D12GraphicsCommandList4* m_pCommandList = nullptr; // a command list we can record commands into, then execute them to render the frame

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
	
	int m_ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

	ConstantBufferPerObject m_cbPerObject; // this is the constant buffer data we will send to the gpu 
																					// (which will be placed in the resource we created above)

	ID3D12Resource* m_pConstantBufferUploadHeaps[m_frameBufferCount]; // this is the memory on the gpu where constant buffers for each frame will be placed

	UINT8* m_pCBVGPUAddress[m_frameBufferCount]; // this is a pointer to each of the constant buffer resource heaps

	XMFLOAT4X4 m_cameraProjMat; // this will store our projection matrix
	XMFLOAT4X4 m_cameraViewMat; // this will store our view matrix

	XMFLOAT4 m_cameraPosition; // this is our cameras position vector
	XMFLOAT4 m_cameraTarget; // a vector describing the point in space our camera is looking at
	XMFLOAT4 m_cameraUp; // the worlds up vector

	XMFLOAT4X4 m_cube1WorldMat; // our first cubes world matrix (transformation matrix)
	XMFLOAT4X4 m_cube1RotMat; // this will keep track of our rotation for the first cube
	XMFLOAT4 m_cube1Position; // our first cubes position in space

	XMFLOAT4X4 m_cube2WorldMat; // our first cubes world matrix (transformation matrix)
	XMFLOAT4X4 m_cube2RotMat; // this will keep track of our rotation for the second cube
	XMFLOAT4 m_cube2PositionOffset; // our second cube will rotate around the first cube, so this is the position offset from the first cube

	int m_numCubeIndices; // the number of indices to draw the cube

	//Texture
	bool CreateTexture();
	ID3D12Resource* textureBuffer; // the resource heap containing our texture

	int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int& bytesPerRow);

	DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
	WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
	int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

	ID3D12DescriptorHeap* mainDescriptorHeap;
	ID3D12Resource* textureBufferUploadHeap;


	//Raytracing
	//ID3D12PipelineState* m_pRayTracePSO; // pso containing a pipeline state
	//ID3D12RootSignature* m_pGlobalRayTraceRootSigniture;
	//ID3D12RootSignature* m_pLocalRayTraceRootSigniture;

	bool InitRayRootSigniture();
	bool InitRayTracePSO();

	bool CreateAcceleratedStructures(int _width);
	AccelerationStructureBuffers CreateBottomLevel();
	AccelerationStructureBuffers CreateTopLevel();
	ID3D12Resource* createBuffer(ID3D12Device5* _pDevice, uint64_t _width, D3D12_RESOURCE_FLAGS _flags, D3D12_RESOURCE_STATES _initState, const D3D12_HEAP_PROPERTIES& _heapProps);

	ID3D12Resource* m_pRayTraceVertexBuffer;
	ID3D12Resource* m_pTopLevelAS;
	ID3D12Resource* m_pBottomLevelAS;
	uint64_t m_TlasSize = 0;

	void createRtPipelineState();
	DxilLibrary createDxilLibrary();
	IDxcBlob* compileLibrary(const WCHAR* filename, const WCHAR* targetString);
	std::string convertBlobToString(ID3DBlob* pBlob);
	ID3D12StateObject* m_pRayTracePSO;
	ID3D12RootSignature* m_pEmptyRootSig;
	
	
};

