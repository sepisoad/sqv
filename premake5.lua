---@diagnostic disable: undefined-global

workspace "ProjectWorkspace"
configurations { "Debug", "Release" }
location "."

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"
optimize "Off"
buildoptions { "-fsanitize=address" }
linkoptions { "-fsanitize=address" }

filter "configurations:Release"
defines { "NDEBUG" }
optimize "Speed"

-- 🔹 Sokol Library
project "mk_sokol"
kind "StaticLib"
language "C"
targetdir ".ignore/build/"
objdir ".ignore/build/obj"
targetname "sokol"
files { "deps/sokol.c" }

filter "system:macosx"
defines { "SOKOL_GLCORE" }
links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
buildoptions { "-x objective-c" }

filter "system:linux"
defines { "SOKOL_GLCORE" }
links { "X11", "Xi", "Xcursor", "GL", "m" }

-- 🔹 Handmade Math (HMM) Library
project "mk_hmm"
kind "StaticLib"
language "C"
targetdir ".ignore/build/"
objdir ".ignore/build/obj"
targetname "hmm"
files { "deps/hmm.c" }

-- 🔹 STB Library
project "mk_stb"
kind "StaticLib"
language "C"
targetdir ".ignore/build/"
objdir ".ignore/build/obj"
targetname "stb"
buildoptions { "-Wno-deprecated-declarations" }   -- Suppress deprecation warnings for macOS
files { "deps/stb.c" }

-- 🔹 Main Application
project "mk_sqv"
kind "ConsoleApp"
language "C"
targetdir ".ignore/build/"
objdir ".ignore/build/obj"
targetname "sqv"
files { "src/*.c" }
includedirs { "src", "deps" }
links { "mk_stb:static", "mk_sokol:static", "mk_hmm:static" }
buildoptions { "-std=c2x" }
defines { "_POSIX_C_SOURCE=199309L" }   -- Needed for some C23 features

filter "system:macosx"
defines { "SOKOL_GLCORE" }
links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }

filter "system:linux"
defines { "SOKOL_GLCORE" }
links { "X11", "Xi", "Xcursor", "GL", "m", "mk_stb:static", "mk_sokol:static", "mk_hmm:static" }

-- 🔹 GLSL Shader Compilation Action
newaction {
  trigger = "glsl",
  description = "Compile shaders into C headers",
  execute = function()
    os.execute("sokol-shdc -i res/shaders/default.glsl -l glsl410 -f sokol -o src/glsl_default.h")
  end
}

-- 🔹 Run Action
newaction {
  trigger = "run",
  description = "Execute the program",
  execute = function()
    os.execute(".ignore/build/sqv")
  end
}
