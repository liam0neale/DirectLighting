#include <Windows.h>

#include "DXDefines.h"
#include "Scene.h"
#include "WindowsApp.h"

int WINAPI WinMain(HINSTANCE hInstance,    //Main windows function
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)

{
	Scene* scene = new Scene(1280, 720, "D3D12 Hello Triangle");
	return WindowsApp::Run(scene, hInstance, nShowCmd);
}