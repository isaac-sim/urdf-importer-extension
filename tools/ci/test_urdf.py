import argparse
import os
import sys
from typing import Callable, Dict

import omni.repo.ci


def main(args: argparse.Namespace):
    build_config_arg = ["-c", args.build_config]
    test_cmd = ["${root}/repo${shell_ext}", "test"] + build_config_arg + args.extra_args
    print(test_cmd)
    # Run test
    omni.repo.ci.launch(test_cmd)
