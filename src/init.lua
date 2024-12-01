local gl = require("moongl")
local glfw = require("moonglfw")
local mth = require("moonglmath")

-- Initialize GLFW and create a window
glfw.version_hint(3, 3, "core")
local window = glfw.create_window(800, 600, "Spinning Cube")
glfw.make_context_current(window)
gl.init()

local vertices = {
    -- Front face
    -0.1, -0.1, 0.1, 1.0, 0.0, 0.0,
    0.1, -0.1, 0.1, 0.0, 1.0, 0.0,
    0.1, 0.1, 0.1, 0.0, 0.0, 1.0,
    -0.1, 0.1, 0.1, 1.0, 1.0, 0.0,
    -- Back face
    -0.1, -0.1, -0.1, 0.0, 1.0, 1.0,
    0.1, -0.1, -0.1, 1.0, 0.0, 1.0,
    0.1, 0.1, -0.1, 1.0, 1.0, 1.0,
    -0.1, 0.1, -0.1, 0.0, 0.0, 0.0,
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

-- Create Vertex Array Object and Vertex Buffer Object
local vao = gl.new_vertex_array()
local vbo = gl.new_buffer("array")
local ebo = gl.new_buffer("element array")

gl.buffer_data("array", gl.pack("float", vertices), "static draw")
gl.buffer_data("element array", gl.pack("uint", indices), "static draw")

gl.vertex_attrib_pointer(0, 3, "float", false, 6 * gl.sizeof("float"), 0)
gl.enable_vertex_attrib_array(0)
gl.vertex_attrib_pointer(1, 3, "float", false, 6 * gl.sizeof("float"), 3 * gl.sizeof("float"))
gl.enable_vertex_attrib_array(1)

gl.unbind_vertex_array()

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
local projection = mth.perspective(math.rad(45.0), 800/600, 0.1, 100.0)

-- Main loop
while not glfw.window_should_close(window) do
    glfw.poll_events()
    
    -- Clear the screen
    gl.clear_color(0.2, 0.3, 0.3, 1.0)
    gl.clear("color", "depth")
    
    -- Bind shader program
    gl.use_program(prog)
    
    -- Set transformations
    local time = glfw.get_time()
    local model = mth.rotate(time, 0.5, 1.0, 0.0)
    local view = mth.translate(0.0, 0.0, -3.0)
    
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "model"), true, model)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "view"), true, view)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "projection"), true, projection)
    
    -- Draw the cube
    gl.bind_vertex_array(vao)
    gl.draw_elements("triangles", #indices, "uint", 0)
    
    -- Swap buffers
    glfw.swap_buffers(window)
end

-- Cleanup
gl.delete_program(prog)
gl.delete_vertex_arrays(vao)
gl.delete_buffers(vbo, ebo)
glfw.terminate()
