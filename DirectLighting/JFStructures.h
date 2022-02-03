#pragma once

#include "DXDefines.h"
#include "dxcapi.use.h"
 //--------------------------------------------------------------------------------------
 // Helpers
 //--------------------------------------------------------------------------------------

static bool CompareVector3WithEpsilon(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs)
{
	const DirectX::XMFLOAT3 vector3Epsilon = DirectX::XMFLOAT3(0.00001f, 0.00001f, 0.00001f);
	return DirectX::XMVector3NearEqual(DirectX::XMLoadFloat3(&lhs), DirectX::XMLoadFloat3(&rhs), DirectX::XMLoadFloat3(&vector3Epsilon)) == TRUE;
}

static bool CompareVector2WithEpsilon(const DirectX::XMFLOAT2& lhs, const DirectX::XMFLOAT2& rhs)
{
	const DirectX::XMFLOAT2 vector2Epsilon = DirectX::XMFLOAT2(0.00001f, 0.00001f);
	return DirectX::XMVector3NearEqual(DirectX::XMLoadFloat2(&lhs), DirectX::XMLoadFloat2(&rhs), DirectX::XMLoadFloat2(&vector2Epsilon)) == TRUE;
}

//--------------------------------------------------------------------------------------
// Global Structures
//--------------------------------------------------------------------------------------

struct ConfigInfo
{
	int				width = 640;
	int				height = 360;
	bool			vsync = false;
	std::string		model = "";
	HINSTANCE		instance = NULL;
};

struct Vertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 uv;

	bool operator==(const Vertex& v) const
	{
		if (CompareVector3WithEpsilon(position, v.position))
		{
			if (CompareVector2WithEpsilon(uv, v.uv)) return true;
			return true;
		}
		return false;
	}

	Vertex& operator=(const Vertex& v)
	{
		position = v.position;
		uv = v.uv;
		return *this;
	}
	Vertex(DirectX::XMFLOAT3 _position, DirectX::XMFLOAT2 _uv)
	{
		position = _position;
		uv = _uv;
	}
};

struct TESTVERTEX
{
	DirectX::XMFLOAT3 pos;
};

struct Material
{
	std::string name = "defaultMaterial";
	std::string texturePath = "";
	float  textureResolution = 512;
};
struct ConstantBufferPerObject
{
	XMFLOAT4X4 wvpMat;
};

struct Model
{
	std::vector<Vertex> vertices;
	std::vector<TESTVERTEX> testVertex;
	std::vector<uint32_t> indices;
	XMFLOAT4 pos;
	XMFLOAT4X4 rotMat;
	XMFLOAT4X4 world;
	ConstantBufferPerObject cbPerObject;
};

struct Camera
{
	XMFLOAT4 pos;
	XMFLOAT4 rot;
	XMFLOAT4X4 view;
	XMFLOAT4X4 projection;
	XMFLOAT4 target;
	XMFLOAT4 up;
};

struct TextureInfo
{
	std::vector<UINT8> pixels;
	int width = 0;
	int height = 0;
	int stride = 0;
	int offset = 0;
};

struct MaterialCB
{
	DirectX::XMFLOAT4 resolution;
};

struct ViewCB
{
	DirectX::XMMATRIX view = DirectX::XMMatrixIdentity();
	DirectX::XMFLOAT4 viewOriginAndTanHalfFovY = DirectX::XMFLOAT4(0, 0.f, 0.f, 0.f);
	DirectX::XMFLOAT2 resolution = DirectX::XMFLOAT2(1280, 720);
};

//--------------------------------------------------------------------------------------
// D3D12
//--------------------------------------------------------------------------------------

struct D3D12BufferCreateInfo
{
	UINT64 size = 0;
	UINT64 alignment = 0;
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;

	D3D12BufferCreateInfo() {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags) : size(InSize), flags(InFlags) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		heapType(InHeapType),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		flags(InFlags),
		state(InState) {}

	D3D12BufferCreateInfo(UINT64 InSize, UINT64 InAlignment, D3D12_HEAP_TYPE InHeapType, D3D12_RESOURCE_FLAGS InFlags, D3D12_RESOURCE_STATES InState) :
		size(InSize),
		alignment(InAlignment),
		heapType(InHeapType),
		flags(InFlags),
		state(InState) {}
};

struct D3D12ShaderCompilerInfo
{
	dxc::DxcDllSupport		DxcDllHelper;
	IDxcCompiler* compiler = nullptr;
	IDxcLibrary* library = nullptr;
};

struct D3D12ShaderInfo
{
	LPCWSTR		filename = nullptr;
	LPCWSTR		entryPoint = nullptr;
	LPCWSTR		targetProfile = nullptr;
	LPCWSTR* arguments = nullptr;
	DxcDefine* defines = nullptr;
	UINT32		argCount = 0;
	UINT32		defineCount = 0;

	D3D12ShaderInfo() {}
	D3D12ShaderInfo(LPCWSTR inFilename, LPCWSTR inEntryPoint, LPCWSTR inProfile)
	{
		filename = inFilename;
		entryPoint = inEntryPoint;
		targetProfile = inProfile;
	}
};

struct D3D12Resources
{
	ID3D12Resource* DXROutput;

	ID3D12Resource* vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW						vertexBufferView;
	ID3D12Resource* indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW							indexBufferView;

	ID3D12Resource* viewCB = nullptr;
	ViewCB											viewCBData;
	UINT8* viewCBStart = nullptr;

	ID3D12Resource* materialCB = nullptr;
	MaterialCB										materialCBData;
	UINT8* materialCBStart = nullptr;

	ID3D12DescriptorHeap* rtvHeap = nullptr;
	ID3D12DescriptorHeap* descriptorHeap = nullptr;

	ID3D12Resource* texture = nullptr;
	ID3D12Resource* textureUploadResource = nullptr;

	UINT											rtvDescSize = 0;

	float											translationOffset = 0;
	float											rotationOffset = 0;
	DirectX::XMFLOAT3								eyeAngle;
	DirectX::XMFLOAT3								eyePosition;
};

struct D3D12Global
{
	static const int frameBufferCount = 3;
	IDXGIFactory4* factory = nullptr;
	IDXGIAdapter1* adapter = nullptr;
	ID3D12Device5* device = nullptr;
	ID3D12GraphicsCommandList4* cmdList = nullptr;
	ID3D12CommandQueue* cmdQueue = nullptr;
	ID3D12CommandAllocator* cmdAlloc[frameBufferCount] = { nullptr, nullptr, nullptr };

	IDXGISwapChain3* swapChain = nullptr;
	ID3D12Resource* backBuffer[frameBufferCount] = { nullptr, nullptr, nullptr};
	ID3D12Fence* fence = nullptr;
	UINT64											fenceValues[frameBufferCount] = { 0, 0, 0 };
	HANDLE											fenceEvent;
	UINT											frameIndex = 0;

	int												width = 800;
	int												height = 600;
	bool											vsync = false;
	
};

//--------------------------------------------------------------------------------------
//  DXR
//--------------------------------------------------------------------------------------

struct AccelerationStructureBuffer
{
	ID3D12Resource* pScratch = nullptr;
	ID3D12Resource* pResult = nullptr;
	ID3D12Resource* pInstanceDesc = nullptr;	// only used in top-level AS
};

struct RtProgram
{
	D3D12ShaderInfo			info = {};
	IDxcBlob* blob = nullptr;
	ID3D12RootSignature* pRootSignature = nullptr;

	D3D12_DXIL_LIBRARY_DESC	dxilLibDesc;
	D3D12_EXPORT_DESC		exportDesc;
	D3D12_STATE_SUBOBJECT	subobject;
	std::wstring			exportName;

	RtProgram()
	{
		exportDesc.ExportToRename = nullptr;
	}

	RtProgram(D3D12ShaderInfo shaderInfo)
	{
		info = shaderInfo;
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		exportName = shaderInfo.entryPoint;
		exportDesc.ExportToRename = nullptr;
		exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;
	}

	void SetBytecode()
	{
		exportDesc.Name = exportName.c_str();

		dxilLibDesc.NumExports = 1;
		dxilLibDesc.pExports = &exportDesc;
		dxilLibDesc.DXILLibrary.BytecodeLength = blob->GetBufferSize();
		dxilLibDesc.DXILLibrary.pShaderBytecode = blob->GetBufferPointer();

		subobject.pDesc = &dxilLibDesc;
	}

};

struct HitProgram
{
	RtProgram ahs;
	RtProgram chs;

	std::wstring exportName;
	D3D12_HIT_GROUP_DESC desc = {};
	D3D12_STATE_SUBOBJECT subobject = {};

	HitProgram() {}
	HitProgram(LPCWSTR name) : exportName(name)
	{
		desc = {};
		desc.HitGroupExport = exportName.c_str();
		subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		subobject.pDesc = &desc;
	}

	void SetExports(bool anyHit)
	{
		desc.HitGroupExport = exportName.c_str();
		if (anyHit) desc.AnyHitShaderImport = ahs.exportDesc.Name;
		desc.ClosestHitShaderImport = chs.exportDesc.Name;
	}

};

struct RasterProgram
{
	ID3D12RootSignature* rootSig;
	ID3D12PipelineState* pso;
	ID3D12Resource* pVertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
																							 // the total size of the buffer, and the size of each element (vertex)
	ID3D12Resource* pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	int numIndices = 0;
};

struct DXRGlobal
{
	AccelerationStructureBuffer						TLAS;
	AccelerationStructureBuffer						BLAS;
	uint64_t										tlasSize;

	ID3D12Resource* shaderTable = nullptr;
	uint32_t										shaderTableRecordSize = 0;

	RtProgram										rgs;
	RtProgram										miss;
	HitProgram										hit;

	ID3D12StateObject* rtpso = nullptr;
	ID3D12StateObjectProperties* rtpsoInfo = nullptr;
};

