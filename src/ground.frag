// Fragment Shader (ground.fs)
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D groundTexture;

void main()
{
  // FragColor = texture(groundTexture, TexCoords);
  FragColor = vec4(17, 97, 20,255)/255.0;
}