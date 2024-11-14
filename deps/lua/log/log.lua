_G.VERBOSE = false

---@enum level
local LEVEL = { INFO = "info", DBG = "debug", ERR = "error", WARN = "warning" }


---@param msg string
---@param ext string?
---@param lvl level
local function print_log (msg, ext, lvl)
  assert(msg ~= nil and msg ~= "")
  assert(lvl ~= nil)

  msg = string.format("[%s]: %s", lvl, msg)
  if ext ~= nil and ext ~= "" then
    msg = string.format("%s\n details: %s", msg, ext)
  end
  print(msg)
end

---@param msg string
---@param ext string?
local function print_info (msg, ext)
  print_log(msg, ext, LEVEL.INFO)
end

---@param msg string
---@param ext string?
local function print_dbg (msg, ext)
  if _G.VERBOSE then
    print_log(msg, ext, LEVEL.DBG)
  end
end

---@param msg string
---@param ext string?
local function print_warn (msg, ext)
  print_log(msg, ext, LEVEL.WARN)
end

---@param msg string
---@param ext string?
local function print_err (msg, ext)
  print_log(msg, ext, LEVEL.ERR)
end

---@param msg string
---@param ext string?
local function print_fatal (msg, ext)
  print_log(msg, ext, LEVEL.ERR) -- it's ok to use ERR level!
  os.exit(1)
end

return {
  info = print_info,
  dbg = print_dbg,
  warn = print_warn,
  err = print_err,
  fatal = print_fatal,
  VERBOSE = VERBOSE
}
