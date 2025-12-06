
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;
layout(location = 2) in vec4 instancePosPhase; // x,y,z,phase

uniform mat4 view;
uniform mat4 projection;
uniform float uTime;

out float vHeightT;
out float vShade;

void main() {
    vec3 pos = instancePosPhase.xyz;
    float phase = instancePosPhase.w;

    float localY = aTex.y;

    float dir = sign(sin(pos.x * 4.0 + pos.z * 2.0));
    float speed = 0.8 + abs(sin(pos.z * 0.75)) * 0.9;
    float amp = 0.03 + fract(sin(pos.x * 32.1) * 43758.5453) * 0.05;

    float sway = cos(uTime * speed + pos.x * 3.1 + phase) *
                  amp * localY * dir;

    vec3 worldPos = vec3(pos.x + aPos.x + sway,
                          pos.y + aPos.y,
                          pos.z);

    gl_Position = projection * view  * vec4(worldPos, 1.0);

    vHeightT = localY;
    vShade = 0.5 + 0.5 * (1.0 - localY);
}