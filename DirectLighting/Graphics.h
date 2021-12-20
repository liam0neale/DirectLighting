#pragma once
#include "DXDefines.h"
#include "GraphicsData.h"
#include "D12Core.h"
#include "LWindow.h"

using namespace GraphicsData;

class Graphics
{
public:
	Graphics() = default;
	~Graphics() = default;

	bool OnInit(LWindow& _window);
private:

	//Pipeline 
	bool InitDevice();
	bool InitCommandQueue();
	bool InitSwapchain(LWindow& _window);
	bool InitRenderTargets();
};

