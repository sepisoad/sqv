--
-- WORKSPACE -----------------------
--

workspace "ProjectWorkspace"
  configurations { "Debug", "Release" }
    location "."
    toolset "gcc"

  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
    optimize "Off"
    buildoptions { "-fsanitize=address,undefined,leak", "-fno-omit-frame-pointer", "-static-libasan" }
    linkoptions { "-fsanitize=address,undefined,leak", "-fno-omit-frame-pointer", "-static-libasan" }

  filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "Speed"

--
-- MODULES -----------------------
--

-- MODULE::log
project "module_log"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "log"
  files { "src/deps/log/log.c" }

-- MODULE::sokol
project "module_sokol"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "sokol"
  files { "src/deps/sokol/sokol.c" }

  filter "system:macosx"
    defines { "SOKOL_GLCORE" }
    links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
    buildoptions { "-x objective-c" }

  filter "system:linux"
    defines { "SOKOL_GLCORE" }
    links { "X11", "Xi", "Xcursor", "GL", "m" }

-- MODULE::hmm
project "module_hmm"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "hmm"
  files { "src/deps/hmm/hmm.c" }

-- MODULE::stb
project "module_stb"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "stb"
  buildoptions { "-Wno-deprecated-declarations" }
  files { "src/deps/stb/stb.c" }

-- MODULE::sepi
project "module_sepi"
  kind "StaticLib"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "sepi"
  buildoptions { "-Wno-deprecated-declarations" }
  buildoptions { "-std=gnu11" }
  defines { "USE_MEM_DEBUGGER" }
  files { "src/deps/sepi/sepi.c" }

--
-- APPS -----------------------
--

-- APP::playground (testing ideas)
project "app_playground"
  kind "ConsoleApp"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "app_playground"
  includedirs { "src", "src/deps" }
  links { "module_log:static", "module_stb:static", "module_hmm:static", "module_sepi:static", "module_sokol:static", }
  files { "src/app_playground.c" }

  buildoptions { "-std=gnu11" }
  defines { "SOKOL_GLCORE" }
  defines { "_POSIX_C_SOURCE=199309L" } -- Needed for some C23 features

  filter "system:macosx"
    links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }

  filter "system:linux"
    links { "X11", "Xi", "Xcursor", "GL", "m" }

-- APP::pak
project "app_pak"
  kind "ConsoleApp"
  language "C"
  location ".build"
  targetdir ".build/"
  objdir ".build/obj"
  targetname "app_pak"
  includedirs { "src", "src/deps" }
  links { "module_log:static", "module_stb:static", "module_hmm:static", "module_sepi:static", "module_sokol:static", }
  files { "src/app_pak.c" }

  buildoptions { "-std=gnu11" }
  defines { "SOKOL_GLCORE" }
  defines { "_POSIX_C_SOURCE=199309L" } -- Needed for some C23 features

  filter "system:macosx"
    links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }

  filter "system:linux"
    links { "X11", "Xi", "Xcursor", "GL", "m" }

--
-- ACTIONS -----------------------
--

-- ACTION::glsl
newaction {
  trigger = "glsl",
  description = "Compile shaders into C headers",
  execute = function()
    os.execute("sokol-shdc -m default -i src/shaders/default.glsl -l glsl410 -f sokol -o src/shaders/default.glsl.h")
    os.execute("sokol-shdc -m bbox -i src/shaders/bbox.glsl -l glsl410 -f sokol -o src/shaders/bbox.glsl.h")
    os.execute("sokol-shdc -m debug -i src/shaders/debug.glsl -l glsl410 -f sokol -o src/shaders/debug.glsl.h")
  end
}

-- ACTION::licence
newaction {
  trigger = "license",
  description = "Adds/Updates license header to all source files",
  execute = function()
    os.execute("lice -f LICENSE_HEADER -e src/deps src")
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

newaction {
  trigger = "app_playground",
  description = "run playground app",
  execute = function()
    os.execute("LSAN_OPTIONS=suppressions=~/Documents/lsan.supp .build/app_playground")
  end
}

newaction {
  trigger = "app_pak",
  description = "run pak app",
  execute = function()
    os.execute("LSAN_OPTIONS=suppressions=~/Documents/lsan.supp .build/app_pak")
  end
}
