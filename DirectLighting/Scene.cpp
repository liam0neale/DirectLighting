#include "Scene.h"

unsigned int Scene::m_width = 0;
unsigned int Scene::m_height = 0;
Scene::Scene(unsigned int _width, unsigned int _height, std::string _name) : D12Core(_width, _height, _name)
{
  
}

Scene::~Scene()
{
  if (m_pGraphics)
  {
    delete(m_pGraphics);
    m_pGraphics = nullptr;
  }
}

bool Scene::onInit(LWindow* _window)
{
  bool result = true;
  result = m_pGraphics->OnInit(*_window);
  return result;
}

bool Scene::onUpdate()
{
  return true;
}

bool Scene::onRender()
{
  HRESULT hr;

  m_pGraphics->UpdatePipeline(); // update the pipeline by sending commands to the commandqueue

  // create an array of command lists (only one command list here)
  ID3D12CommandList* ppCommandLists[] = { m_pCommandList };

  // execute the array of command lists
  m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

  // this command goes in at the end of our command queue. we will know when our command queue 
  // has finished because the fence value will be set to "fenceValue" from the GPU since the command
  // queue is being executed on the GPU
  hr = m_pCommandQueue->Signal(m_pFence[m_frameIndex], m_fenceValue[m_frameIndex]);
  if (FAILED(hr))
  {
    m_status = Status::sERRORED;
  }

  // present the current backbuffer
  hr = m_pSwapChain->Present(0, 0);
  if (FAILED(hr))
  {
    m_status = Status::sERRORED;
  }

  return true;
}

bool Scene::onDestroy()
{
  return true;
}
