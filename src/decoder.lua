local os = require('os')
local stb = require('stb')
local pp = require("pprint.pprint")
local xio = require('utils.io')
local bits = require('utils.bits')
local paths = require('utils.paths')
local data = require('app.data')
local glm = require("moonglmath")

local read = bits.reader
local IN = ".keep/armor.mdl"
local palette = data.QUAKE_PALETTE


local function save_png_file(data, palette, width, height, path)
   local pixels = {}
   for i = 1, #data, 1 do
      pixels[i] = string.byte(data, i)
   end

   local ok, err = pcall(stb.encode_paletted_png, path, palette, pixels, width, height)
   if not ok then
      os.exit(1)
   end
end

local f <close> = xio.open(IN, "rb");
if f == nil then
   os.exit(1)
end

local header = {
   Code = read.string(f, 4),
   Version = read.integer(f),
   Scale = glm.vec3(read.float(f), read.float(f), read.float(f)),
   -- {
   --    X = read.float(f),
   --    Y = read.float(f),
   --    Z = read.float(f),
   -- },
   Origin = glm.vec3(read.float(f), read.float(f), read.float(f)),
   -- {
   --    X = read.float(f),
   --    Y = read.float(f),
   --    Z = read.float(f),
   -- },
   BoundingRadius = read.float(f),
   EyePosition = glm.vec3(read.float(f), read.float(f), read.float(f)),
   -- {
   --    X = read.float(f),
   --    Y = read.float(f),
   --    Z = read.float(f),
   -- },
   SkinsCount = read.integer(f),
   SkinWidth = read.integer(f),
   SkinHeight = read.integer(f),
   VerticesCount = read.integer(f),
   TrianglesCount = read.integer(f), -- WTF is this?
   FramesCount = read.integer(f),
   SyncType = read.integer(f),
   Flags = read.integer(f),
   Size = read.float(f),
}

local skin_size = header.SkinWidth * header.SkinHeight
local skins = { single = {}, group = {} }
for idx = 1, header.SkinsCount, 1 do
   local group = read.integer(f)
   if group == 0 then
      local skin = read.bytes(f, skin_size)
      table.insert(skins.single, skin)
      local fp = ".ignore/out/" .. idx .. ".png"
      paths.create_dir_for_file_path(".ignore/out/" .. idx .. ".png")
      save_png_file(skin, palette, header.SkinWidth, header.SkinHeight, fp)
   else
      local count = read.integer(f)
      local group = {}
      for _ = 1, count, 1 do
         local skin = {
            Time = read.float(f),
            Data = read.bytes(f, skin_size)
         }
         table.insert(group, skin)
      end
      table.insert(skins.group, group)
   end
end

local tex_coords = {} -- TODO: needs improvements
for _ = 1, header.VerticesCount, 1 do
   local tex_coord = {
      OnSeam = read.integer(f) ~= 0,
      S = read.integer(f),
      T = read.integer(f),
   }
   table.insert(tex_coords, tex_coord)
end

local triangles = {}
for _ = 1, header.TrianglesCount, 1 do
   local triangle = {
      FrontFace = read.integer(f) ~= 0,
      Vec = glm.vec3(
         read.integer(f),
         read.integer(f),
         read.integer(f)),
   }
   table.insert(triangles, triangle)
end

local frames = {}
for _ = 1, header.FramesCount, 1 do
   local frame = {
      Type = read.integer(f),
      Min = glm.vec4(
         read.byte(f),
         read.byte(f),
         read.byte(f),
         read.byte(f)),
      Max = glm.vec4(
         read.byte(f),
         read.byte(f),
         read.byte(f),
         read.byte(f)),
      Name = read.cstring(f, 16)
   }

   local vertices = {}
   for _ = 1, header.VerticesCount, 1 do
      local vertex = glm.vec4(
         read.byte(f),
         read.byte(f),
         read.byte(f),
         read.byte(f))
      table.insert(vertices, vertex)
   end

   frame.Vertices = vertices

   table.insert(frames, frame)
end

return {
   header = header,
   skins = skins,
   tex_coords = tex_coords,
   triangles = triangles,
   frames = frames,
}
