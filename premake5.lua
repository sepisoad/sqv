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

    -- yes address sanitizer
    -- buildoptions { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }
    -- linkoptions  { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }

    -- in between
    -- buildoptions { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }
    -- linkoptions  { "-fsanitize=undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }

    -- no address sanitizer
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
  files { "src/deps/sepi/sepi.c" }
  buildoptions { "-Wno-deprecated-declarations" }

  filter "configurations:Debug"
    buildoptions { "-Wall", "-Wextra", "-Wconversion", "-Wdouble-promotion", "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-sign-conversion"}

  filter {}




--
-- APPS -----------------------
--

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

  filter "configurations:Debug"
    buildoptions { "-Wall", "-Wextra", "-Wconversion", "-Wdouble-promotion", "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-sign-conversion"}

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

-- APP::md1
project "app_md1"
  kind "ConsoleApp"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "app_md1"
  includedirs { "src", "src/deps" }
  links { "lib_log:static", "lib_stb:static", "lib_hmm:static", "lib_sepi:static", "lib_sokol:static", }
  files { "src/app_md1.c" }

  filter "configurations:Debug"
    buildoptions { "-Wall", "-Wextra", "-Wconversion", "-Wdouble-promotion", "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-sign-conversion"}

  filter "system:macosx"
    defines { "SOKOL_METAL" }
    links { "Cocoa.framework", "Metal.framework", "MetalKit.framework", "QuartzCore.framework", "IOKit.framework" }

  filter "system:linux"
    links { "X11", "Xi", "Xcursor", "GL", "m" }

  filter "system:windows"
    links { "opengl32", "gdi32", "user32", "shell32", "ole32", "winmm" }
    defines { "SOKOL_WIN32_FORCE_MAIN", "NK_INCLUDE_FIXED_TYPES" }

  filter {}


--
-- ACTIONS -----------------------
--

-- ACTION::gen-code
newaction {
  trigger = "gen-code",
  description = "Generates code using templates",
  execute = function()
    os.execute("source ~/.zshrc && /opt/homebrew/bin/lua scripts/gen-code.lua")
  end
}

-- ACTION::shader
newaction {
  trigger = "shader",
  description = "Compile shaders into C headers",
  execute = function()
    os.execute("sokol-shdc -m default -i src/shaders/default.glsl -l glsl410 -f sokol -o src/shaders/default.ogl.h")
    os.execute("sokol-shdc -m bbox -i src/shaders/bbox.glsl -l glsl410 -f sokol -o src/shaders/bbox.ogl.h")
    os.execute("sokol-shdc -m default -i src/shaders/default.glsl -l metal_macos -f sokol -o src/shaders/default.mtl.h")
    os.execute("sokol-shdc -m bbox -i src/shaders/bbox.glsl -l metal_macos -f sokol -o src/shaders/bbox.mtl.h")
    os.execute("sokol-shdc -m default -i src/shaders/default.glsl -l hlsl5 -f sokol -o src/shaders/default.d3d.h")
    os.execute("sokol-shdc -m bbox -i src/shaders/bbox.glsl -l hlsl5 -f sokol -o src/shaders/bbox.d3d.h")
  end
}

-- ACTION::analyze
newaction {
  trigger = "analyze",
  description = "run static code analysis using cppcheck",
  execute = function()
    os.execute("/usr/local/bin/cppcheck . -i res -i scripts -i src/deps/hmm -i src/deps/log -i src/deps/nuklear -i src/deps/rapidhash -i src/deps/sokol -i src/deps/stb -i src/deps/tracy -i src/shaders --enable=all --quiet --suppress=missingIncludeSystem --check-level=exhaustive --inline-suppr")
  end
}

-- ACTION::tidy
newaction {
  trigger = "analyze-clang",
  description = "run clang static analyzer",
  execute = function()
    os.execute("/opt/homebrew/opt/llvm/bin/scan-build --status-bugs make")
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

-- ACTION::app-md1
newaction {
  trigger = "app-md1",
  description = "run md1 app",
  execute = function()
    os.execute(".build/app_md1 --input=.keep/dog.mdl")
  end
}

-- ACTION::app-pak
newaction {
  trigger = "app-pak",
  description = "run pak app",
  execute = function()
    os.execute(".build/app_pak --input=/Users/sepi/Games/Quake1/lq")
  end
}
