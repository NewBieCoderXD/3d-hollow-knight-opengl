#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    vec4 color = texture(skybox, TexCoords);
    FragColor = vec4(color.xyz,0.0);
}