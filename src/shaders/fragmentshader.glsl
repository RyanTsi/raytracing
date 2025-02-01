#version 420 core
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

// uniform sampler2D ourTexture;
uniform float ambient;

uniform vec3 lightPos;
uniform vec3 lightColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
    sampler2D height;
    float shininess;
}; 

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_diffuse2;
uniform sampler2D texture_diffuse3;
uniform sampler2D texture_specular1;
uniform sampler2D texture_specular2;
uniform sampler2D texture_specular3;
uniform sampler2D texture_normal1;
uniform sampler2D texture_normal2;
uniform sampler2D texture_normal3;

uniform Material material;

void main() {
    // vec3  norm = normalize(Normal);
    // vec3  lightArrow = normalize(lightPos - FragPos);
    // vec3  lookArrow  = normalize(-FragPos);
    // // float dis = length(lightPos - FragPos);

    // vec3 ambient  = ambient  * lightColor;
    // vec3 diffuse  = material.diffuse  * max(0.0, dot(norm, lightArrow)) * lightColor;
    // vec3 specular = material.specular * pow(max(0.0, dot(norm, normalize(lightArrow + lookArrow))), material.shininess) * lightColor;
    
    // FragColor = vec4((ambient + diffuse + specular), 1.0);
    FragColor = texture(texture_diffuse1, TexCoord);
}