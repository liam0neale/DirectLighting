#include "Scene.h"
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
