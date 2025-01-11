@vs vs
in vec3 position;
in vec2 aTexCoord;
  
out vec2 TexCoord;

void main() {
    gl_Position = vec4(position, 1.0);
    TexCoord = aTexCoord;
}
@end

@fs fs
out vec4 FragColor;

in vec2 TexCoord;

uniform layout(binding=0) texture2D _ourTexture;
uniform layout(binding=1) sampler ourTexture_smp;
#define ourTexture sampler2D(_ourTexture, ourTexture_smp)

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
@end

@program simple vs fs