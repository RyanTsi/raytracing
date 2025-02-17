#version 460 core
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;

struct Material {
    float shininess;
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    vec3  emissive;
    float reflact;
    float opacity;
}; 

uniform Material material;

void main() {
    FragColor = vec4(material.diffuse, 1);
}