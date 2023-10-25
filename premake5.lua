-- Shared build scripts from repo_build package.
repo_build = require("omni/repo/build")

-- Repo root
root = repo_build.get_abs_path(".")

-- Set the desired MSVC, WINSDK, and MSBUILD versions before executing the kit template premake configuration.
MSVC_VERSION = "14.27.29110"
WINSDK_VERSION = "10.0.18362.0"
MSBUILD_VERSION = "Current"

-- Execute the kit template premake configuration, which creates the solution, finds extensions, etc.
dofile("_repo/deps/repo_kit_tools/kit-template/premake5.lua")

-- The default kit dev app with extensions from this repo made available.
-- define_app("omni.app.kit.dev")
define_app("omni.importer.urdf.app")
