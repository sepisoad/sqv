---@diagnostic disable: undefined-global

workspace "ProjectWorkspace"
  -- toolset "gcc-14.2.0"
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

project "mk_sokol"
    kind "StaticLib"
    language "C"
    targetdir ".ignore/build/"
    objdir ".ignore/build/obj"
    targetname "sokol"
  
    files {
      "deps/sokol.c",
    }
  
    links { "X11", "Xi", "Xcursor", "GL", "m" }

project "mk_hmm"
    kind "StaticLib"
    language "C"
    targetdir ".ignore/build/"
    objdir ".ignore/build/obj"
    targetname "hmm"
  
    files {
      "deps/hmm.c",
    }


project "mk_stb"
    kind "StaticLib"
    language "C"
    targetdir ".ignore/build/"
    objdir ".ignore/build/obj"
    targetname "stb"
  
    files {
      "deps/stb.c",
    }

project "mk_sqv"
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
    "deps",
    "res/shaders",
  }

  links { "X11", "Xi", "Xcursor", "GL", "m", "mk_stb:static", "mk_sokol:static", "mk_hmm:static" }

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
