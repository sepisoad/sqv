@ctype mat4 hmm_mat4

@vs vs
layout(binding=0) uniform vs_params {
    mat4 mvp;
};

in vec4 position;

void main() {
    gl_Position = mvp * position;
}
@end

@fs fs
out vec4 frag_color;

void main() {
    frag_color = vec4(1.0, 1.0, 1.0, 1.0);
}
@end

@program cube vs fs
