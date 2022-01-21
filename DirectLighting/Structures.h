#pragma once
#include "DXDefines.h"
#include <dxcapi.h>

#include "dxcapi.use.h"
struct ConfigInfo
{
	int	width = 640;
	int	height = 360;
	std::string		model = "";
	HINSTANCE		instance = NULL;
};

struct Vertex
{
	Vertex(DirectX::XMFLOAT3 _pos, DirectX::XMFLOAT2 _texCoord)
	{
		texCoord = _texCoord;
		pos = _pos;
	}
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 texCoord;
};

struct Material
{
	std::string name = "defaultMaterial";
	std::string texturePath = "";
	float  textureResolution = 512;
};

struct Model
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
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


// RAY TRACING
struct D3D12ShaderCompilerInfo
{
	dxc::DxcDllSupport		DxcDllHelper;
	IDxcCompiler* compiler = nullptr;
	IDxcLibrary* library = nullptr;
};

struct AccelerationStructureBuffer
{
	ID3D12Resource* pScratch = nullptr;
	ID3D12Resource* pResult = nullptr;
	ID3D12Resource* pInstanceDesc = nullptr;	// only used in top-level AS
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
