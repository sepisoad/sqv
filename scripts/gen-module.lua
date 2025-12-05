--
-- DESCRIPTIONS:
--
-- use this script to generate a c module,
-- well we do not really have anything known as a c module!
-- in this context a header only c file that is sort of self contained
--
-- USAGE:
--
-- run `premake gen-module
-- this will produce a module scaffold for you
-- the output is a file named module_XXX.h
-- renamed the 'XXX' part to some name
-- then you add the actual source code
--

local module_name = 'XXX'
local function_prefix = string.lower(module_name)..'_'
local main_type_name = string.sub(module_name, 1, 1) .. string.lower(string.sub(module_name, 2))
local error_type = main_type_name..'Error'
local filename = "src/module_"..module_name..".h"

local f, err = io.open(filename, "w+")
if not f then
  error("could not open file: " .. err)
end

f:write('#ifndef MODULE_'..module_name..'_HEADER\n')
f:write('#define MODULE_'..module_name..'_HEADER\n\n')

f:write('/* ===================================================== */\n')
f:write('/*                     DEPENDENCIES                      */\n')
f:write('/* ===================================================== */\n\n')

f:write('#include "deps/sepi/base.h"\n\n')

f:write('/* ===================================================== */\n')
f:write('/*                       CONSTANTS                       */\n')
f:write('/* ===================================================== */\n\n')

f:write('// #define '..module_name..'_FOO "FOO"\n\n')

f:write('/* ===================================================== */\n')
f:write('/*                         TYPES                         */\n')
f:write('/* ===================================================== */\n\n')

f:write('// typedef enum {\n')
f:write('//   '..module_name..'_ERR_UNKNOWN,\n')
f:write('//   '..module_name..'_ERR__COUNT,\n')
f:write('// } '..error_type..';\n\n')

f:write('// typedef struct {\n')
f:write('//   Arena*     arena;\n')
f:write('// } '..main_type_name..';\n\n')

f:write('/* ===================================================== */\n')
f:write('/*                          API                          */\n')
f:write('/* ===================================================== */\n\n')

f:write('// '..error_type..' '..function_prefix..'foo();\n\n')

f:write('/* ===================================================== */\n')
f:write('/*                    IMPLEMENTATION                     */\n')
f:write('/* ===================================================== */\n\n')

f:write('#ifdef MODULE_'..module_name..'_IMPLEMENTATION\n\n')

f:write('// '..error_type..'\n// '..function_prefix..'foo(){\n// }\n\n')

f:write('/* ===================================================== */\n')
f:write('/*                          END                          */\n')
f:write('/* ===================================================== */\n\n')

f:write('#endif  // MODULE_'..module_name..'_IMPLEMENTATION\n')
f:write('#endif  // MODULE_'..module_name..'_HEADER\n')



f:close()
