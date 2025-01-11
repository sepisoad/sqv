workspace "ProjectWorkspace"
    toolset "gcc"
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
        "src/*.c",
    }
    
    includedirs {
        "res/shaders",
        "/usr/local/include",
    }
    
    links { "X11", "Xi", "Xcursor", "GL", "m" }
    
    buildoptions { "-std=c2x" }
    
    newaction {
        trigger = "glsl",
        description = "compile shaders into c headers",
        execute = function()
            os.execute("sokol-shdc -i res/shaders/shader.glsl -l glsl410 -f sokol -o res/shaders/shader.h")
        end
    }
    newaction {
        trigger = "run",
        description = "execute the program",
        execute = function()
            os.execute(".ignore/build/sqv")
        end
    }
