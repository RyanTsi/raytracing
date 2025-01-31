#version 420 core
in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;
uniform sampler2D ourTexture;
uniform vec3 lightPos;
uniform vec3 lightColor;

void main() {
   float ambientStrength  = 0.2;
   float diffuseStrength  = 0.3;
   float specularStrength = 0.5;
   vec3  norm = normalize(Normal);
   vec3  lightArrow = normalize(lightPos - FragPos);
   vec3  lookArrow  = normalize(-FragPos);
   // float dis = length(lightPos - FragPos);

   vec3 ambient  = ambientStrength  * lightColor;
   vec3 diffuse  = diffuseStrength  * max(0.0, dot(norm, lightArrow)) * lightColor;
   vec3 specular = specularStrength * pow(max(0.0, dot(norm, normalize(lightArrow + lookArrow))), 64) * lightColor;
   
   FragColor = vec4((ambient + diffuse + specular) * Color, 1.0);
}