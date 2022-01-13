#include "D12Core.h"
unsigned int D12Core::m_width = 0;
unsigned int D12Core::m_height = 0;

D12Core::D12Core(unsigned int _width, unsigned int _height, std::string _name)
{
	m_width = _width;
	m_height = _height;
	
	m_aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);

	m_title = _name;

	m_pGraphics = new Graphics();
}

D12Core::~D12Core()
{
}