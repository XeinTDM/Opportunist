#pragma once

#include <filesystem>
#include <string>

bool compressDirectoryToZip(const std::filesystem::path& sourceDir, const std::filesystem::path& zipFilePath);