#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex;
layout(location = 5) in ivec4 boneIds; 
layout(location = 6) in vec4 weights;
	
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
	
const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];
	
out vec2 TexCoords;
	
void main()
{
     vec4 totalPosition = vec4(0.0f);
     vec3 totalNormal = vec3(0.0f);
     
     for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
     {
         // Basic check to see if this slot is even used
         if(weights[i] == 0.0f)
             continue;
         
         // Safety checks (optional, but good practice)
         if(boneIds[i] < 0 || boneIds[i] >= MAX_BONES) 
             continue;
             
         mat4 boneTransform = finalBonesMatrices[boneIds[i]];
         float weight = weights[i];
         
        // Accumulate Position: Final_Transform * (Vertex_Position * Weight)
        // Note: The matrix multiplication with the position already includes the weight
        totalPosition += (boneTransform * vec4(pos, 1.0f)) * weight;
        
        // Accumulate Normal: Final_Transform * (Vertex_Normal * Weight)
        // Use the inverse transpose of the upper-left 3x3 matrix for normal transformation
        totalNormal += mat3(boneTransform) * norm * weight;
    }
    
    // Fallback: If no bones influence the vertex (all weights are 0), use the model matrix.
    // This isn't necessary if weights are properly normalized, but prevents zero-division/bad normals.
    if (length(totalNormal) < 0.0001) {
        // If weights sum to zero, this is a static vertex, so transform it normally.
        totalPosition = vec4(pos, 1.0f); 
        totalNormal = norm;
    }
    
     mat4 viewModel = view * model;
     gl_Position =  projection * viewModel * totalPosition;
     TexCoords = tex;

    //vec4 totalPosition = vec4(pos,1.0f);
    //mat4 viewModel = view * model;
    //gl_Position =  projection * viewModel * totalPosition;
    //TexCoords = tex;
}