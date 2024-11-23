local fs = require('lfs')
local mfs = require('minifs.minifs')
local log = require('log.log')
local str = require('str.str')

local function create_dir_if_doesnt_exist (p)
  if not mfs.exists(p) then
    if not mfs.mkrdir(p) then
      log.err(string.format("failed to create extraction directory at '%s'", p))
      os.exit(1)
    end
  end
end

local function create_dir_for_file_path (p)
  local sep = mfs.separator()
  local p2 = p:match(".*[" .. sep .. "]")
  if not mfs.exists(p2) then
    return mfs.mkrdir(p2)
  end
end

local function join_item_path (dir, item)
  local full = mfs.join(dir, item)
  local base = mfs.dirname(full)
  return full, base
end

local function get_file_disk_size (p)
  if not mfs.exists(p) then
    log.err(string.format("failed to open '%s'", p))
    os.exit(1)
  end

  local attr = fs.attributes(p)
  if not attr.size then
    log.warn(string.format("cannot get file disk size for '%s'", p))
    return 0
  end
  return attr.size
end

local function list_files_in_dir (p)
  if not mfs.exists(p) then
    return {}
  end

  return mfs.rfiles(p, true)
end

local function trim_path (p, p2)
  local p3, _ = string.gsub(p, p2, "")
  return p3
end

local function read_file_data (p)
  return mfs.read(p)
end

local function file_name(path)
  local x = mfs.filename(path)
  local parts = str.split(x,".")
  if #parts <= 1 or #parts > 2 then return path end
  return parts[1], parts[2]
end

return {
  create_dir_if_doesnt_exist = create_dir_if_doesnt_exist,
  create_dir_for_file_path = create_dir_for_file_path,
  join_item_path = join_item_path,
  get_file_disk_size = get_file_disk_size,
  list_files_in_dir = list_files_in_dir,
  read_file_data = read_file_data,
  trim_path = trim_path,
  file_name = file_name
}
