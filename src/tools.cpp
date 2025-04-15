#include "tools.h"

std::string read_file(const std::string &file_path) {
    std::string res;
    std::ifstream inputFile;
    inputFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    try {
        inputFile.open(file_path);
        std::stringstream stringStream;
        stringStream << inputFile.rdbuf();
        inputFile.close();
        res = stringStream.str();
    }
    catch(std::ifstream::failure e) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    return res;
}