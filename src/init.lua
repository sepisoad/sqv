local gl = require("moongl")
local glfw = require("moonglfw")
local mth = require("moonglmath")

-- Initialize GLFW
glfw.version_hint(3, 3, "core")

-- Create a window
local window = glfw.create_window(500, 400, "Spinning Cube")
glfw.make_context_current(window)

-- Initialize OpenGL
gl.init()

-- Shader program
local vertex_shader = [[
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 ourColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor;
}
]]

local fragment_shader = [[
#version 330 core
in vec3 ourColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
]]

-- Cube data: positions, colors, and indices
local positions = {
    -- Front face
    -0.1, -0.1,  0.1,
     0.1, -0.1,  0.1,
     0.1,  0.1,  0.1,
    -0.1,  0.1,  0.1,
    -- Back face
    -0.1, -0.1, -0.1,
     0.1, -0.1, -0.1,
     0.1,  0.1, -0.1,
    -0.1,  0.1, -0.1,
}

local colors = {
    -- Front face colors
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 1.0, 0.0,
    -- Back face colors
    0.0, 1.0, 1.0,
    1.0, 0.0, 1.0,
    1.0, 1.0, 1.0,
    0.0, 0.0, 0.0,
}

local indices = {
    -- Front face
    0, 1, 2, 2, 3, 0,
    -- Back face
    4, 5, 6, 6, 7, 4,
    -- Left face
    0, 3, 7, 7, 4, 0,
    -- Right face
    1, 2, 6, 6, 5, 1,
    -- Top face
    3, 2, 6, 6, 7, 3,
    -- Bottom face
    0, 1, 5, 5, 4, 0,
}

-- Create VAO and buffers
local vao = gl.new_vertex_array()
local position_vbo = gl.new_buffer("array")
local color_vbo = gl.new_buffer("array")
local ebo = gl.new_buffer("element array")

-- Upload position data to GPU
gl.bind_buffer("array", position_vbo)
gl.buffer_data("array", gl.pack("float", positions), "static draw")
gl.vertex_attrib_pointer(0, 3, "float", false, 0, 0)
gl.enable_vertex_attrib_array(0)

-- Upload color data to GPU
gl.bind_buffer("array", color_vbo)
gl.buffer_data("array", gl.pack("float", colors), "static draw")
gl.vertex_attrib_pointer(1, 3, "float", false, 0, 0)
gl.enable_vertex_attrib_array(1)

-- Upload indices to GPU
gl.bind_buffer("element array", ebo)
gl.buffer_data("element array", gl.pack("uint", indices), "static draw")

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

-- Configure OpenGL
gl.enable("depth test")

-- Transformation matrices
local projection = mth.perspective(math.rad(45.0), 800 / 600, 0.1, 100.0)

gl.bind_vertex_array(vao)

-- Main loop
while not glfw.window_should_close(window) do
    glfw.poll_events()

    -- Clear the screen
    gl.clear_color(0.0, 0.0, 0.0, 1.0)
    gl.clear("color", "depth")

    -- Set transformations
    local time = glfw.get_time()
    local model = mth.rotate(time, 0.1, 1.0, 0.1)
    local view = mth.translate(0.0, 0.0, -1.0)

    gl.use_program(prog)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "model"), true, model)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "view"), true, view)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "projection"), true, projection)

    -- Draw the cube
    -- 
    gl.draw_elements("triangles", #indices, "uint", 0)

    -- Swap buffers
    glfw.swap_buffers(window)
end

-- Cleanup
gl.delete_program(prog)
gl.delete_vertex_arrays(vao)
gl.delete_buffers(position_vbo, color_vbo, ebo)
glfw.terminate()
