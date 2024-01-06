#version 330 core

out vec4 FragColor;
precision mediump float;

in vec3 v_frag_coord;
in vec3 v_normal;
in vec2 TexCoord;

uniform vec3 u_view_pos;

// establish light
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
uniform float selected;

uniform sampler2D ourTexture;

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(light.light_pos - v_frag_coord);
    vec3 V = normalize(u_view_pos - v_frag_coord);

    float specular = 0.0;
    if (shininess > 0.0) {
        vec3 R = reflect(-L, N);
        float cosTheta = dot(R, V);
        specular = light.specular_strength * pow(max(cosTheta, 0.0), shininess);
    }

    float diffuse = light.diffuse_strength * max(dot(N, L), 0.0);
    float distance = length(light.light_pos - v_frag_coord);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    float calculatedLight = light.ambient_strength + attenuation * (diffuse + specular);

    vec3 textureColor = texture(ourTexture, TexCoord).xyz;
    vec3 materialColour = vec3(1.0, 0.5, 0.0); // Orange color

    vec3 finalColor = vec3(1.0);

    if (selected == 1.0) {
        finalColor = mix(materialColour, textureColor, calculatedLight);
    } else {
        finalColor = textureColor * calculatedLight;
    }

    FragColor = vec4(finalColor, 1.0);
}
