#pragma once

#include <string>
#include <vector>	
#include <fstream>
#include <filesystem>
#include <print>

inline std::vector<char> loadFile(const std::string& filename)
{
	auto file = std::ifstream{ filename, std::ios::ate | std::ios::binary };
	if (!file.is_open())
		throw std::runtime_error{ "failed to open file" };

	auto fileSize = static_cast<size_t>(file.tellg());
	std::println("{} size is {} bytes", filename, fileSize);

	auto buffer = std::vector<char>(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	return buffer;
}