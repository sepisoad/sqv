--
-- WORKSPACE -----------------------
--

workspace "ProjectWorkspace"
  configurations { "Debug", "Release", "Profiling" }
  location "."
  toolset "clang"
  includedirs { "src/deps" }
  buildoptions { "-std=gnu11" }
  -- linkoptions  { "-fuse-ld=mold" }

  filter "system:windows"
    gccprefix "x86_64-w64-mingw32-"
    architecture "x86_64"

  -- Debug (common)
  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
    optimize "Off"

  -- Debug + macOS
  filter { "configurations:Debug", "system:macosx" }
    -- FUCK MACOS, i have to disable ASAN for now!
    -- buildoptions { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }
    -- linkoptions  { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }
    buildoptions { "-fsanitize=undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }
    linkoptions  { "-fsanitize=undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }

  -- Debug + Linux
  filter { "configurations:Debug", "system:linux" }
    buildoptions { "-fsanitize=address,undefined,leak", "-fno-omit-frame-pointer", "-static-libasan" }
    linkoptions  { "-fsanitize=address,undefined,leak", "-fno-omit-frame-pointer", "-static-libasan" }

  -- Release (common)
  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "Speed"

  -- Release + macOS
  filter { "configurations:Release", "system:macosx" }
    buildoptions { "-Wno-initializer-overrides" }
    linkoptions  { "-Wno-initializer-overrides" }

  -- Profiling (common)
  filter "configurations:Profiling"
    defines { "DEBUG", "PROFILING", "TRACY_ENABLE" }
    symbols "On"
    optimize "On"
    links { "TracyClient", "c++" }

  -- Profiling + macOS
  filter { "configurations:Profiling", "system:macosx" }
    buildoptions { "-Wno-initializer-overrides" }
    linkoptions  { "-Wno-initializer-overrides" }

  filter {} -- reset at end


--
-- LIBRARIES -----------------------
--

-- LIBRARY::log
project "lib_log"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "log"
  files { "src/deps/log/log.c" }

-- LIBRARY::sokol
project "lib_sokol"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "sokol"
  files { "src/deps/sokol/sokol.c" }

  filter "system:macosx"
    defines { "SOKOL_METAL" }
    -- links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
    links { "Cocoa.framework", "Metal.framework", "MetalKit.framework", "QuartzCore.framework", "IOKit.framework" }
    buildoptions { "-x objective-c" }

  filter "system:linux"
    defines { "SOKOL_GLCORE" }
    links { "X11", "Xi", "Xcursor", "GL", "m" }

  filter "system:windows"
    links { "opengl32", "gdi32", "user32", "shell32", "ole32", "winmm" }
    defines { "SOKOL_WIN32_FORCE_MAIN", "SOKOL_GLCORE", "NK_POINTER_TYPE=uintptr_t", }

  filter {}


-- LIBRARY::hmm
project "lib_hmm"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "hmm"
  files { "src/deps/hmm/hmm.c" }

-- LIBRARY::stb
project "lib_stb"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "stb"
  buildoptions { "-Wno-deprecated-declarations" }
  files { "src/deps/stb/stb.c" }

-- LIBRARY::sepi
project "lib_sepi"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "sepi"
  buildoptions { "-Wno-deprecated-declarations" }
  files { "src/deps/sepi/sepi.c" }


--
-- APPS -----------------------
--

-- APP::playground (testing ideas)
-- project "app_playground"
--   kind "ConsoleApp"
--   language "C"
--   location ".build"
--   targetdir ".build/"
--   objdir ".build/obj"
--   targetname "app_playground"
--   includedirs { "src", "src/deps" }
--   links { "lib_log:static", "lib_stb:static", "lib_hmm:static", "lib_sepi:static", "lib_sokol:static", }
--   files { "src/app_playground.c" }

--   filter "system:macosx"
--     links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework", "m" }

--   filter "system:linux"
--     links { "X11", "Xi", "Xcursor", "GL", "m" }

--   filter "system:windows"
--     links { "opengl32", "gdi32", "user32", "shell32", "ole32", "winmm" }
--     defines { "SOKOL_WIN32_FORCE_MAIN", "NK_INCLUDE_FIXED_TYPES" }

--   filter {}

-- APP::pak
project "app_pak"
  kind "ConsoleApp"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "app_pak"
  includedirs { "src", "src/deps" }
  links { "lib_log:static", "lib_stb:static", "lib_sepi:static", "lib_sokol:static", }
  files { "src/app_pak.c" }

  filter "system:macosx"
    defines { "SOKOL_METAL" }
    links { "Cocoa.framework", "Metal.framework", "MetalKit.framework", "QuartzCore.framework", "IOKit.framework" }

  filter "system:linux"
    defines { "SOKOL_GLCORE" }
    links { "X11", "Xi", "Xcursor", "GL", "m" }

  filter "system:windows"
    defines { "SOKOL_GLCORE" }
    links { "opengl32", "gdi32", "user32", "shell32", "ole32", "winmm" }
    defines { "SOKOL_WIN32_FORCE_MAIN", "NK_INCLUDE_FIXED_TYPES" }

  filter {}


--
-- ACTIONS -----------------------
--

-- ACTION::glsl
newaction {
  trigger = "gen-code",
  description = "Generates code using templates",
  execute = function()
    os.execute("source ~/.zshrc && /opt/homebrew/bin/lua scripts/gen-code.lua")
  end
}

-- ACTION::glsl
newaction {
  trigger = "glsl",
  description = "Compile shaders into C headers",
  execute = function()
    os.execute("sokol-shdc -m default -i src/shaders/default.glsl -l glsl410 -f sokol -o src/shaders/default.glsl.h")
    os.execute("sokol-shdc -m bbox -i src/shaders/bbox.glsl -l glsl410 -f sokol -o src/shaders/bbox.glsl.h")
  end
}

-- ACTION::licence
newaction {
  trigger = "license",
  description = "Adds/Updates license header to all source files",
  execute = function()
    os.execute("lice -f license_header -e src/deps src")
  end
}

-- ACTION::clean
newaction {
  trigger = "clean",
  description = "Execute the program with optional arguments",
  execute = function()
    os.execute("make --no-print-directory -C .build -f mk_sqv.make clean")
  end
}

-- ACTION::app-playground
newaction {
  trigger = "app-playground",
  description = "run playground app",
  execute = function()
    os.execute(".build/app_playground")
  end
}

-- ACTION::app-pak
newaction {
  trigger = "app-pak",
  description = "run pak app",
  execute = function()
    -- os.execute(".build/app_pak --input=/Users/sepi/Games/Quake1/lq/pak0.pak")
    os.execute(".build/app_pak --input=/Users/sepi/Games/Quake1/lq")
  end
}
