#

import importlib
import os

import carb
from omni.importer.urdf import new_extension_name as _new_ext
from omni.importer.urdf import old_extension_name as _old_ext

# Get the file name
_file_name = os.path.splitext(os.path.basename(__file__))[0]

_new_package = __package__.replace(_old_ext, new_ext)
carb.log_warn(
    f"{__package__}.{_file_name} has been deprecated in favor of {_new_package}.{_file_name}. Please update your code accordingly "
)

_module = importlib.import_module(f"{_new_package}.{_file_name}")

del os
del importlib
del carb

globals().update({k: v for k, v in _module.__dict__.items() if not k.startswith("_")})

del _new_package
del _file_name
del _module
del _old_ext
del _new_ext
