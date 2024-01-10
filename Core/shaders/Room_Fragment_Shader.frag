#version 330 core
out vec4 FragColor;
precision mediump float;

in vec3 v_frag_coord;
in vec3 v_normal;

uniform vec3 u_view_pos;

struct Light {
    vec3 light_pos;
    float ambient_strength;
    float diffuse_strength;
    float specular_strength;
    float constant; //attenuation factors
    float linear;
    float quadratic;
};

#define MAX_LIGHTS 9
uniform Light lights[MAX_LIGHTS]; // Array of lights

uniform Light light;
uniform float shininess;
uniform vec3 materialColour;

float blinnPhongSpecular(vec3 N, vec3 L, vec3 V) {
    vec3 H = normalize(L + V); //half-vector
    float spec = pow(max(dot(N, H), 0.0), shininess);
    return light.specular_strength * spec;
}

void main() {
    vec3 N = normalize(v_normal); //normalized interpolated normal vector
    vec3 V = normalize(u_view_pos - v_frag_coord); //normalized view direction vector

    vec3 totalLight = vec3(0.0); // Accumulator for total light

    for (int i = 0; i < MAX_LIGHTS; ++i) {
        vec3 L = normalize(lights[i].light_pos - v_frag_coord); //normalized light source vector
        float specular = blinnPhongSpecular(N, L, V) * 3;
        float diffuse = lights[i].diffuse_strength * max(dot(N, L), 0.0) * 2;
        float distance = length(lights[i].light_pos - v_frag_coord) * 0.15;
        float attenuation = 1.0 / (lights[i].constant + lights[i].linear * distance + lights[i].quadratic * distance * distance);
        totalLight += attenuation * (lights[i].ambient_strength + (diffuse + specular));
    }

    FragColor = vec4(materialColour * totalLight, 1.0);
}

