local gl = require("moongl")
local glfw = require("moonglfw")
local glm = require("moonglmath")
local img = require("moonimage")
local mdl = require("src.decoder")
local pp = require("pprint.pprint")

local window_width = 300
local window_height = 250

-- Initialize GLFW
glfw.version_hint(3, 3, "core")
local window = glfw.create_window(window_width, window_height, "Correct Winding Order")
glfw.make_context_current(window)
gl.init()


-- Generate position data
local function create_position(vertices)
  local out_vertices = {}

  for _, vertex in ipairs(vertices) do
    local sclae = mdl.header.Scale
    local origin = mdl.header.Origin
    table.insert(out_vertices, (vertex[1] * sclae[1]) + origin[1])
    table.insert(out_vertices, (vertex[2] * sclae[2]) + origin[2])
    table.insert(out_vertices, (vertex[3] * sclae[3]) + origin[3])
  end
  return out_vertices
end

-- Generate index data with correct winding order based on the triangle side
local function create_indices_with_side(triangles)
  local out_triangles = {}
  for _, triangle in ipairs(triangles) do
    table.insert(out_triangles, triangle.Vec[1])
    table.insert(out_triangles, triangle.Vec[2])
    table.insert(out_triangles, triangle.Vec[3])
  end
  return out_triangles
end



-- Expand vertices and indices to ensure unique vertices per triangle
local function expand_vertices_and_indices(vertices, indices)
  local expanded_vertices = {}
  local expanded_indices = {}
  local current_index = 0

  for i = 1, #indices, 3 do
    for j = 0, 2 do
      local vertex_index = indices[i + j] + 1
      table.insert(expanded_vertices, vertices[(vertex_index - 1) * 3 + 1]) -- X
      table.insert(expanded_vertices, vertices[(vertex_index - 1) * 3 + 2]) -- Y
      table.insert(expanded_vertices, vertices[(vertex_index - 1) * 3 + 3]) -- Z
      table.insert(expanded_indices, current_index)
      current_index = current_index + 1
    end
  end

  return expanded_vertices, expanded_indices
end


local function create_texture_coordinates(tex_coords, triangles)
  local out_tex_coords = {}
  for _, triangle in ipairs(triangles) do
    for j = 1, 3 do
      local idx = triangle.Vec[j] + 1

      local s = tex_coords[idx].S
      local t = tex_coords[idx].T
      local onseam = tex_coords[idx].OnSeam

      if not triangle.FrontFace and onseam then
        s = s + mdl.header.SkinWidth * 0.5;
      end

      s = (s + 0.5) / mdl.header.SkinWidth;
      t = (t + 0.5) / mdl.header.SkinHeight;

      table.insert(out_tex_coords, s)
      table.insert(out_tex_coords, t)
    end
  end
  return out_tex_coords
end

-- Generate data
local positions = create_position(mdl.frames[1].Vertices)
local indices = create_indices_with_side(mdl.triangles)
local expanded_positions, expanded_indices = expand_vertices_and_indices(positions, indices)
local text_coords = create_texture_coordinates(mdl.tex_coords, mdl.triangles)

-- Create VAO and buffers
local vao = gl.gen_vertex_arrays()
local vbo_positions, vbo_texcoords, ebo = gl.gen_buffers(3)

-- Bind the vertex array object
gl.bind_vertex_array(vao)

-- Bind and buffer positions
gl.bind_buffer('array', vbo_positions)
gl.buffer_data('array', gl.pack('float', expanded_positions), 'static draw')
gl.vertex_attrib_pointer(0, 3, 'float', false, 3 * gl.sizeof('float'), 0)
gl.enable_vertex_attrib_array(0)

-- Bind and buffer texture coordinates
gl.bind_buffer('array', vbo_texcoords)
gl.buffer_data('array', gl.pack('float', text_coords), 'static draw')
gl.vertex_attrib_pointer(1, 2, 'float', false, 2 * gl.sizeof('float'), 0)
gl.enable_vertex_attrib_array(1)

-- Bind and buffer element indices
gl.bind_buffer('element array', ebo)
gl.buffer_data('element array', gl.pack('uint', expanded_indices), 'static draw')

gl.unbind_buffer('array')
gl.unbind_vertex_array()

-- load and create a texture --------------------------------------------------
local data = mdl.skins.single[1]
local texture = gl.gen_textures()
gl.bind_texture('2d', texture)
gl.texture_parameter('2d', 'wrap s', 'repeat')
gl.texture_parameter('2d', 'wrap t', 'repeat')
gl.texture_parameter('2d', 'min filter', 'linear mipmap linear')
gl.texture_parameter('2d', 'mag filter', 'linear')
gl.texture_image('2d', 0, 'rgb', 'rgb', 'ubyte', data, mdl.header.SkinWidth, mdl.header.SkinHeight)
gl.generate_mipmap('2d')

-- Compile and use shaders
local prog = gl.make_program(
  "vertex", "res/shaders/color.vert",
  "fragment", "res/shaders/color.frag"
)

-- Main rendering loop
local projection = glm.perspective(math.rad(45.0), window_width / window_height, 0.1, 10000.0)

local t0 = glfw.now()
local angle, speed = 0, math.pi / 3

-- gl.enable("cull face") -- SEPI: NEED THIS
-- gl.polygon_mode("front and back", "line") -- SEPI: VERY NICE
-- gl.cull_face("front")  -- SEPI: DUNNO
gl.front_face("ccw")   -- SEPI: DUNNO

while not glfw.window_should_close(window) do
  glfw.poll_events()
  if glfw.get_key(window, 'escape') == 'press' then
    glfw.set_window_should_close(window, true)
  end

  gl.clear_color(0.0, 0.0, 0.0, 1.0)
  gl.clear("color")
  gl.bind_texture('2d', texture)

  local t = glfw.now()
  local dt = t - t0
  t0 = t
  angle = angle + speed * dt
  if angle >= 2 * math.pi then angle = angle - 2 * math.pi end

  -- Set transformations
  local view = glm.translate(0.0, 0.0, -100.0)
  local model =
      glm.translate(0, 0, 0)
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

gl.delete_vertex_arrays(vao)
gl.delete_buffers(vbo_positions, vbo_texcoords, ebo)
gl.delete_program(prog)
