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
    float constant;
    float linear;
    float quadratic;
};

#define MAX_LIGHTS 8
uniform Light lights[MAX_LIGHTS]; // Array of lights

uniform Light light;
uniform float shininess;
uniform vec3 materialColour;

float blinnPhongSpecular(vec3 N, vec3 L, vec3 V) {
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    return light.specular_strength * spec;
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_view_pos - v_frag_coord);

    vec3 totalLight = vec3(0.0); // Accumulator for total light

    // Assuming you have already set values for lights[] array in C++ application

    // Light 1
    vec3 L = normalize(lights[0].light_pos - v_frag_coord);
    float specular = blinnPhongSpecular(N, L, V);
    float diffuse = lights[0].diffuse_strength * max(dot(N, L), 0.0);
    float distance = length(lights[0].light_pos - v_frag_coord) * 0.3;
    float attenuation = 1.0 / (lights[0].constant + lights[0].linear * distance + lights[0].quadratic * distance * distance);
    totalLight += attenuation * (lights[0].ambient_strength + (diffuse + specular));

    // Light 2
    L = normalize(lights[1].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[1].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[1].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[1].constant + lights[1].linear * distance + lights[1].quadratic * distance * distance);
    totalLight += attenuation * (lights[1].ambient_strength + (diffuse + specular));

    // Light 3
    L = normalize(lights[2].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[2].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[2].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[2].constant + lights[2].linear * distance + lights[2].quadratic * distance * distance);
    totalLight += attenuation * (lights[2].ambient_strength + (diffuse + specular));

    // Light 4
    L = normalize(lights[3].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[3].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[3].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[3].constant + lights[3].linear * distance + lights[3].quadratic * distance * distance);
    totalLight += attenuation * (lights[3].ambient_strength + (diffuse + specular));

    // Light 5
    L = normalize(lights[4].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[4].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[4].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[4].constant + lights[4].linear * distance + lights[4].quadratic * distance * distance);
    totalLight += attenuation * (lights[4].ambient_strength + (diffuse + specular));

    // Light 6
    L = normalize(lights[5].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[5].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[5].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[5].constant + lights[5].linear * distance + lights[5].quadratic * distance * distance);
    totalLight += attenuation * (lights[5].ambient_strength + (diffuse + specular));

    // Light 7
    L = normalize(lights[6].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[6].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[6].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[6].constant + lights[6].linear * distance + lights[6].quadratic * distance * distance);
    totalLight += attenuation * (lights[6].ambient_strength + (diffuse + specular));

    // Light 8
    L = normalize(lights[7].light_pos - v_frag_coord);
    specular = blinnPhongSpecular(N, L, V);
    diffuse = lights[7].diffuse_strength * max(dot(N, L), 0.0);
    distance = length(lights[7].light_pos - v_frag_coord) * 0.3;
    attenuation = 1.0 / (lights[7].constant + lights[7].linear * distance + lights[7].quadratic * distance * distance);
    totalLight += attenuation * (lights[7].ambient_strength + (diffuse + specular));

    FragColor = vec4(materialColour * totalLight, 1.0);
}

