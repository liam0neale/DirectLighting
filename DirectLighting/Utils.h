#pragma once
#include "JFStructures.h"
namespace Utils
{
	HRESULT ParseCommandLine(LPSTR lpCmdLine, ConfigInfo& config);

	std::vector<char> ReadFile(const std::string& filename);

	void LoadModel(std::string filepath, Model& model, Material& material);

	void Validate(HRESULT hr, LPSTR message);

	TextureInfo LoadTexture(std::string filepath);
}