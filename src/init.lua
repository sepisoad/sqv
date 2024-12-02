local gl = require("moongl")
local glfw = require("moonglfw")
local mth = require("moonglmath")
local mdl = require("src.decoder")
local pp = require("pprint.pprint")

local window_width = 500
local window_height = 400

-- Initialize GLFW
glfw.version_hint(3, 3, "core")
local window = glfw.create_window(window_width, window_height, "Wireframe Cube")
glfw.make_context_current(window)
gl.init()


local function create_position()
    local array = {}
    for _, value in ipairs(mdl.frames[1].Vertices) do
        table.insert(array, (value.X * mdl.header.Scale.X) + mdl.header.ScaleOrigin.X)
        table.insert(array, (value.Y * mdl.header.Scale.Y) + mdl.header.ScaleOrigin.Y)
        table.insert(array, (value.Z * mdl.header.Scale.Z) + mdl.header.ScaleOrigin.Z)
    end
    return array
end

local function create_indices()
    local array = {}
    for _, value in ipairs(mdl.triangles) do
        table.insert(array, value.X)
        table.insert(array, value.Y)
        table.insert(array, value.Z)
    end
    return array
end

-- LOG
local positions = create_position()
local indices = create_indices()

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

-- Compile and use shaders
local prog = gl.make_program(
    "vertex", "res/shaders/simple.vert",
    "fragment", "res/shaders/simple.frag"
)

-- Enable wireframe mode
gl.polygon_mode("front and back", "line")

-- Main rendering loop
local projection = mth.perspective(math.rad(45.0), window_width / window_height, 0.1, 10000.0)

local t0 = glfw.now()
local angle, speed = 0, math.pi / 3

while not glfw.window_should_close(window) do
    glfw.poll_events()

    gl.clear_color(0.0, 0.0, 0.0, 1.0)
    gl.clear("color", "depth")

    local t = glfw.now()
    local dt = t - t0
    t0 = t
    angle = angle + speed * dt
    if angle >= 2 * math.pi then angle = angle - 2 * math.pi end

    -- Set transformations    
    local view = mth.translate(0.0, 0.0, -100.0)
    local model =
        mth.translate(0, -20, 0)
        * mth.rotate(angle, 0, 1, 0)
        * mth.rotate(math.rad(-90), 1, 0, 0)
        * mth.rotate(math.rad(90), 0, 0, 1)

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
gl.delete_vertex_arrays(vao)
gl.delete_buffers(vbo, ebo)
gl.clean_program(prog)
glfw.terminate()
