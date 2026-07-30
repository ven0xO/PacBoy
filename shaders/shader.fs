#version 330 core
out vec4 FragColor;

in vec3 worldNormal;

uniform vec3 objectColor;
uniform vec3 emissionColor;
uniform float emissionStrength;
uniform float objectAlpha;

void main()
{
    const vec3 lightDirection = normalize(vec3(-0.45, 0.9, 0.35));
    const float ambientStrength = 0.68;
    float diffuseStrength = max(dot(normalize(worldNormal), lightDirection), 0.0);

    vec3 litColor = objectColor * (ambientStrength + 0.32 * diffuseStrength);
    vec3 finalColor = litColor + emissionColor * emissionStrength;

    FragColor = vec4(finalColor, objectAlpha);
}
