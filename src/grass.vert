#version 330 core
layout(location = 0) in vec3 aPos; // Fixed to vec3 (Position)
layout(location = 1) in vec2 aTex; // (Texcoord)

// New Uniforms for Procedural Generation
uniform int uGridX;
uniform float uSpacing;
uniform vec2 uOffset; // (offX, offZ)

uniform mat4 view;
uniform mat4 projection;
uniform float uTime;

out float vHeightT;
out float vShade;

float hash1D(float n) {
    return fract(sin(n) * 43758.5453123);
}

void main() {
    // ------------------------------------------------------------
    // 1. Procedural Instance Position and Phase Generation (REPLACES INSTANCE VBO)
    // ------------------------------------------------------------
    int instanceID = gl_InstanceID;
    
    // Calculate 2D grid coordinates from 1D Instance ID
    int gridX = instanceID % uGridX;
    int gridZ = instanceID / uGridX;

    // Calculate base world position
    float px = uOffset.x + float(gridX) * uSpacing;
    float pz = uOffset.y + float(gridZ) * uSpacing;
    
    // Use the base position to generate deterministic random values
    float rand_seed = px * 13.7 + pz * 79.1;
    
    // Jitter the position slightly for a natural look
    float jitter = (hash1D(rand_seed) * 2.0 - 1.0) * 0.1; // Range [-0.1, 0.1]
    
    // Calculate a random phase offset for the wind animation
    float phase = hash1D(rand_seed + 123.4) * 6.28318; // Range [0, 2*PI]

    vec3 pos = vec3(px + jitter, 0.0, pz + jitter);
    // ------------------------------------------------------------
    
    // Wind Animation (Your original logic, but now using the procedural phase)
    float localY = aTex.y;

    float dir = sign(sin(pos.x * 4.0 + pos.z * 2.0));
    float speed = 0.8 + abs(sin(pos.z * 0.75)) * 0.9;
    float amp = 0.03 + fract(sin(pos.x * 32.1) * 43758.5453) * 0.05;

    float sway = cos(uTime * speed + pos.x * 3.1 + phase) *
                 amp * localY * dir;

    // Apply sway to the blade's X (width) and Z (depth) and then apply instance position
    vec3 localPos = vec3(aPos.x + sway, 
                         aPos.y, 
                         aPos.z + sway * 0.1); // Add a small Z sway for depth perception

    // World position is local position + instance offset
    vec3 worldPos = localPos + pos;

    float scale_x = 1+0.5*(hash1D(rand_seed+1) * 2.0 - 1.0);
    float scale_y = 1+0.2*(hash1D(rand_seed+2) * 2.0 - 1.0);
    float scale_z = 1+0.5*(hash1D(rand_seed+3) * 2.0 - 1.0);

    mat4 scaleMatrix = mat4(1.0); 

    scaleMatrix[0][0] = scale_x;
    scaleMatrix[1][1] = scale_y;
    scaleMatrix[2][2] = scale_z; 

    gl_Position = projection * view * scaleMatrix * vec4(worldPos, 1.0); // Correct matrix order

    vHeightT = localY;
    vShade = 0.5 + 0.5 * (1.0 - localY);
}