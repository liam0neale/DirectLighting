#pragma once
#include <initguid.h> //has to be first

#include <d3d12.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgi1_4.h>
#include <dxgi.h>


#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

#define NAME_D3D_RESOURCES 1
#define SAFE_RELEASE( x ) { if ( x ) { x->Release(); x = NULL; } }
#define SAFE_DELETE( x ) { if( x ) delete x; x = NULL; }
#define SAFE_DELETE_ARRAY( x ) { if( x ) delete[] x; x = NULL; }
#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

using namespace DirectX;