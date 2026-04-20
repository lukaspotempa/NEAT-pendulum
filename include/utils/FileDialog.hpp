#pragma once

#include <string>

namespace utils {

    std::string openFileDialog(const char* filter = "All Files (*.*)\0*.*\0", void* windowHandle = nullptr);

    std::string saveFileDialog(const char* filter = "All Files (*.*)\0*.*\0", const char* defExt = "", void* windowHandle = nullptr);

} // namespace utils