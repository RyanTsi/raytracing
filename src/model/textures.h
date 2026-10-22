#pragma once
#include <glad/gl.h>
#include <assimp/scene.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class TextureBank {
    struct Image {
        int width=0,height=0;
        bool transparent=false;
        std::vector<unsigned char> rgba;
    };
    bool srgb;
    GLuint texture=0;
    std::vector<Image> images;
    std::unordered_map<std::string,int> cache;
    size_t modelSerial=0;
public:
    explicit TextureBank(bool color) : srgb(color) {}
    ~TextureBank();
    TextureBank(const TextureBank&)=delete;
    TextureBank& operator=(const TextureBank&)=delete;
    void beginModel() { ++modelSerial; }
    int load(const std::string& name,const std::filesystem::path& directory,const aiScene* scene);
    bool hasTransparency(int layer) const;
    size_t count() const { return images.size(); }
    void upload();
    void bind(int unit) const;
};
