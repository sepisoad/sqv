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

local function expand_position(positions)
  local _position = {}
  for _, position in ipairs(positions) do
    table.insert(_position, position[1])
    table.insert(_position, position[2])
    table.insert(_position, position[3])
  end
  return _position
end

local function expand_triangles(triangles)
  local _triangles = {}
  for _, triangle in ipairs(triangles) do
    table.insert(_triangles, triangle.Vec[1])
    table.insert(_triangles, triangle.Vec[2])
    table.insert(_triangles, triangle.Vec[3])
  end
  return _triangles
end

local function expand_positions_and_triangles(positions, triangles)
  local _positions = {}
  local _tirangles = {}
  local current_index = 0

  for i = 1, #triangles, 3 do
    for j = 0, 2 do
      local idx = triangles[i + j] + 1
      table.insert(_positions, positions[(idx - 1) * 3 + 1]) -- X
      table.insert(_positions, positions[(idx - 1) * 3 + 2]) -- Y
      table.insert(_positions, positions[(idx - 1) * 3 + 3]) -- Z
      table.insert(_tirangles, current_index)
      current_index = current_index + 1
    end
  end

  return _positions, _tirangles
end

local function create_texture_coordinates(uvs, triangles)
  local out_uv = {}
  for _, triangle in ipairs(triangles) do
    for j = 1, 3 do
      local idx = triangle.Vec[j] + 1

      local s = uvs[idx].U
      local t = uvs[idx].V
      local onseam = uvs[idx].OnSeam

      if not triangle.FrontFace and onseam then
        s = s + mdl.header.SkinWidth * 0.5;
      end

      s = (s + 0.5) / mdl.header.SkinWidth;
      t = (t + 0.5) / mdl.header.SkinHeight;

      table.insert(out_uv, s)
      table.insert(out_uv, t)
    end
  end
  return out_uv
end

-- Generate data
local positions = expand_position(mdl.frames.Simple[1].Positions)
local triangles = expand_triangles(mdl.triangles)
local positions_ex, indices_ex = expand_positions_and_triangles(positions, triangles)
local uvs = create_texture_coordinates(mdl.uvs, mdl.triangles)

-- Create VAO and buffers
local vao = gl.gen_vertex_arrays()
local vbo_positions, vbo_texcoords, ebo = gl.gen_buffers(3)

-- Bind the vertex array object
gl.bind_vertex_array(vao)

-- Bind and buffer positions
gl.bind_buffer('array', vbo_positions)
gl.buffer_data('array', gl.pack('float', positions_ex), 'static draw')
gl.vertex_attrib_pointer(0, 3, 'float', false, 3 * gl.sizeof('float'), 0)
gl.enable_vertex_attrib_array(0)

-- Bind and buffer texture coordinates
gl.bind_buffer('array', vbo_texcoords)
gl.buffer_data('array', gl.pack('float', uvs), 'static draw')
gl.vertex_attrib_pointer(1, 2, 'float', false, 2 * gl.sizeof('float'), 0)
gl.enable_vertex_attrib_array(1)

-- Bind and buffer element indices
gl.bind_buffer('element array', ebo)
gl.buffer_data('element array', gl.pack('uint', indices_ex), 'static draw')

gl.unbind_buffer('array')
gl.unbind_vertex_array()

-- load and create a texture --------------------------------------------------
local data = mdl.skins.Single[1]
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
  gl.draw_elements("triangles", #indices_ex, "uint", 0)

  glfw.swap_buffers(window)
end

gl.delete_vertex_arrays(vao)
gl.delete_buffers(vbo_positions, vbo_texcoords, ebo)
gl.delete_program(prog)
