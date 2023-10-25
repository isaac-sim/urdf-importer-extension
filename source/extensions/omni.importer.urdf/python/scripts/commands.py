# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os

import omni.client
import omni.kit.commands

# import omni.kit.utils
from omni.client._omniclient import Result
from omni.importer.urdf import _urdf
from pxr import Usd


class URDFCreateImportConfig(omni.kit.commands.Command):
    """
    Returns an ImportConfig object that can be used while parsing and importing.
    Should be used with `URDFParseFile` and `URDFParseAndImportFile` commands

    Returns:
        :obj:`omni.importer.urdf._urdf.ImportConfig`: Parsed URDF stored in an internal structure.

    """

    def __init__(self) -> None:
        pass

    def do(self) -> _urdf.ImportConfig:
        return _urdf.ImportConfig()

    def undo(self) -> None:
        pass


class URDFParseFile(omni.kit.commands.Command):
    """
    This command parses a given urdf and returns a UrdfRobot object

    Args:
        arg0 (:obj:`str`): The absolute path to where the urdf file is

        arg1 (:obj:`omni.importer.urdf._urdf.ImportConfig`): Import Configuration

    Returns:
        :obj:`omni.importer.urdf._urdf.UrdfRobot`: Parsed URDF stored in an internal structure.
    """

    def __init__(self, urdf_path: str = "", import_config: _urdf.ImportConfig = _urdf.ImportConfig()) -> None:
        self._root_path, self._filename = os.path.split(os.path.abspath(urdf_path))
        self._import_config = import_config
        self._urdf_interface = _urdf.acquire_urdf_interface()
        pass

    def do(self) -> _urdf.UrdfRobot:
        return self._urdf_interface.parse_urdf(self._root_path, self._filename, self._import_config)

    def undo(self) -> None:
        pass


class URDFParseAndImportFile(omni.kit.commands.Command):
    """
    This command parses and imports a given urdf and returns a UrdfRobot object

    Args:
        arg0 (:obj:`str`): The absolute path to where the urdf file is

        arg1 (:obj:`omni.importer.urdf._urdf.ImportConfig`): Import Configuration

        arg2 (:obj:`str`): destination path for robot usd. Default is "" which will load the robot in-memory on the open stage.

    Returns:
        :obj:`str`: Path to the robot on the USD stage.
    """

    def __init__(self, urdf_path: str = "", import_config=_urdf.ImportConfig(), dest_path: str = "") -> None:
        self.dest_path = dest_path
        self._urdf_path = urdf_path
        self._root_path, self._filename = os.path.split(os.path.abspath(urdf_path))
        self._import_config = import_config
        self._urdf_interface = _urdf.acquire_urdf_interface()
        pass

    def do(self) -> str:
        status, imported_robot = omni.kit.commands.execute(
            "URDFParseFile", urdf_path=self._urdf_path, import_config=self._import_config
        )
        if self.dest_path:
            self.dest_path = self.dest_path.replace(
                "\\", "/"
            )  # Omni client works with both slashes cross platform, making it standard to make it easier later on
            result = omni.client.read_file(self.dest_path)
            if result[0] != Result.OK:
                stage = Usd.Stage.CreateNew(self.dest_path)
                stage.Save()
        return self._urdf_interface.import_robot(
            self._root_path, self._filename, imported_robot, self._import_config, self.dest_path
        )

    def undo(self) -> None:
        pass


omni.kit.commands.register_all_commands_in_module(__name__)
