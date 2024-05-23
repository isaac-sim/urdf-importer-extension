function include_physx()

    defines {  "PX_PHYSX_STATIC_LIB"}

    filter { "system:windows" }
        libdirs { "%{root}/_build/target-deps/nvtx/lib/x64" }
    filter {}
    filter { "system:linux" }
        libdirs { "%{root}/_build/target-deps/nvtx/lib64" }
    filter {}

    filter { "configurations:debug" }
        defines { "_DEBUG" }
    filter { "configurations:release" }
        defines { "NDEBUG" }
    filter {}

    filter { "system:windows", "platforms:x86_64", "configurations:debug" }
        libdirs {
            "%{root}/_build/target-deps/physx/bin/win.x86_64.vc142.md/debug",
        }
    filter { "system:windows", "platforms:x86_64", "configurations:release" }
        libdirs {
            "%{root}/_build/target-deps/physx/bin/win.x86_64.vc142.md/checked",
        }
    filter {}

    filter { "system:linux", "platforms:x86_64","configurations:debug" }
        libdirs {
            "%{root}/_build/target-deps/physx/bin/linux.clang/debug",
        }
    filter { "system:linux", "platforms:x86_64","configurations:release" }
        libdirs {
            "%{root}/_build/target-deps/physx/bin/linux.clang/checked",
        }
    filter {}

    links { "PhysXExtensions_static_64", "PhysX_static_64", "PhysXPvdSDK_static_64","PhysXCooking_static_64","PhysXCommon_static_64", "PhysXFoundation_static_64", "PhysXVehicle_static_64"}

    includedirs {
        "%{root}/_build/target-deps/physx/include",
        "%{root}/_build/target-deps/usd_ext_physics/%{cfg.buildcfg}/include",
    }

end

local ext = get_current_extension_info()
project_ext (ext)
-- C++ Carbonite plugin
project_ext_plugin(ext, "omni.importer.urdf.plugin")
    symbols "On"
    exceptionhandling "On"
    rtti "On"
    staticruntime "On"

    cppdialect "C++17"

    flags { "FatalCompileWarnings", "MultiProcessorCompile", "NoPCH", "NoIncrementalLink" }
    removeflags { "FatalCompileWarnings", "UndefinedIdentifiers" }

    add_files("impl", "plugins")

    include_physx()
    includedirs {
        "%{root}/_build/target-deps/pybind11/include",
        -- "%{root}/include/pch",
        "%{root}/_build/target-deps/nv_usd/%{cfg.buildcfg}/include",
        "%{root}/_build/target-deps/usd_ext_physics/%{cfg.buildcfg}/include",
        "%{root}/_build/target-deps/assimp/include",
        "%{root}/_build/target-deps/omni_physics/include",
        "%{root}/_build/target-deps/python/include",
        "%{root}/_build/target-deps/tinyxml2/include",
        "%{root}/_build/target-deps/omni_client_library/include",
        "%{target_deps}/rapidjson/include",
    }

    libdirs {
        "%{root}/_build/target-deps/nv_usd/%{cfg.buildcfg}/lib",
        "%{root}/_build/target-deps/usd_ext_physics/%{cfg.buildcfg}/lib",
        "%{root}/_build/target-deps/tinyxml2/lib",
        "%{root}/_build/target-deps/omni_client_library/%{cfg.buildcfg}",
        "%{kit_sdk_bin_dir}/exts/omni.usd.core/bin"
    }

    links {
        "gf", "tf", "sdf", "vt","usd", "usdGeom", "usdUtils", "usdShade", "usdImaging", "usdPhysics", "physicsSchemaTools", "physxSchema", "omni.usd", "assimp", "tinyxml2", "omniclient",
    }

    if os.target() == "linux" then
        includedirs {
            "%{root}/_build/target-deps/nv_usd/%{cfg.buildcfg}/include/boost",
            "%{root}/_build/target-deps/python/include/python3.10",
        }
        libdirs {
            "%{root}/_build/target-deps/assimp/lib64",
        }
        links {
            "stdc++fs"
        }
    else
        filter { "system:windows", "platforms:x86_64" }
            toolset "v142"
            filter { "system:windows", "platforms:x86_64" }
            defines { "_WIN32" }
            link_boost_for_windows({"boost_python310"})
        filter {}
        libdirs {
            "%{root}/_build/target-deps/tbb/lib/intel64/vc14",
            "%{root}/_build/target-deps/assimp/lib",
        }
    end


-- Python Bindings for Carobnite Plugin
project_ext_bindings {
    ext = ext,
    project_name = "omni.importer.urdf.python",
    module = "_urdf",
    src = "bindings",
    target_subdir = "omni/importer/urdf"
}
cppdialect "C++17"
repo_build.prebuild_link {
    { "python/scripts", ext.target_dir.."/omni/importer/urdf/scripts" },
    { "python/tests", ext.target_dir.."/omni/importer/urdf/tests" },
    { "docs", ext.target_dir.."/docs" },
    { "data", ext.target_dir.."/data" },
}


if os.target() == "linux" then
    repo_build.prebuild_copy {
        { "%{root}/_build/target-deps/assimp/lib64/lib**", ext.target_dir.."/bin" },
        { "%{root}/_build/target-deps/tinyxml2/lib/lib**", ext.target_dir.."/bin" },
    }
else
    repo_build.prebuild_copy {
        { "%{root}/_build/target-deps/assimp/bin/*.dll", ext.target_dir.."/bin" },
        { "%{root}/_build/target-deps/tinyxml2/bin/*.dll", ext.target_dir.."/bin" },
    }
end

repo_build.prebuild_copy {

    { "python/*.py", ext.target_dir.."/omni/importer/urdf" },
}
