#pragma once
#include "D12Core.h"
#include "JellyFishDev.h"
#include "Graphics.h"
class Scene : public D12Core
{
public:
  Scene(unsigned int _width, unsigned int _height, std::string _name);
  ~Scene() override;

  bool onInit(LWindow* _window) override;
  bool onUpdate() override;
  bool onRender() override;
  bool onDestroy() override;

private:
  static JellyFishDev m_jellyFish;
  static Graphics m_graphics;
  LWindow m_window;
  bool m_useRayTracing = false;

  Model model;
  Material material;
};

