
--- open, either sucessfully opens a file, or aborts with a message
---@param path string
---@param mode string
---@param msg string|nil
---@return file*
local  function open (path, mode, msg)
  if msg == nil then
    msg = string.format("failed to open '%s'", path)
  end
  return assert(io.open(path, mode), msg)
end

return {
  open = open
}