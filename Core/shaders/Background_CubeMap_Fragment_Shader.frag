#version 330 core

out vec4 FragColor;
precision mediump float; 
uniform samplerCube cubemapSampler; 
in vec3 texCoord_v;

void main() {
    FragColor = texture(cubemapSampler, texCoord_v) * 0.7; //correct the brightness a bit
}