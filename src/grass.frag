#version 330 core
in float vHeightT;
in float vShade;

out vec4 FragColor;

void main() {
  vec3 baseGreen = vec3(0.06, 0.45, 0.06);
  vec3 tipGreen  = vec3(0.35, 0.8, 0.25);
  vec3 col = mix(tipGreen, baseGreen, vHeightT);
  col *= vShade;
  FragColor = vec4(col, 1.0);
}