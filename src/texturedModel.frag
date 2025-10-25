#version 460 core

// === Inputs ===
in vec2 TexCoords;

// === Outputs ===
out vec4 FragColor;

// === Uniforms ===
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_base_color1;
uniform vec3 uBaseColor = vec3(0.0);

void main()
{
    // Sample diffuse texture first
    vec3 color = texture(texture_diffuse1, TexCoords).rgb;

    // Fallback to PBR base color
     if (length(color) < 0.01)
        color = texture(texture_base_color1, TexCoords).rgb;

    FragColor = vec4(color, 1.0);
}
