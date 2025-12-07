// Fragment Shader (ground.fs)
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D groundTexture;

void main()
{
  // FragColor = texture(groundTexture, TexCoords);

  // float vary_x = 0.05*sin((TexCoords.x*2.5+TexCoords.y)*3.0);
  // float vary_z = 0.05*sin((TexCoords.x+TexCoords.y*2.0)*3.0);
  // float darkness = (0.5+0.05*sin((TexCoords.x+TexCoords.y)*3.0));
  FragColor = vec4(0.25, 0.9, 0.25,1.0)*0.4;
  // FragColor = vec4(0.25+vary_x, 0.9, 0.25+vary_z,1.0)*darkness;
}