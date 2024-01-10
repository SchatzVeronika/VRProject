#version 330 core

in vec3 position;
in vec2 tex_coord;
in vec3 normal;

out vec3 v_frag_coord;
out vec3 v_normal;
out vec2 TexCoord;
out vec3 FragPos; // Pass the fragment position to the fragment shader
out vec3 LightPos; // Pass the light position to the fragment shader

uniform mat4 M; //model
uniform mat4 itM; //inverse transposed model
uniform mat4 V; //view
uniform mat4 P; //projection
uniform vec3 lightPos; // The position of the light source

void main() {
    vec4 frag_coord = M * vec4(position, 1.0);
    gl_Position = P * V * M * vec4(position, 1);

    v_normal = vec3(itM * vec4(normal, 0.0)); //cancels out non-uniform scaling part of the original model matrix (maintains orthogonality)
    v_frag_coord = frag_coord.xyz;
    TexCoord = tex_coord;

    // Calculate the world-space position of the fragment
    FragPos = vec3(M * vec4(position, 1.0));

    // Pass the light position to the fragment shader
    LightPos = lightPos;
}
