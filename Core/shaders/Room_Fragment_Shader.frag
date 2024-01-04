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
    vec3 L = normalize(light.light_pos - v_frag_coord);
    vec3 V = normalize(u_view_pos - v_frag_coord);

    float specular = blinnPhongSpecular(N, L, V);
    float diffuse = light.diffuse_strength * max(dot(N, L), 0.0) * 5;

    float distance = length(light.light_pos - v_frag_coord);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    float totalLight = light.ambient_strength + attenuation * (diffuse + specular) * 4;

    FragColor = vec4(materialColour * vec3(totalLight), 1.0);
}
