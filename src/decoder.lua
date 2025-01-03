local os = require('os')
local pp = require("pprint.pprint")
local xio = require('utils.io')
local bits = require('utils.bits')
local data = require('app.data')

local read = bits.reader
local IN = ".keep/dog.mdl"
local palette = data.QUAKE_PALETTE

local function decode_plt_image(indices)
  local image = {}

  for i = 1, #indices, 1 do
    local index = string.byte(indices, i) + 1
    table.insert(image, string.pack("B", palette[index].Red))
    table.insert(image, string.pack("B", palette[index].Green))
    table.insert(image, string.pack("B", palette[index].Blue))
  end

  return table.concat(image)
end

local f <close> = xio.open(IN, "rb");
if f == nil then
  os.exit(1)
end

local header = {
  Code = read.string(f, 4),
  Version = read.integer(f),
  Scale = { read.float(f), read.float(f), read.float(f) },
  Origin = { read.float(f), read.float(f), read.float(f) },
  BoundingRadius = read.float(f),
  EyePosition = { read.float(f), read.float(f), read.float(f) },
  SkinsCount = read.integer(f),
  SkinWidth = read.integer(f),
  SkinHeight = read.integer(f),
  VerticesCount = read.integer(f),
  TrianglesCount = read.integer(f),
  FramesCount = read.integer(f),
  SyncType = read.integer(f),
  Flags = read.integer(f),
  Size = read.float(f),
}

local skin_size = header.SkinWidth * header.SkinHeight
local skins = { Single = {}, Group = {} }
for idx = 1, header.SkinsCount, 1 do
  local kind = read.integer(f)
  if kind == 0 then
    local skin = read.bytes(f, skin_size)
    local image = decode_plt_image(skin)
    table.insert(skins.Single, image)
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
    table.insert(skins.Group, group)
  end
end

local uvs = {} -- TODO: needs improvements
for _ = 1, header.VerticesCount, 1 do
  local uv = {
    OnSeam = read.integer(f) ~= 0,
    U = read.integer(f),
    V = read.integer(f),
  }
  table.insert(uvs, uv)
end

local triangles = {}
for _ = 1, header.TrianglesCount, 1 do
  local triangle = {
    FrontFace = read.integer(f) ~= 0,
    Vec = {
      read.integer(f),
      read.integer(f),
      read.integer(f),
    }
  }
  table.insert(triangles, triangle)
end

local frames = { Simple = {}, Group = {} }
local frame_triangles = { Positions = {}, UVs = {} }
for _ = 1, header.FramesCount, 1 do
  local kind = read.integer(f)

  if kind == 0 then -- single frame
    local frame = {
      Min = { read.byte(f), read.byte(f), read.byte(f), read.byte(f) },
      Max = { read.byte(f), read.byte(f), read.byte(f), read.byte(f) },
      Name = read.cstring(f, 16)
    }

    local positions = {}
    for _ = 1, #uvs do
      local position = {
        (read.byte(f) * header.Scale[1]) + header.Origin[1],
        (read.byte(f) * header.Scale[2]) + header.Origin[2],
        (read.byte(f) * header.Scale[3]) + header.Origin[3],
        read.byte(f),
      }
      table.insert(positions, position)
    end

    frame.Positions = positions

    for _, triangle in ipairs(triangles) do
      for i = 1, 3 do
        local vertidx = triangle.Vec[i] + 1
        local u = uvs[vertidx].U
        local v = uvs[vertidx].V
        local onseam = uvs[vertidx].OnSeam

        if onseam and not triangle.FrontFace then
          u = u + header.SkinWidth * 0.5
        end

        u = (u + 0.5) / header.SkinWidth
        v = (v + 0.5) / header.SkinHeight

        table.insert(frame_triangles.Positions, positions[vertidx])
        table.insert(frame_triangles.UVs, { u, v })
      end
    end

    table.insert(frames.Simple, frame)
  else
    -- TODO
    pp("===========TODO===========")
  end
end

return {
  header = header,
  skins = skins,
  uvs = uvs,
  triangles = triangles,
  frames = frames,
  frame_triangles = frame_triangles,
}
