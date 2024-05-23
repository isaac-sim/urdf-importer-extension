import argparse
import os
import sys
from typing import Callable, Dict

import omni.repo.ci


def pull_library_from_linbuild_usr_lib64(d: str, name: str):
    # Hack to be able to link to libasan.
    # Our toolchain is using an older version of GCC that can't statically link ASAN, so we need
    # to pull the library out of docker
    omni.repo.ci.launch(["${root}/repo${shell_ext}", "build", "--fetch", "-rd"])
    omni.repo.ci.launch(["_build/host-deps/linbuild/linbuild.sh", "--", "cp", f"/usr/lib64/{name}", d])


def main(args: argparse.Namespace):
    build_config = args.build_config

    extra_flags = []
    if build_config == "release":
        extra_flags.append("-r")
    elif build_config == "debug":
        extra_flags.append("-d")
    if not omni.repo.ci.is_windows():
        extra_flags.append("--no-docker")

    build_cmd = ["${root}/repo${shell_ext}", "build", "-x"] + extra_flags

    # Full rebuild config
    omni.repo.ci.launch(build_cmd)

    if build_config == "release":
        # Extensions verification for publishing (if publishing enabled)
        if omni.repo.ci.get_repo_config().get("repo_publish_exts", {}).get("enabled", True):
            omni.repo.ci.launch(["${root}/repo${shell_ext}", "publish_exts", "--verify"])

        # Tool to promote extensions to the public registry pipeline, if enabled (for apps)
        if omni.repo.ci.get_repo_config().get("repo_deploy_exts", {}).get("enabled", False):
            omni.repo.ci.launch(["${root}/repo${shell_ext}", "deploy_exts"])

    # Use repo_docs.enabled as indicator for whether to build docs
    repo_docs_enabled = omni.repo.ci.get_repo_config().get("repo_docs", {}).get("enabled", True)
    # repo_docs_enabled = repo_docs_enabled and omni.repo.ci.is_windows()

    # Docs
    if repo_docs_enabled:
        omni.repo.ci.launch(["${root}/repo${shell_ext}", "docs", "--config", build_config, "--warn-as-error=0"])
        # omni.repo.ci.launch(["${root}/repo${shell_ext}", "docs", "--config", build_config])

    # publish artifacts to teamcity
    print("##teamcity[publishArtifacts '_build/packages']")
    print("##teamcity[publishArtifacts '_build/**/*.log']")
