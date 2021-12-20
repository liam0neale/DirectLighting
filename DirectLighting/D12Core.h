#pragma once
#include <string>
#include "LWindow.h"


class D12Core
{
public:
	D12Core(unsigned int _width, unsigned int _height, std::string _name);
	virtual ~D12Core();

	virtual bool onInit(LWindow* _window) = 0;
	virtual bool onUpdate() = 0;
	virtual bool onRender() = 0;
	virtual bool onDestroy() = 0;

	int getWidth(){return m_width;}
	int getHeight(){return m_height;}
	std::string getTitle(){return m_title;}

protected:
	unsigned int m_width;
	unsigned int m_height;
	float m_aspectRatio;

private:
	// Root assets path.
	std::string m_assetsPath;

	// Window title.
	std::string m_title;
};

