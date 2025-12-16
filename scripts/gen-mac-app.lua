local cfg = {
  bin      = ".build/app_pak",   -- input binary
  app_name = "sqv_pak",              -- display name + default exe name
  bundle_id= "sqv.sepi.me",
  out_app  = "sqv_pak.app",          -- output bundle folder

  version  = "1",
  short    = "1.0",
  min_macos= "10.15",

  icon_icns = nil,              -- e.g. "SQV.icns" (or nil to skip)

  codesign = true,              -- ad-hoc sign for local use
  verify   = true,
}
local app      = cfg.out_app
local contents = app .. "/Contents"
local macos    = contents .. "/MacOS"
local res      = contents .. "/Resources"
local dst_bin  = macos .. "/" .. cfg.app_name

local function q(s) return "'" .. tostring(s):gsub("'", "'\\''") .. "'" end
local function run(cmd)
  local ok, how, code = os.execute(cmd)
  if ok ~= true then
    error(("command failed (%s %s): %s"):format(tostring(how), tostring(code), cmd))
  end
end

local function write(path, data)
  local f = assert(io.open(path, "wb"))
  f:write(data)
  f:close()
end

local function plist()
  local icon_block = ""
  if cfg.icon_icns then
    icon_block = ("\n  <key>CFBundleIconFile</key><string>%s</string>"):format(cfg.app_name)
  end

  return ([[<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>%s</string>
  <key>CFBundleDisplayName</key><string>%s</string>
  <key>CFBundleIdentifier</key><string>%s</string>
  <key>CFBundleVersion</key><string>%s</string>
  <key>CFBundleShortVersionString</key><string>%s</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleExecutable</key><string>%s</string>
  <key>LSMinimumSystemVersion</key><string>%s</string>%s
</dict>
</plist>
]]):format(
    cfg.app_name, cfg.app_name, cfg.bundle_id,
    cfg.version, cfg.short, cfg.app_name, cfg.min_macos,
    icon_block
  )
end


-- 1) dirs
run("mkdir -p " .. q(macos) .. " " .. q(res))

-- 2) binary
run("cp -f " .. q(cfg.bin) .. " " .. q(dst_bin))
run("chmod +x " .. q(dst_bin))

-- 3) plist
write(contents .. "/Info.plist", plist())

-- 4) icon (optional)
if cfg.icon_icns then
  run("cp -f " .. q(cfg.icon_icns) .. " " .. q(res .. "/" .. cfg.app_name .. ".icns"))
end

-- 5) codesign (optional)
if cfg.codesign then
  run("codesign --force --deep --sign - " .. q(app))
  if cfg.verify then
    run("codesign --verify --deep --strict --verbose=2 " .. q(app))
  end
end

print("OK: " .. app)
