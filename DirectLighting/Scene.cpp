#include "Scene.h"

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
    return true;
}

bool Scene::onUpdate()
{
  return true;
}

bool Scene::onRender()
{
  return true;
}

bool Scene::onDestroy()
{
  return true;
}
