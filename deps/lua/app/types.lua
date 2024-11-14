--- ===============================================
--- WadItemType
--- ===============================================

---@enum WadItemType
WadItemType = {
  None    = 0,  -- not used anywher in quake engine!
  Label   = 1,  -- not used anywher in quake engine!
  Lumpy   = 64, -- not used anywher in quake engine!
  Palette = 64, -- not used anywher in quake engine!
  QTex    = 65, -- not used anywher in quake engine!
  QPic    = 66,
  Sound   = 67, -- not used anywher in quake engine!
  MipTex  = 68, -- not used anywher in quake engine!
}

--- ===============================================
--- WadHeader
--- ===============================================

---@class WadHeader
---@field Code string        (4 bytes)
---@field ItemsCount integer (4 bytes)
---@field Offset integer     (4 bytes)
WadHeader = {
  _Magic = "WAD2"
}

--- ===============================================
--- WadItem(s)Header
--- ===============================================

---@class WadItemHeader
---@field Position integer        (4 bytes)
---@field Size integer            (4 bytes)
---@field CompressedSize integer  (4 bytes)
---@field Type integer            (1 bytes)
---@field CompressionType integer (1 bytes)
---@field Paddings string         (2 bytes)
---@field Name string             (16 bytes)
WadItemHeader = {
  _Type = 1,
  _CompressionType = 1,
  _Paddings = 2,
  _Name = 16,
}

---@alias WadItemsHeader WadItemHeader[]

--- ===============================================
--- TexHeader
--- ===============================================

---@class TexHeader
---@field Name string     (16 bytes)
---@field Width integer   (4 bytes)
---@field Height integer  (4 bytes)
---@field Data any        (variable size)
TexHeader = {
  _Name = 16,
}

--- ===============================================
--- TexHeader
--- ===============================================

---@class PakHeader
---@field Code string (4 bytes)
---@field Offset integer (4 bytes)
---@field Length integer (4 bytes)
PakHeader = {
  _Magic = "PACK",
  _Size = 4 + 4 + 4
}

--- ===============================================
--- PakItemHeader
--- ===============================================

---@class PakItemHeader
---@field Name string
---@field Position integer
---@field Length integer
PakItemHeader = {
  _Name = 56,
  _Size = 56 + 4 + 4
}

---@alias PakItemsHeader PakItemHeader[]

--- ===============================================
--- PakItemHeader
--- ===============================================

---@class LumpHeader
---@field Width integer (4 bytes )
---@field Height integer (4 bytes )
---@field Data any (buffer)
LumpHeader = {}

--- ===============================================
--- MipTex
--- ===============================================
---@class MipTex
---@field Name string (16 bytes)
---@field Width integer (4 bytes)
---@field Height integer (4 bytes)
---@field Offsets integer[] (16 bytes - 4 integers)
MipTex = {}

--- ===============================================
--- RGBColor
--- ===============================================

---@class RGBColor
---@field Red integer (1 byte unsigned, 0-255)
---@field Green integer (1 byte unsigned, 0-255)
---@field Blue integer (1 byte unsigned, 0-255)
RGBColor = {
  _Size = 3
}

---@alias Colors RGBColor[]

--- ===============================================
--- PaletteData
--- ===============================================

---@class PaletteData
---@field Colors Colors
PaletteData = {}

--- ===============================================
--- MdlData
--- ===============================================

---@class MdlData
---@field Type string (4 bytes)
MdlData = {}

--- ===============================================
--- Md5Mesh
--- ===============================================

---@class Md5Mesh
---@field Type string (4 bytes)
MdlData = {}