#pragma once
#include <Windows.h>
#include <D3d12.h>
#include <D3d12SDKLayers.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <dxgi.h>
#include <string>
class LWindow
{
	public:
	LWindow();
	~LWindow();
	bool Init(HINSTANCE& _hInstance, int _showWnd, int _width, int _height, bool _windowed, WNDPROC _wndProc, LPCSTR _title);
	HWND getWindow() {return m_hwnd;}

	private:
	const LPCSTR m_WND_CLASS_NAME = "firstwindow";
	HWND m_hwnd = NULL;
};

