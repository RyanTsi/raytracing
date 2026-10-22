#include "tools.h"
#include <stdexcept>
#include <filesystem>
std::string read_file(const std::string& path) {
    std::ifstream file(std::filesystem::u8path(path),std::ios::binary);
    if (!file) throw std::runtime_error("Cannot read file: "+path);
    std::ostringstream out; out<<file.rdbuf(); return out.str();
}
