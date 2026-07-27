#version 330 core

out vec4 FragColor;

uniform vec4 hudColor;

void main()
{
    FragColor = vec4(hudColor);
}