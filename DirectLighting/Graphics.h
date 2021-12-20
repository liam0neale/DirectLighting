#pragma once
#include "DXDefines.h"
#include "GraphicsData.h"
#include "LWindow.h"
#include "Status.h"

using namespace GData;
using namespace EngineStatus;

class Graphics
{
public:
	Graphics() = default;
	~Graphics() = default;

	bool OnInit(LWindow& _window);

	void UpdatePipeline();
	void WaitForPreviousFrame();
	void CleanUp();
private:

	//Pipeline 
	bool InitDevice();
	bool InitCommandQueue();
	bool InitSwapchain(LWindow& _window);
	bool InitRenderTargets();
	bool InitCommandAllocators();
	bool InitCommandList();
	bool InitFence();

	
};

