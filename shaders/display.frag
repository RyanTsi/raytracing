#version 430 core
in vec2 screen_uv;
out vec4 FragColor;
uniform sampler2D image;
uniform float exposure;
vec3 aces(vec3 x) { return clamp((x*(2.51*x+0.03))/(x*(2.43*x+0.59)+0.14),0.0,1.0); }
vec3 linearToSRGB(vec3 c) {
    return mix(12.92*c,1.055*pow(c,vec3(1.0/2.4))-0.055,step(vec3(0.0031308),c));
}
void main() { FragColor=vec4(linearToSRGB(aces(max(texture(image,screen_uv).rgb,vec3(0))*exp2(exposure))),1); }
