#version 330 core

out vec4 FragColor;
precision mediump float;

in vec3 v_frag_coord;
in vec3 v_normal;
in vec2 TexCoord;

uniform vec3 u_view_pos;

// Establish light
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

const float eta = 1.52; // Refractive index of the glass

vec3 reflectVector(vec3 I, vec3 N) {
    return reflect(I, N);
}

vec3 refractVector(vec3 I, vec3 N, float eta) {
    return refract(I, N, eta);
}

void main() {
    vec3 N = normalize(v_normal);
    vec3 L = normalize(light.light_pos - v_frag_coord);
    vec3 V = normalize(u_view_pos - v_frag_coord);

    float specular = 0.0;
    if (shininess > 0.0) {
        vec3 R = reflectVector(-L, N);
        float cosTheta = dot(R, V);
        specular = light.specular_strength * pow(max(cosTheta, 0.0), shininess);
    }

    float diffuse = light.diffuse_strength * max(dot(N, L), 0.0);
    float distance = length(light.light_pos - v_frag_coord);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance) * 10.0;
    float calculatedLight = light.ambient_strength + attenuation * (diffuse + specular);

    vec3 textureColor = texture(ourTexture, TexCoord).xyz;
    vec3 glowColor = vec3(0.0, 1.0, 0.0); // Set the glow color (yellow in this example)
    float glowIntensity = 2.0; // Adjust this intensity as needed

    // Refraction calculation
    vec3 viewDirection = normalize(u_view_pos - v_frag_coord);
    vec3 refracted = refractVector(viewDirection, N, eta);

    // Reflection calculation
    vec3 reflected = reflectVector(viewDirection, N);

    // Combine reflection and refraction with Fresnel effect
    float R0 = ((eta - 1.0) * (eta - 1.0)) / ((eta + 1.0) * (eta + 1.0));
    float fresnel = R0 + (1.0 - R0) * pow(1.0 - dot(-viewDirection, N), 2.0);
    vec3 finalColor = mix(refracted, reflected, fresnel);

    FragColor = vec4(finalColor * calculatedLight, 1.0);
}
