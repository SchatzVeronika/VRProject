#version 330 core

in vec3 position;
in vec2 tex_coord;
in vec3 normal;

out vec3 v_frag_coord;
out vec3 v_normal;
out vec2 TexCoord;
out vec3 LightPos; // Pass the light position to the fragment shader

uniform mat4 M;
uniform mat4 itM;
uniform mat4 V;
uniform mat4 P;
uniform vec3 lightPos; // The position of the light source

void main() {
    gl_Position = P * V * M * vec4(position, 1.0);

    v_normal = normalize(vec3(itM * vec4(normal, 0.0)));
    v_frag_coord = vec3(M * vec4(position, 1.0));
    TexCoord = tex_coord;

    // Pass the light position to the fragment shader
    LightPos = lightPos;
}
