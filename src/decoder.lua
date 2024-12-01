local os = require('os')
local stb = require('stb')
local pp = require("pprint.pprint")
local xio = require('utils.io')
local bits = require('utils.bits')
local paths = require('utils.paths')
local data = require('app.data')

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
      pp(err)
      os.exit(1)
   end
end

local f <close> = xio.open(IN, "rb");
if f == nil then
   os.exit(1)
end

local hdr = {
   Code = read.string(f, 4),
   Version = read.integer(f),
   Scale = {
      X = read.float(f),
      Y = read.float(f),
      Z = read.float(f),
   },
   ScaleOrigin = {
      X = read.float(f),
      Y = read.float(f),
      Z = read.float(f),
   },
   BoundingRadius = read.float(f),
   EyePosition = {
      X = read.float(f),
      Y = read.float(f),
      Z = read.float(f),
   },
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

local skin_size = hdr.SkinWidth * hdr.SkinHeight
local skins = { single = {}, group = {} }
for idx = 1, hdr.SkinsCount, 1 do
   local group = read.integer(f)
   if group == 0 then
      local skin = read.bytes(f, skin_size)
      table.insert(skins.single, skin)
      local fp = ".ignore/out/" .. idx .. ".png"
      paths.create_dir_for_file_path(".ignore/out/" .. idx .. ".png")
      save_png_file(skin, palette, hdr.SkinWidth, hdr.SkinHeight, fp)
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
for _ = 1, hdr.VerticesCount, 1 do
   local tex_coord = {
      OnSeam = read.integer(f) ~= 0,
      S = read.integer(f),
      T = read.integer(f),
   }
   table.insert(tex_coords, tex_coord)
end

local triangles = {}
for _ = 1, hdr.TrianglesCount, 1 do
   local triangle = {
      FrontFace = read.integer(f) ~= 0,
      X = read.integer(f),
      Y = read.integer(f),
      Z = read.integer(f),
   }
   table.insert(triangles, triangle)
end

local frames = {}
for _ = 1, hdr.FramesCount, 1 do
   local frame = {
      Type = read.integer(f),
      Min = {
         X = read.byte(f),
         Y = read.byte(f),
         Z = read.byte(f),
         N = read.byte(f),
      },
      Max = {
         X = read.byte(f),
         Y = read.byte(f),
         Z = read.byte(f),
         N = read.byte(f),
      },
      Name = read.cstring(f, 16)
   }

   local vertices = {}
   for _ = 1, hdr.VerticesCount, 1 do
      local vertex = {
         X = read.byte(f),
         Y = read.byte(f),
         Z = read.byte(f),
         N = read.byte(f),
      }
      table.insert(vertices, vertex)
   end

   frame.Vertices = vertices

   table.insert(frames, frame)
end

pp(frames)
