---@diagnostic disable: undefined-global

workspace "ProjectWorkspace"
configurations { "Debug", "Release" }
location "."
toolset "clang"

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"
optimize "Off"
buildoptions { "-fsanitize=address" }
linkoptions { "-fsanitize=address" }

filter "configurations:Release"
defines { "NDEBUG" }
optimize "Speed"

-- Rxi Log Library
project "mk_log"
kind "StaticLib"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "log"
files { "deps/log.c" }

-- Rxi INI Library
project "mk_ini"
kind "StaticLib"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "ini"
files { "deps/ini.c" }

-- Sokol Library
project "mk_sokol"
kind "StaticLib"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "sokol"
files { "deps/sokol.c" }

filter "system:macosx"
defines { "SOKOL_GLCORE" }
links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
buildoptions { "-x objective-c" }

filter "system:linux"
defines { "SOKOL_GLCORE" }
links { "X11", "Xi", "Xcursor", "GL", "m" }

-- Handmade Math (HMM) Library
project "mk_hmm"
kind "StaticLib"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "hmm"
files { "deps/hmm.c" }

-- STB Library
project "mk_stb"
kind "StaticLib"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "stb"
buildoptions { "-Wno-deprecated-declarations" }
files { "deps/stb.c" }

-- STB Library
project "mk_sepi"
kind "StaticLib"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "sepi"
buildoptions { "-Wno-deprecated-declarations" }
buildoptions { "-std=c23" }
files { "deps/sepi.c" }

-- Main Application
project "mk_sqv"
kind "ConsoleApp"
language "C"
location "BUILD"
targetdir "BUILD/"
objdir "BUILD/obj"
targetname "sqv"
includedirs { "src", "deps" }
links {
  "mk_log:static",
  "mk_ini:static",
  "mk_stb:static",
  "mk_hmm:static",
  "mk_sepi:static",
  "mk_sokol:static",
}
files {
  "src/app.c",
  "src/render_common.c",
  "src/render_init.c",
  "src/render_lmp.c",
  "src/render_md1.c",
  "src/render_pak.c",
  "src/render_ui.c",
  "src/render_wad.c",
}
buildoptions { "-std=c23" }
defines { "SOKOL_GLCORE" }
defines { "_POSIX_C_SOURCE=199309L" }   -- Needed for some C23 features

filter "system:macosx"
links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }

filter "system:linux"
links { "X11", "Xi", "Xcursor", "GL", "m" }

-- GLSL Shader Compilation Action
newaction {
  trigger = "glsl",
  description = "Compile shaders into C headers",
  execute = function()
    os.execute("sokol-shdc -i res/shaders/default.glsl -l glsl410 -f sokol -o res/shaders/default.glsl.h")
  end
}

-- Clean SQV Action
newaction {
  trigger = "c",
  description = "Execute the program with optional arguments",
  execute = function()
    os.execute("make --no-print-directory -C BUILD -f mk_sqv.make clean")
  end
}

-- Run Action 1
newaction {
  trigger = "p",
  description = "execute",
  execute = function()
    os.execute("BUILD/sqv")
  end
}


-- Run Action 1
newaction {
  trigger = "r",
  description = "quick execute",
  execute = function()
    os.execute("BUILD/sqv -i=KEEP/dog.mdl")
  end
}

-- Run Action 2
newaction {
  trigger = "rr",
  description = "execute with args",
  execute = function()
    -- Capture additional command-line arguments
    local args = _ARGS
    local args_str = table.concat(args, " ")

    -- Execute the program with arguments
    os.execute("BUILD/sqv -i=" .. args_str)
  end
}
