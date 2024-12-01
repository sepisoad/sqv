#version 410 core

in vec3 vertexColor;  // Interpolated color from the vertex shader
uniform vec3 viewPos; // Camera position

out vec4 FragColor;

void main()
{
    // Example: Use the viewPos for lighting calculation (or adjust as needed)
    vec3 lightDirection = normalize(viewPos - vec3(0.0, 0.0, 0.0));  // Assume a light source at (0,0,0)
    vec3 lightingEffect = vertexColor * max(dot(lightDirection, vec3(0.0, 0.0, 1.0)), 0.0);

    // Output the final color
    FragColor = vec4(lightingEffect, 1.0);
}
