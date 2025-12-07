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

float hash22(vec2 p) {
    // Generate a pseudo-random value based on the input coordinates
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    
    // Multiply by a large number and add a small offset
    p3 += dot(p3, p3.yzx + 33.33);
    
    // Take the fractional part, which is our pseudo-random hash
    return fract((p3.x + p3.y) * p3.z);
}

float noise(vec2 p) {
    vec2 ip = floor(p);    // Integer part of the coordinate (grid corner)
    vec2 fp = fract(p);    // Fractional part (position within the grid cell)

    // Hash the four corners of the grid cell
    float c00 = hash22(ip);
    float c10 = hash22(ip + vec2(1.0, 0.0));
    float c01 = hash22(ip + vec2(0.0, 1.0));
    float c11 = hash22(ip + vec2(1.0, 1.0));

    // Smoothstep function for interpolation (smoother than linear)
    vec2 u = fp * fp * (3.0 - 2.0 * fp); 
    // vec2 u = fp; // For linear interpolation

    // Interpolate horizontally, then vertically
    float a = mix(c00, c10, u.x); // Interpolate between c00/c10 and c01/c11
    float b = mix(c01, c11, u.x);

    // Final result
    return mix(a, b, u.y);
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

    // float dir = sign(sin(pos.x * 4.0 + pos.z * 2.0));
    // float speed = 0.8 + abs(sin(pos.z * 0.75)) * 0.9;
    // float amp = 0.10 + fract(sin(pos.x * 32.1) * 43758.5453) * 0.5;

    float noise_sample = noise(pos.xz * 0.1);
    float wind_strength = 0.5 + noise_sample * 0.5;

    float slow_sway = cos(uTime * 0.5 + pos.x * 0.1) * wind_strength * 0.8 * pow(localY, 3.0);
    float fast_sway = cos(uTime * 4.0 + pos.x * 5.0) * wind_strength * 0.1 * pow(localY, 1.5);
    float sway = slow_sway + fast_sway;

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
    vShade = 0.5 + 0.5 * (localY);
}