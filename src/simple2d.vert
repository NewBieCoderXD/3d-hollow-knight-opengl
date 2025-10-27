#version 330 core

// Input vertex position
layout(location = 0) in vec2 aPos;

// Uniform to transform quad (e.g., for position, scale)
uniform mat4 model;
uniform mat4 projection;

void main()
{
    // Convert 2D position to vec4 (z = 0, w = 1) and apply transforms
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}
