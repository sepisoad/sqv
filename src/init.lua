local gl = require("moongl")
local glfw = require("moonglfw")
local glm = require("moonglmath")
local mdl = require("src.decoder")
local pp = require("pprint.pprint")

local window_width = 300
local window_height = 250

-- Initialize GLFW
glfw.version_hint(3, 3, "core")
local window = glfw.create_window(window_width, window_height, "Correct Winding Order")
glfw.make_context_current(window)
gl.init()

-- Helper to generate random colors
local function random_color()
    return { math.random(), math.random(), math.random() }
end

-- Generate position data
local function create_position(vertices)
    local array = {}
    for _, vertex in ipairs(vertices) do
        local sclae = mdl.header.Scale
        local origin = mdl.header.Origin
        table.insert(array, (vertex.x * sclae.x) + origin.x)
        table.insert(array, (vertex.y * sclae.y) + origin.y)
        table.insert(array, (vertex.z * sclae.z) + origin.z)
    end
    return array
end

-- Generate index data with correct winding order based on the triangle side
local function create_indices_with_side(triangles)
    local array = {}
    for _, triangle in ipairs(triangles) do
        if triangle.FrontFace then
            -- Clockwise (flip winding order)
            table.insert(array, triangle.Vec.x)
            table.insert(array, triangle.Vec.z)
            table.insert(array, triangle.Vec.y)
        else
            -- Counter-clockwise (front-facing)
            table.insert(array, triangle.Vec.x)
            table.insert(array, triangle.Vec.y)
            table.insert(array, triangle.Vec.z)
        end
    end
    return array
end

-- Expand vertices and indices to ensure unique vertices per triangle
local function expand_vertices_and_indices(vertices, indices)
    local expanded_vertices = {}
    local expanded_indices = {}
    local current_index = 0

    for i = 1, #indices, 3 do
        for j = 0, 2 do                             -- For each vertex in the triangle
            local vertex_index = indices[i + j] + 1 -- Lua is 1-based
            pp(vertex_index)

            table.insert(expanded_vertices, vertices[(vertex_index - 1) * 3 + 1]) -- X
            table.insert(expanded_vertices, vertices[(vertex_index - 1) * 3 + 2]) -- Y
            table.insert(expanded_vertices, vertices[(vertex_index - 1) * 3 + 3]) -- Z
            table.insert(expanded_indices, current_index)
            current_index = current_index + 1
        end
    end

    return expanded_vertices, expanded_indices
end

-- Create random colors for each triangle
local function create_triangle_colors(num_triangles)
    local array = {}
    for _ = 1, num_triangles do
        local color = random_color()
        for _ = 1, 3 do                   -- Same color for all 3 vertices of the triangle
            table.insert(array, color[1]) -- R
            table.insert(array, color[2]) -- G
            table.insert(array, color[3]) -- B
        end
    end
    return array
end

-- Generate data
local positions = create_position(mdl.frames[1].Vertices)
local indices = create_indices_with_side(mdl.triangles)
pp(positions)
pp(indices)
local expanded_positions, expanded_indices = expand_vertices_and_indices(positions, indices)
local num_triangles = #expanded_indices / 3
local colors = create_triangle_colors(num_triangles)

-- Create VAO and buffers
local vao = gl.new_vertex_array()
local vbo_position = gl.new_buffer("array")
local vbo_color = gl.new_buffer("array")
local ebo = gl.new_buffer("element array")

gl.bind_vertex_array(vao)

-- Upload position data
gl.bind_buffer("array", vbo_position)
gl.buffer_data("array", gl.pack("float", expanded_positions), "static draw")
gl.vertex_attrib_pointer(0, 3, "float", false, 0, 0) -- Layout location 0
gl.enable_vertex_attrib_array(0)

-- Upload color data
gl.bind_buffer("array", vbo_color)
gl.buffer_data("array", gl.pack("float", colors), "static draw")
gl.vertex_attrib_pointer(1, 3, "float", false, 0, 0) -- Layout location 1
gl.enable_vertex_attrib_array(1)

-- Upload index data
gl.bind_buffer("element array", ebo)
gl.buffer_data("element array", gl.pack("uint", expanded_indices), "static draw")

gl.unbind_vertex_array()

-- Compile and use shaders
local prog = gl.make_program(
    "vertex", "res/shaders/color.vert",
    "fragment", "res/shaders/color.frag"
)

-- Main rendering loop
local projection = glm.perspective(math.rad(45.0), window_width / window_height, 0.1, 10000.0)

local t0 = glfw.now()
local angle, speed = 0, math.pi / 3


gl.disable("cull face")
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
    local view = glm.translate(0.0, 0.0, -100.0)
    local model =
        glm.translate(0, -20, 0)
        * glm.rotate(angle, 0, 1, 0)
        * glm.rotate(math.rad(-90), 1, 0, 0)

    gl.use_program(prog)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "model"), true, model)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "view"), true, view)
    gl.uniform_matrix4f(gl.get_uniform_location(prog, "projection"), true, projection)

    -- Draw the model with random colors
    gl.bind_vertex_array(vao)
    gl.draw_elements("triangles", #expanded_indices, "uint", 0)

    glfw.swap_buffers(window)
end

-- Cleanup
gl.delete_vertex_arrays(vao)
gl.delete_buffers(vbo_position, vbo_color, ebo)
gl.clean_program(prog)
glfw.terminate()
