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
    filter {}

    cppdialect "C++17"

    filter "system:linux"
        toolset "clang"
        --toolset "gcc"
    filter "system:windows"
        toolset "msc-v141"
    filter {}
        
    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
        optimize "Off"

        filter "system:windows"
            symbols "Full"
        filter "system:linux"
            symbols "On"
            buildoptions "-g3"
        filter ""
        runtime "Debug"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "Full"
        symbols "Off"
        --flags {"LinkTimeOptimization"}
        floatingpoint "Fast"
        vectorextensions "AVX"
    filter {}

    require "oxide/import"

    project "doghook"
        kind "SharedLib"
        language "C++"
        targetdir "bin/%{cfg.buildcfg}"

        filter "system:linux"
            pchheader "src/precompiled.hh"
        filter "system:windows"
            pchheader "precompiled.hh"
        filter {}

        --[[
        filter{"configurations:Debug"}
            libdirs{"oxide/bin/Debug/"}
        filter{"configurations:Release"}
            libdirs{"oxide/bin/Release"}
        filter{}

        links{"oxide"}
        ]]--


        filter {"system:linux"}
            links{"X11", "Xfixes"}
        filter {}

        includedirs{"oxide/include"}
        links{"oxide"}

        pchsource "src/precompiled.cc"

        includedirs { "src" }
        files { "src/**.hh", "src/**.cc" }

        filter "system:linux"
            prebuildcommands {
                "{MKDIR} %{wks.location}/compile_commands/",
                "{TOUCH} %{wks.location}/compile_commands/%{cfg.shortname}.json",
                "{COPY} %{wks.location}/compile_commands/%{cfg.shortname}.json ../compile_commands.json"
            }
        filter "system:windows"
            prebuildcommands {
                "cmd.exe /c \"" .. "{MKDIR} %{wks.location}/compile_commands/",
                "cmd.exe /c \""  .. "{TOUCH} %{wks.location}/compile_commands/%{cfg.shortname}.json",
                "cmd.exe /c \""  .. "{COPY} %{wks.location}/compile_commands/%{cfg.shortname}.json ../compile_commands.json*"
            }

