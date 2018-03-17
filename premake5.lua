require("premake_modules/export-compile-commands")
require("premake_modules/cmake")

workspace "doghook"
    configurations { "Debug", "Release" }
    platforms { "x32" }

    location "premake"
    
    filter "system:windows"
        characterset "MBCS"

    filter {}

    filter "platforms:x32"
        architecture "x32"

    filter "system:windows"
        cppdialect "C++17"
    filter "system:linux"
        buildoptions "-std=c++2a"
    filter {}
        

    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
        optimize "Off"
        symbols "Full"
        runtime "Debug"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "Full"
        symbols "Off"
        flags {"LinkTimeOptimization"}

    filter {}

    project "doghook"
        filter "system:linux"
            toolset "clang"
        filter "system:windows"
            toolset "msc-v141"
            buildoptions{ "--driver-mode=cl" }
        filter {}

        kind "SharedLib"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"

        filter "system:linux"
            pchheader "src/precompiled.hh"
        filter "system:windows"
            pchheader "precompiled.hh"
        
        filter {}

        pchsource "src/precompiled.cc"

		includedirs { "src" }
        files { "src/**.hh", "src/**.cc" }
