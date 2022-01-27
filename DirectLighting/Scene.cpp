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
  config.vsync = false;
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

  
  if(!m_useRayTracing)
  {
    Vertex vList[] = {
      // front face
      Vertex({-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f}),
      Vertex({0.5f, -0.5f, -0.5f}, {1.0f, 1.0f }),
      Vertex({ -0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f }),
      Vertex({  0.5f,  0.5f, -0.5f}, { 1.0f, 0.0f}),

      // right side face
      Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f, 1.0f }),
      Vertex({  0.5f,  0.5f,  0.5f}, { 1.0f, 0.0 }),
      Vertex({  0.5f, -0.5f,  0.5f}, { 1.0f, 1.0f }),
      Vertex({  0.5f,  0.5f, -0.5f}, { 0.0f, 0.0f }),

      // left side face
      Vertex({ -0.5f,  0.5f,  0.5f}, {  0.0f, 0.0f}),
      Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f}),
      Vertex({ -0.5f, -0.5f,  0.5f}, { 0.0f, 1.0f }),
      Vertex({ -0.5f,  0.5f, -0.5f}, {  1.0f, 0.0f }),

      // back face
      Vertex({  0.5f,  0.5f,  0.5f}, { 0.0f, 0.0f }),
      Vertex({ -0.5f, -0.5f,  0.5f}, { 1.0f, 1.0f }),
      Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 1.0f }),
      Vertex({ -0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f }),

      // top face
      Vertex({ -0.5f,  0.5f, -0.5f}, { 0.0f, 1.0f }),
      Vertex({ 0.5f,  0.5f,  0.5f}, { 1.0f, 0.0f }),
      Vertex({ 0.5f,  0.5f, -0.5f}, {  1.0f, 1.0f }),
      Vertex({ -0.5f,  0.5f,  0.5f}, { 0.0f,0.0f }),

      // bottom face
      Vertex({  0.5f, -0.5f,  0.5f}, { 0.0f, 0.0f}),
      Vertex({ -0.5f, -0.5f, -0.5f}, { 1.0f, 1.0f }),
      Vertex({  0.5f, -0.5f, -0.5f}, { 0.0f,  1.0f }),
      Vertex({ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f })
    };
    for (size_t i = 0; i < ARRAYSIZE(vList); i++)
    {
      m_cube.vertices.push_back(vList[i]);
    }
    DWORD iList[] = {
      // ffront face
      0, 1, 2, // first triangle
      0, 3, 1, // second triangle

      // left face
      4, 5, 6, // first triangle
      4, 7, 5, // second triangle

      // right face
      8, 9, 10, // first triangle
      8, 11, 9, // second triangle

      // back face
      12, 13, 14, // first triangle
      12, 15, 13, // second triangle

      // top face
      16, 17, 18, // first triangle
      16, 19, 17, // second triangle

      // bottom face
      20, 21, 22, // first triangle
      20, 23, 21, // second triangle
    };
    for (size_t i = 0; i < ARRAYSIZE(iList); i++)
    {
      m_cube.indices.push_back(iList[i]);
    }
    
    //m_graphics.OnInit(*_window);
    m_jellyFish.CreateRasterProgram(*dx);
    m_jellyFish.CreateRasterPSO(*dx, model);
  }

  dx->cmdList->Close();
  ID3D12CommandList* pGraphicsList = { dx->cmdList };
  dx->cmdQueue->ExecuteCommandLists(1, &pGraphicsList);

  D3D12::WaitForGPU(*dx);
  D3D12::Reset_CommandList(*dx);
  return result;
}

bool Scene::onUpdate()
{
 
  D3DResources::Update_View_CB(*m_jellyFish.getDXGlobal(), *m_jellyFish.getResources());
  
  if(!m_useRayTracing)
  {
    //m_graphics.Update();
  }

  return true;
}

bool Scene::onRender()
{
  if (!m_useRayTracing)
  {
    //m_jellyFish.UpdatePipeline(*m_jellyFish.getDXGlobal(), *m_jellyFish.getResources());
  }
  DXR::Build_Command_List(*m_jellyFish.getDXGlobal(), *m_jellyFish.getDXRGlobal(), *m_jellyFish.getResources());
    
  
  
  D3D12::Present(*m_jellyFish.getDXGlobal());
  D3D12::MoveToNextFrame(*m_jellyFish.getDXGlobal());
  D3D12::Reset_CommandList(*m_jellyFish.getDXGlobal());
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
