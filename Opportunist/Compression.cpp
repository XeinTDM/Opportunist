#include "Compression.h"
#include "miniz.h"
#include <fstream>
#include <vector>
#include <windows.h>

namespace fs = std::filesystem;

bool compressDirectoryToZip(const fs::path& sourceDir, const fs::path& zipFilePath) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_writer_init_file(&zip, zipFilePath.string().c_str(), 0)) {
        return false;
    }

    int fileCount = 0;

    try {
        for (auto& p : fs::recursive_directory_iterator(sourceDir)) {
            if (fs::is_regular_file(p.path())) {
                fs::path relativePath = fs::relative(p.path(), sourceDir);
                std::string relativePathStr = relativePath.generic_string();

                std::ifstream fileStream(p.path(), std::ios::binary);
                if (!fileStream) {
                    mz_zip_writer_end(&zip);
                    return false;
                }
                std::vector<unsigned char> fileData((std::istreambuf_iterator<char>(fileStream)),
                    std::istreambuf_iterator<char>());
                fileStream.close();

                if (fileData.empty()) {
                    continue;
                }

                if (!mz_zip_writer_add_mem(&zip, relativePathStr.c_str(), fileData.data(),
                    fileData.size(), MZ_BEST_COMPRESSION)) {
                    mz_zip_writer_end(&zip);
                    return false;
                }

                fileCount++;
            }
        }
    }
    catch (...) {
        mz_zip_writer_end(&zip);
        return false;
    }

    if (!mz_zip_writer_finalize_archive(&zip)) {
        mz_zip_writer_end(&zip);
        return false;
    }

    if (!mz_zip_writer_end(&zip)) {
        return false;
    }

    return fileCount > 0;
}
