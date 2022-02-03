#include "Scene.h"

JellyFishDev Scene::m_jellyFish = JellyFishDev();
Graphics Scene::m_graphics = Graphics();
unsigned int Scene::m_width = 0;
unsigned int Scene::m_height = 0;
Scene::Scene(unsigned int _width, unsigned int _height, std::string _name) : D12Core(_width, _height, _name)
{

  
}

Scene::~Scene()
{

}

bool Scene::onInit(LWindow* _window)
{
  bool result = true;

  ConfigInfo config;
  config.width = _window->getWidth();
  config.height = _window->getHeight();
  config.model = "quad.obj";
  config.vsync = true;
  m_jellyFish.Init(config);

  // Initialize D3D12
  HWND win = _window->getWindow();

  D3D12Global* dx = m_jellyFish.getDXGlobal();
  DXRGlobal* dxr = m_jellyFish.getDXRGlobal();
  D3D12ShaderCompilerInfo* shaderInfo = m_jellyFish.getShaderInfo();
  D3D12Resources* resources = m_jellyFish.getResources();

  D3D12::Create_Device(*dx);
  D3D12::Create_Command_Queue(*dx);
  D3D12::Create_Command_Allocator(*dx);
  D3D12::Create_Fence(*dx);
  D3D12::Create_SwapChain(*dx, win);
  D3D12::Create_CommandList(*dx);
  D3D12::Reset_CommandList(*dx);

  // Create common resources
  D3DResources::Create_Descriptor_Heaps(*dx, *resources);
  D3DResources::Create_BackBuffer_RTV(*dx, *resources);

  if (m_useRayTracing)
  {
    
    
    
 

    // Load a model
    Utils::LoadModel(config.model, model, material);

    // Initialize the shader compiler
    D3DShaders::Init_Shader_Compiler(*m_jellyFish.getShaderInfo());
    
    D3DResources::Create_Vertex_Buffer(*dx, *resources, model);
    D3DResources::Create_Index_Buffer(*dx, *resources, model);
    D3DResources::Create_Texture(*dx, *resources, material);
    D3DResources::Create_View_CB(*dx, *resources);
    D3DResources::Create_Material_CB(*dx, *resources, material);

    // Create DXR specific resources
    DXR::Create_Bottom_Level_AS(*dx, *dxr, *resources, model);
    DXR::Create_Top_Level_AS(*dx, *dxr, *resources);
    DXR::Create_DXR_Output(*dx, *resources);
    DXR::Create_Descriptor_Heaps(*dx, *dxr, *resources, model);
    DXR::Create_RayGen_Program(*dx, *dxr, *shaderInfo);
    DXR::Create_Miss_Program(*dx, *dxr, *shaderInfo);
    DXR::Create_Closest_Hit_Program(*dx, *dxr, *shaderInfo);
    DXR::Create_Pipeline_State_Object(*dx, *dxr);
    DXR::Create_Shader_Table(*dx, *dxr, *resources);

    dx->cmdList->Close();
    ID3D12CommandList* pGraphicsList = { dx->cmdList };
    dx->cmdQueue->ExecuteCommandLists(1, &pGraphicsList);

   
  }
  
  if(!m_useRayTracing)
  {
    m_jellyFish.InitRasterTest(_window);
  }

  D3D12::WaitForGPU(*dx);
  D3D12::Reset_CommandList(*dx);
 
  return result;
}

bool Scene::onUpdate()
{ 
  if(!m_useRayTracing)
  {
    // create rotation matrices
 /*XMMATRIX rotXMat = XMMatrixRotationX(0.0001f);
    XMMATRIX rotYMat = XMMatrixRotationY(0.0002f);
    XMMATRIX rotZMat = XMMatrixRotationZ(0.0003f);

    // add rotation to cube1's rotation matrix and store it
    XMMATRIX rotMat = XMLoadFloat4x4(&m_cube.rotMat) * rotXMat * rotYMat * rotZMat;
    XMStoreFloat4x4(&m_cube.rotMat, rotMat);

    // create translation matrix for cube 1 from cube 1's position vector
    XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&m_cube.pos));

    // create cube1's world matrix by first rotating the cube, then positioning the rotated cube
    XMMATRIX worldMat = rotMat * translationMat;

    // store cube1's world matrix
    XMStoreFloat4x4(&m_cube.world, worldMat);

    // update constant buffer for cube1
    // create the wvp matrix and store in constant buffer
    XMMATRIX viewMat = XMLoadFloat4x4(&m_camera.view); // load view matrix
    XMMATRIX projMat = XMLoadFloat4x4(&m_camera.projection); // load projection matrix
    XMMATRIX wvpMat = XMLoadFloat4x4(&m_cube.world) * viewMat * projMat; // create wvp matrix
    XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&m_cube.cbPerObject.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    // copy our ConstantBuffer instance to the mapped constant buffer resource
    memcpy(m_jellyFish.m_pCBVGPUAddress[m_jellyFish.getDXGlobal()->frameIndex], &m_cube.cbPerObject, sizeof(m_cube.cbPerObject));*/
  }
  else
  {
    D3DResources::Update_View_CB(*m_jellyFish.getDXGlobal(), *m_jellyFish.getResources());
  }
  return true;
}

bool Scene::onRender()
{
  if (!m_useRayTracing)
  {
    
    m_jellyFish.RenderTest();
   
    // present the current backbuffer
   // hr = m_jellyFish.getDXGlobal()->swapChain->Present(0, 0);*/
  }
  else
  {
    DXR::Build_Command_List(*m_jellyFish.getDXGlobal(), *m_jellyFish.getDXRGlobal(), *m_jellyFish.getResources());
    // Submit the command list and wait for the GPU to idle
    D3D12::Submit_CmdList(*m_jellyFish.getDXGlobal());
    D3D12::WaitForGPU(*m_jellyFish.getDXGlobal());

    D3D12::Present(*m_jellyFish.getDXGlobal());
    D3D12::MoveToNextFrame(*m_jellyFish.getDXGlobal());
    D3D12::Reset_CommandList(*m_jellyFish.getDXGlobal());
  }
  

  return true;
}

bool Scene::onDestroy()
{
  if (m_useRayTracing)
  {
    D3D12::WaitForGPU(*m_jellyFish.getDXGlobal());
    CloseHandle(m_jellyFish.getDXGlobal()->fenceEvent);

    DXR::Destroy(*m_jellyFish.getDXRGlobal());
    D3DResources::Destroy(*m_jellyFish.getResources());
    D3DShaders::Destroy(*m_jellyFish.getShaderInfo());
    D3D12::Destroy(*m_jellyFish.getDXGlobal());
  }
  return true;
}
