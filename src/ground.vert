// Vertex Shader (ground.vs)
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

uniform mat4 model; // Set this to identity matrix or use it for minor shifts
uniform mat4 view;
uniform mat4 projection;
uniform float scaleFactor; // e.g., 1000.0 or 10000.0
uniform float tileFactor;  // e.g., 100.0 or 500.0

out vec2 TexCoords;

void main()
{
    // 1. Apply massive scaling to the vertex position
    vec3 scaledPos = aPos * scaleFactor; 
    
    // 2. Set the final position
    gl_Position = projection * view * model * vec4(scaledPos, 1.0);
    
    // 3. Calculate highly tiled texture coordinates
    // We multiply the base texture coordinates by the tile factor
    TexCoords = aTexCoords * tileFactor;
}