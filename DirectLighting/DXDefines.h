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

#define d3d_call(a) {HRESULT hr_ = a; if(FAILED(hr_)) { d3dTraceHR( #a, hr_); }}
#define arraysize(a) (sizeof(a)/sizeof(a[0]))