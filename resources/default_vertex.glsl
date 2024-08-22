#version 330 core 
layout (location = 0) in vec3 aPos;

out vec3 FragPos; // not yet needed as the chunk isnt moving 
out vec3 Normal;

uniform mat4 u_transform; 

void main() { 
    gl_Position = u_transform * vec4(aPos, 1.0);
    Normal = aPos;
    FragPos = aPos;
} 
