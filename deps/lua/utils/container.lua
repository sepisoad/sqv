local function get_container_files_mapping (container)
  local mapping = {}
  for _, item in pairs(container) do
    local file = item:match("([^/]+)$")
    if file then
      local ext = item:match("([^%.]+)$")
      if ext then
        mapping[ext] = (mapping[ext] or 0) + 1
      end
    end
  end

  local sorted = {}
  local idx = 1
  for key, val in pairs(mapping) do
    sorted[idx] = {
      "." .. string.upper(key) .. "",
      val
    }
    idx = idx + 1
  end

  table.sort(sorted, function(a, b) return a[2] > b[2] end)
  return sorted
end

return {
  get_container_files_mapping = get_container_files_mapping
}
