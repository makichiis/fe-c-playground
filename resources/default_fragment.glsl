#version 330 core 
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 u_lightpos;

void main() {
    vec3 light_color = vec3(1.0f, 1.0f, 1.0f);
    vec3 object_color = vec3(0.35f, 0.35f, 0.35f);

    float ambient_strength = 0.4;
    vec3 ambient = ambient_strength * light_color;

    vec3 norm = normalize(Normal);
    //vec3 light_dir = normalize(u_lightpos - FragPos);
    vec3 light_dir = normalize(u_lightpos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * light_color;

    vec3 result = (ambient + diffuse) * object_color;
    FragColor = vec4(result, 1.0);

    // cyan color  
    // FragColor = vec4(0.5f, 0.0f, 0.5f, 1.0f);
    // funne editor color it looks kinda like blender 
    // FragColor = vec4(0.3f, 0.3f, 0.35f, 1.0f);
    //FragColor = vec4(0.35f, 0.35f, 0.35f, 1.0f);
}

