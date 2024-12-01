#!/usr/bin/env lua

local gl = require("moongl")
local glfw = require("moonglfw")
local glmath = require("moonglmath")
local new_camera = require("nuklear.camera")

local vec3, mat4 = glmath.vec3, glmath.mat4
local perspective = glmath.perspective
local rad = math.rad

local SCR_WIDTH, SCR_HEIGHT = 800, 600

-- Camera setup
local camera = new_camera(vec3(0.0, 0.0, 3.0))       -- Start slightly further from the cube
local last_x, last_y = SCR_WIDTH / 2, SCR_HEIGHT / 2 -- Initially centered
local first_mouse = true

-- GLFW and OpenGL setup
glfw.version_hint(4, 1, 'core')
local window = glfw.create_window(SCR_WIDTH, SCR_HEIGHT, "GLSL 410 Cube")
glfw.make_context_current(window)
gl.init()

glfw.set_framebuffer_size_callback(window, function(window, w, h)
    gl.viewport(0, 0, w, h)
    SCR_WIDTH, SCR_HEIGHT = w, h
end)

glfw.set_cursor_pos_callback(window, function(window, xpos, ypos)
    -- whenever the mouse moves, this callback is called
    -- if first_mouse then
    --     last_x, last_y = xpos, ypos
    --     first_mouse = false
    -- end
    local xoffset = xpos - last_x
    local yoffset = last_y - ypos -- reversed since y-coordinates go from bottom to top
    last_x, last_y = xpos, ypos
    camera:process_mouse(xoffset, yoffset, true)
end)

glfw.set_scroll_callback(window, function(window, xoffset, yoffset)
    -- whenever the mouse scroll wheel scrolls, this callback is called
    camera:process_scroll(yoffset)
end)

-- tell GLFW to capture our mouse:
glfw.set_input_mode(window, 'cursor', 'disabled')

-- configure global opengl state
gl.enable('depth test')

-- Shader code
local vsh_code = [[
#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
}
]]

local fsh_code = [[
#version 410 core

in vec3 vertexColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
]]

local vsh = gl.create_shader('vertex')
gl.shader_source(vsh, vsh_code)
gl.compile_shader(vsh)
assert(gl.get_shader(vsh, 'compile status'), gl.get_shader_info_log(vsh))

local fsh = gl.create_shader('fragment')
gl.shader_source(fsh, fsh_code)
gl.compile_shader(fsh)
assert(gl.get_shader(fsh, 'compile status'), gl.get_shader_info_log(fsh))

local program = gl.create_program()
gl.attach_shader(program, vsh)
gl.attach_shader(program, fsh)
gl.link_program(program)
assert(gl.get_program(program, 'link status'), gl.get_program_info_log(program))
gl.use_program(program)

-- Cube vertices and colors
local positions = {
    -- Front face
    -0.1, -0.1, 0.1,
    0.1, -0.1, 0.1,
    0.1, 0.1, 0.1,
    -0.1, 0.1, 0.1,
    -- Back face
    -0.1, -0.1, -0.1,
    0.1, -0.1, -0.1,
    0.1, 0.1, -0.1,
    -0.1, 0.1, -0.1,
}

local colors = {
    -- Front face colors (red, green, blue, yellow)
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    1.0, 1.0, 0.0,
    -- Back face colors (cyan, magenta, white, black)
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

-- Vertex buffers
local vao = gl.new_vertex_array()
local position_vbo = gl.new_buffer('array')
gl.buffer_data('array', gl.pack('float', positions), 'static draw')
gl.vertex_attrib_pointer(0, 3, 'float', false, 0, 0)
gl.enable_vertex_attrib_array(0)

local color_vbo = gl.new_buffer('array')
gl.buffer_data('array', gl.pack('float', colors), 'static draw')
gl.vertex_attrib_pointer(1, 3, 'float', false, 0, 0)
gl.enable_vertex_attrib_array(1)

local index_buffer = gl.new_buffer('element array')
gl.buffer_data('element array', gl.pack('uint', indices), 'static draw')

gl.unbind_vertex_array()

-- Uniform locations
local locs = {}
local function getloc(name) locs[name] = gl.get_uniform_location(program, name) end
getloc("model")
getloc("view")
getloc("projection")

-- Rendering loop
gl.clear_color(0.5, 0.5, 0.5, 1.0)
gl.enable('depth test') -- Enable depth testing
while not glfw.window_should_close(window) do
    glfw.poll_events()

    -- Corrected Projection Matrix (near = 0.1, far = 100.0)
    local view = camera:view()
    local model = glmath.translate(vec3(0.0, 0.0, -100.0)) -- Move cube slightly back
    local projection = perspective(rad(camera.zoom), SCR_WIDTH/SCR_HEIGHT, -100, 100.0)

    -- Set uniforms
    gl.uniform_matrix4f(locs.projection, false, projection)
    gl.uniform_matrix4f(locs.view, false, view)
    gl.uniform_matrix4f(locs.model, false, model)

    gl.clear('color', 'depth')
    gl.bind_vertex_array(vao)
    gl.draw_elements('triangles', #indices, 'uint', 0)
    gl.unbind_vertex_array()

    glfw.swap_buffers(window)
end

-- Cleanup
gl.delete_program(program)
gl.delete_vertex_arrays(vao)
gl.delete_buffers(position_vbo, color_vbo, index_buffer)
