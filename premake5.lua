workspace "ProjectWorkspace"
toolset "gcc-14"
configurations { "Debug", "Release" }
location "."

filter "configurations:Debug"
defines { "DEBUG" }
symbols "On"
optimize "Off"

filter "configurations:Release"
defines { "NDEBUG" }
optimize "Speed"

filter {}

project "Host"
kind "ConsoleApp"
language "C"
targetdir ".ignore/build/"
objdir ".ignore/build/obj"
targetname "sqv"

files {
    "src/main.c",
    "src/hotreload.c",
    "deps/c/quake/*.c",
}

includedirs {
    "deps/c",
    "/usr/local/include",
}

links {
    "raylib",
    "GL",
    "X11",
    "Xrandr",
    "Xinerama",
    "Xi",
    "Xxf86vm",
    "Xcursor",
    "m",
    "pthread",
    "dl",
    "rt"
}

buildoptions {
    "-std=c23",
    "-fPIC",
    "-DRAYLIB_LIBTYPE=SHARED",
    "-DPLATFORM=PLATFORM_DESKTOP_RGFW"
}

newaction {
    trigger = "run",
    description = "Run the Host binary",
    execute = function()
        os.execute(".ignore/build/sqv")
    end
}

-- project "Plugin"
--     kind "SharedLib"
--     language "C"
--     targetdir ".ignore/build/"
--     objdir ".ignore/build/obj"
--     targetname "plugin"

--     files {
--         "src/plugin.c"
--     }

--     includedirs {
--         "deps/c",
--         "/usr/local/include"
--     }

--     links {
--         "raylib",
--         "GL",
--         "X11",
--         "Xrandr",
--         "Xinerama",
--         "Xi",
--         "Xxf86vm",
--         "Xcursor",
--         "m",
--         "pthread",
--         "dl",
--         "rt"
--     }

--     buildoptions {
--         "-std=c23",
--         "-fPIC",
--         "-DRAYLIB_LIBTYPE=SHARED",
--         "-DPLATFORM=PLATFORM_DESKTOP_RGFW"
--     }
