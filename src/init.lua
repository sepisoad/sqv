local gl = require("moongl")
local glfw = require("moonglfw")
local mth = require("moonglmath")

-- Initialize GLFW
glfw.version_hint(3, 3, "core")
local window = glfw.create_window(800, 600, "Wireframe Cube")
glfw.make_context_current(window)
gl.init()

-- Shader program
local vertex_shader = [[
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
]]

local fragment_shader = [[
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 1.0, 1.0, 1.0); // White color
}
]]

-- Cube data
local positions = {
    -- Cube vertices
    -0.5, -0.5, -0.5,   0.5, -0.5, -0.5,   0.5,  0.5, -0.5,  -0.5,  0.5, -0.5,
    -0.5, -0.5,  0.5,   0.5, -0.5,  0.5,   0.5,  0.5,  0.5,  -0.5,  0.5,  0.5,
}

local indices = {
    -- Cube faces
    0, 1, 2,  2, 3, 0,
    4, 5, 6,  6, 7, 4,
    0, 3, 7,  7, 4, 0,
    1, 5, 6,  6, 2, 1,
    3, 2, 6,  6, 7, 3,
    0, 1, 5,  5, 4, 0,
}

-- Create VAO and buffers
local vao = gl.new_vertex_array()
local vbo = gl.new_buffer("array")
local ebo = gl.new_buffer("element array")

gl.bind_vertex_array(vao)

-- Upload vertex data
gl.bind_buffer("array", vbo)
gl.buffer_data("array", gl.pack("float", positions), "static draw")
gl.vertex_attrib_pointer(0, 3, "float", false, 0, 0)
gl.enable_vertex_attrib_array(0)

-- Upload index data
gl.bind_buffer("element array", ebo)
gl.buffer_data("element array", gl.pack("uint", indices), "static draw")

gl.unbind_vertex_array()

-- Compile shaders
local vertex = gl.create_shader("vertex")
gl.shader_source(vertex, vertex_shader)
gl.compile_shader(vertex)

local fragment = gl.create_shader("fragment")
gl.shader_source(fragment, fragment_shader)
gl.compile_shader(fragment)

local prog = gl.create_program()
gl.attach_shader(prog, vertex)
gl.attach_shader(prog, fragment)
gl.link_program(prog)

-- Enable wireframe mode
gl.polygon_mode("front and back", "line")

-- Main rendering loop
local projection = mth.perspective(math.rad(45.0), 800 / 600, 0.1, 100.0)

while not glfw.window_should_close(window) do
    glfw.poll_events()

    gl.clear_color(0.0, 0.0, 0.0, 1.0)
    gl.clear("color", "depth")

    -- Set transformations
    local time = glfw.get_time()
    local model = mth.rotate(time, 0.3, 0.5, 0.7)
    local view = mth.translate(0.0, 0.0, -2.0)

    gl.use_program(prog)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "model"), true, model)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "view"), true, view)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "projection"), true, projection)

    -- Draw wireframe cube
    gl.bind_vertex_array(vao)
    gl.draw_elements("triangles", #indices, "uint", 0)

    glfw.swap_buffers(window)
end

-- Cleanup
gl.delete_program(prog)
gl.delete_vertex_arrays(vao)
gl.delete_buffers(vbo, ebo)
glfw.terminate()
