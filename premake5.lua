---@diagnostic disable: undefined-global

workspace "ProjectWorkspace"
  toolset "gcc-14.2.0"
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

project "Host"
  kind "ConsoleApp"
  language "C"
  targetdir ".ignore/build/"
  objdir ".ignore/build/obj"
  targetname "sqv"

  files {
    "src/*.c",
  }

  includedirs {
    "src",
    "res/shaders",
    "/usr/local/include",
  }

  links { "X11", "Xi", "Xcursor", "GL", "m" }

  buildoptions { "-std=c2x" }

  defines {
    "_POSIX_C_SOURCE=199309L" -- only needed for c23
  }

  newaction {
    trigger = "glsl",
    description = "compile shaders into c headers",
    execute = function()
      os.execute("sokol-shdc -i res/shaders/default.glsl -l glsl410 -f sokol -o src/glsl_default.h")
    end
  }
  newaction {
    trigger = "run",
    description = "execute the program",
    execute = function()
      os.execute(".ignore/build/sqv")
    end
  }
