-- src/strease.lua
local strease = {}

-- Trim spaces off the edges, because who likes extra whitespace?
function strease.trim(s)
    return s:match("^%s*(.-)%s*$") -- Just cutting off those annoying spaces from both ends.
end

-- Split the string by a separator, default is space if you don’t provide one
function strease.split(s, sep)
    sep = sep or " "
    local parts = {}
    for part in s:gmatch("([^" .. sep .. "]+)") do
        table.insert(parts, part) -- Slice up the string with the separator.
    end
    return parts
end

-- Capitalize each word's first letter, just to make things look neat
function strease.capitalize(s)
    return s:gsub("(%a)([%w]*)", function(a, b) return a:upper() .. b:lower() end) -- Make it look all formal.
end

-- Replace old substring with new one, simple as that
function strease.replace(s, old, new)
    return s:gsub(old, new) -- Swap out the old for the new stuff.
end

-- Join elements of a table into a single string, using whatever separator you want
function strease.join(tbl, sep)
    sep = sep or " "
    return table.concat(tbl, sep) -- Stick everything together with the separator.
end

-- Convert all characters to lowercase, if you’re feeling mellow
function strease.lower(s)
    return s:lower() -- Turn it all to lowercase, if that's your thing.
end

-- Convert all characters to uppercase, if you're in shouting mode
function strease.upper(s)
    return s:upper() -- Shout everything out in uppercase!
end

-- Check if the string starts with a given prefix
function strease.startswith(s, prefix)
    return s:sub(1, #prefix) == prefix -- Does it start with what we expect?
end

-- Check if the string ends with a given suffix
function strease.endswith(s, suffix)
    return s:sub(-#suffix) == suffix -- Does it end as we like?
end

-- Count how many times a substring appears in the string
function strease.count(s, sub)
    local count = 0
    for _ in s:gmatch(sub) do count = count + 1 end -- Count how many times the substring shows up.
    return count
end

return strease