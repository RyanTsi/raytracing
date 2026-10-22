#include "textures.h"
#include <stb_image.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

TextureBank::~TextureBank() { if (texture) glDeleteTextures(1,&texture); }
int TextureBank::load(const std::string& name,const std::filesystem::path& directory,const aiScene* scene) {
    if (name.empty()) return -1;
    std::string normalized=name;
    std::replace(normalized.begin(),normalized.end(),'\\','/');
    auto path=std::filesystem::u8path(normalized);
    if (path.is_relative()) path=directory/path;
    std::string key=path.lexically_normal().u8string();
    const aiTexture* embedded=scene ? scene->GetEmbeddedTexture(name.c_str()) : nullptr;
    if (embedded) key="embedded:"+std::to_string(modelSerial)+":"+name;
    auto existing=cache.find(key);
    if (existing!=cache.end()) return existing->second;
    Image image;
    int channels=0;
    unsigned char* decoded=nullptr;
    if (embedded && embedded->mHeight==0) {
        if (embedded->mWidth>unsigned(std::numeric_limits<int>::max())) throw std::runtime_error("Embedded texture too large");
        decoded=stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(embedded->pcData),
            int(embedded->mWidth),&image.width,&image.height,&channels,4);
    } else if (embedded) {
        image.width=int(embedded->mWidth); image.height=int(embedded->mHeight);
        image.rgba.resize(size_t(image.width)*image.height*4);
        for (size_t i=0;i<size_t(image.width)*image.height;++i) {
            image.rgba[i*4]=embedded->pcData[i].r; image.rgba[i*4+1]=embedded->pcData[i].g;
            image.rgba[i*4+2]=embedded->pcData[i].b; image.rgba[i*4+3]=embedded->pcData[i].a;
        }
    } else {
        // filesystem::path keeps Unicode on Windows; stb's narrow fopen does not.
        std::ifstream input(path,std::ios::binary|std::ios::ate);
        if (!input) throw std::runtime_error("Missing texture: "+key);
        auto size=input.tellg();
        if (size<=0 || size>std::numeric_limits<int>::max()) throw std::runtime_error("Invalid texture size: "+key);
        std::vector<unsigned char> bytes(static_cast<size_t>(size));
        input.seekg(0); input.read(reinterpret_cast<char*>(bytes.data()),size);
        if (!input) throw std::runtime_error("Cannot read texture: "+key);
        decoded=stbi_load_from_memory(bytes.data(),int(bytes.size()),&image.width,&image.height,&channels,4);
    }
    if (decoded) {
        image.rgba.assign(decoded,decoded+size_t(image.width)*image.height*4);
        stbi_image_free(decoded);
    }
    if (image.rgba.empty()) throw std::runtime_error("Cannot decode texture "+key+": "+(stbi_failure_reason()?stbi_failure_reason():"unknown format"));
    GLint limit=0; glGetIntegerv(GL_MAX_TEXTURE_SIZE,&limit);
    if (image.width<1 || image.height<1 || image.width>limit || image.height>limit)
        throw std::runtime_error("Texture dimensions exceed GPU limits: "+key);
    for (size_t i=3;i<image.rgba.size();i+=4) if (image.rgba[i]!=255) { image.transparent=true; break; }
    int index=int(images.size());
    std::cout<<"Texture "<<(srgb?"sRGB":"data")<<" ["<<index<<"] "<<key<<" "<<image.width<<"x"<<image.height<<"\n";
    images.push_back(std::move(image)); cache.emplace(key,index); return index;
}
bool TextureBank::hasTransparency(int layer) const { return layer>=0 && images.at(size_t(layer)).transparent; }
void TextureBank::upload() {
    if (texture) return;
    int width=1,height=1;
    for (const auto& i:images) { width=std::max(width,i.width); height=std::max(height,i.height); }
    GLint limit=0; glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS,&limit);
    if (images.size()>size_t(limit)) throw std::runtime_error("Too many texture array layers");
    int layers=std::max(1,int(images.size()));
    glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D_ARRAY,texture);
    glTexImage3D(GL_TEXTURE_2D_ARRAY,0,srgb?GL_SRGB8_ALPHA8:GL_RGBA8,width,height,layers,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY,GL_TEXTURE_MAX_LEVEL,0);
    std::vector<unsigned char> pixels(size_t(width)*height*4,255);
    if (images.empty()) glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,0,1,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    for (size_t layer=0;layer<images.size();++layer) {
        const auto& image=images[layer];
        // Texture arrays share dimensions. Nearest resampling preserves encoded
        // colors; same-resolution maps are uploaded losslessly. Flip rows once.
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
            int sx=std::min(image.width-1,int((double(x)+0.5)*image.width/width));
            int sy=image.height-1-std::min(image.height-1,int((double(y)+0.5)*image.height/height));
            size_t src=(size_t(sy)*image.width+sx)*4, dst=(size_t(y)*width+x)*4;
            std::copy_n(image.rgba.data()+src,4,pixels.data()+dst);
        }
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,int(layer),width,height,1,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
    }
    glBindTexture(GL_TEXTURE_2D_ARRAY,0);
    // Keep only metadata after upload, avoid retaining a second full CPU copy.
    for (auto& image:images) { image.rgba.clear(); image.rgba.shrink_to_fit(); }
}
void TextureBank::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0+unit); glBindTexture(GL_TEXTURE_2D_ARRAY,texture);
}
