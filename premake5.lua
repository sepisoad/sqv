--
-- WORKSPACE -----------------------
--

workspace "ProjectWorkspace"
  configurations { "Debug", "Release", "Profiling" }
  location "."
  toolset "gcc"

  -- Debug (common)
  filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
    optimize "Off"

  -- Debug + macOS
  filter { "configurations:Debug", "system:macosx" }
    buildoptions { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }
    linkoptions  { "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-Wno-initializer-overrides" }

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

  -- Profiling + macOS
  filter { "configurations:Profiling", "system:macosx" }
    buildoptions { "-Wno-initializer-overrides" }
    linkoptions  { "-Wno-initializer-overrides" }

  filter {} -- reset at end (good hygiene)


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
    defines { "SOKOL_GLCORE" }
    links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
    buildoptions { "-x objective-c" }
  filter {}

  filter "system:linux"
    defines { "SOKOL_GLCORE" }
    links { "X11", "Xi", "Xcursor", "GL", "m" }
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
  links { "lib_log:static", "lib_stb:static", "lib_hmm:static", "lib_sepi:static", "lib_sokol:static", }
  files { "src/app_playground.c" }

  buildoptions { "-std=gnu11" }
  defines { "SOKOL_GLCORE" }
  defines { "_POSIX_C_SOURCE=199309L" } -- Needed for some C23 features

  filter "system:macosx"
    links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
  filter {}

  filter "system:linux"
    links { "X11", "Xi", "Xcursor", "GL", "m" }
  filter {}

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

  buildoptions { "-std=gnu11" }
  defines { "SOKOL_GLCORE" }

  filter "system:macosx"
    links { "Cocoa.framework", "OpenGL.framework", "IOKit.framework" }
  filter {}

  filter "system:linux"
    links { "X11", "Xi", "Xcursor", "GL", "m" }
  filter {}

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

-- ACTION::gen_module
newaction {
  trigger = "gen-module",
  description = "generate a c module scaffold",
  execute = function()
    os.execute("lua scripts/gen-module.lua")
  end
}

-- ACTION::app-playground
newaction {
  trigger = "app-playground",
  description = "run playground app",
  execute = function()
    os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_playground")
  end
}

-- ACTION::app-pak
newaction {
  trigger = "app-pak",
  description = "run pak app",
  execute = function()
    -- SUCCESSFULL:
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/id1/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/sprawl/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/copper/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/immortal/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/libre/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/PE1_landing/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/sacrilege/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/smej2_1.1/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/sprawl/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/tf/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/alkaline/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/bni/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/st_full/pak0.pak")
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/peril3.0/pak0.pak")

    -- FAILED:
    -- os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/peril3.0/pak2.pak")
    os.execute("LSAN_OPTIONS=suppressions=lsan.supp .build/app_pak --input=/home/sepi/Games/pc/quake1/MALICE/PAK0.PAK")
  end
}
