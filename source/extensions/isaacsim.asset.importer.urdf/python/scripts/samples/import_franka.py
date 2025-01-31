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

import asyncio
import math
import weakref

import omni
import omni.ui as ui
from isaacsim.examples.browser import get_instance as get_browser_instance
from isaacsim.gui.components.ui_utils import btn_builder, get_style, setup_ui_headers
from omni.kit.viewport.utility.camera_state import ViewportCameraState
from pxr import Gf, PhysicsSchemaTools, PhysxSchema, Sdf, UsdLux, UsdPhysics

from .common import set_drive_parameters

EXTENSION_NAME = "Import Franka"


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id: str):
        ext_manager = omni.kit.app.get_app().get_extension_manager()
        self._ext_id = ext_id
        self._extension_path = ext_manager.get_extension_path(ext_id)

        self.example_name = "Franka URDF"
        self.category = "Import Robots"

        get_browser_instance().register_example(
            name=self.example_name,
            execute_entrypoint=self._build_window,
            ui_hook=self._build_ui,
            category=self.category,
        )

    def _build_window(self):
        # self._window = omni.ui.Window(
        #     EXTENSION_NAME, width=0, height=0, visible=False, dockPreference=ui.DockPreference.LEFT_BOTTOM
        # )
        pass

    def _build_ui(self):

        with ui.VStack(spacing=5, height=0):

            title = "Import a Franka Panda via URDF"
            doc_link = "https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/ext_omni_isaac_urdf.html"

            overview = (
                "This Example shows you import a URDF.\n\nPress the 'Open in IDE' button to view the source code."
            )

            setup_ui_headers(self._ext_id, __file__, title, doc_link, overview)

            frame = ui.CollapsableFrame(
                title="Command Panel",
                height=0,
                collapsed=False,
                style=get_style(),
                style_type_name_override="CollapsableFrame",
                horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
                vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
            )
            with frame:
                with ui.VStack(style=get_style(), spacing=5):
                    dict = {
                        "label": "Load Robot",
                        "type": "button",
                        "text": "Load",
                        "tooltip": "Load a UR10 Robot into the Scene",
                        "on_clicked_fn": self._on_load_robot,
                    }
                    btn_builder(**dict)

                    dict = {
                        "label": "Configure Drives",
                        "type": "button",
                        "text": "Configure",
                        "tooltip": "Configure Joint Drives",
                        "on_clicked_fn": self._on_config_robot,
                    }
                    btn_builder(**dict)

                    dict = {
                        "label": "Move to Pose",
                        "type": "button",
                        "text": "move",
                        "tooltip": "Drive the Robot to a specific pose",
                        "on_clicked_fn": self._on_config_drives,
                    }
                    btn_builder(**dict)

    def on_shutdown(self):
        get_browser_instance().deregister_example(name=self.example_name, category=self.category)
        self._window = None

    def _menu_callback(self):
        self._window.visible = not self._window.visible

    def _on_load_robot(self):
        load_stage = asyncio.ensure_future(omni.usd.get_context().new_stage_async())
        asyncio.ensure_future(self._load_franka(load_stage))

    async def _load_franka(self, task):
        done, pending = await asyncio.wait({task})
        if task in done:
            status, import_config = omni.kit.commands.execute("URDFCreateImportConfig")
            import_config.merge_fixed_joints = False
            import_config.fix_base = True
            import_config.make_default_prim = True
            import_config.create_physics_scene = True
            omni.kit.commands.execute(
                "URDFParseAndImportFile",
                urdf_path=self._extension_path + "/data/urdf/robots/franka_description/robots/panda_arm_hand.urdf",
                import_config=import_config,
            )

            camera_state = ViewportCameraState("/OmniverseKit_Persp")
            camera_state.set_position_world(Gf.Vec3d(1.22, -1.24, 1.13), True)
            camera_state.set_target_world(Gf.Vec3d(-0.96, 1.08, 0.0), True)

            stage = omni.usd.get_context().get_stage()
            scene = UsdPhysics.Scene.Define(stage, Sdf.Path("/physicsScene"))
            scene.CreateGravityDirectionAttr().Set(Gf.Vec3f(0.0, 0.0, -1.0))
            scene.CreateGravityMagnitudeAttr().Set(9.81)
            plane_path = "/groundPlane"
            PhysicsSchemaTools.addGroundPlane(
                stage,
                plane_path,
                "Z",
                1500.0,
                Gf.Vec3f(0, 0, 0),
                Gf.Vec3f([0.5, 0.5, 0.5]),
            )
            # make sure the ground plane is under root prim and not robot
            omni.kit.commands.execute(
                "MovePrimCommand", path_from=plane_path, path_to="/groundPlane", keep_world_transform=True
            )
            distantLight = UsdLux.DistantLight.Define(stage, Sdf.Path("/DistantLight"))
            distantLight.CreateIntensityAttr(500)

    def _on_config_robot(self):
        stage = omni.usd.get_context().get_stage()

        # Set the solver parameters on the articulation
        PhysxSchema.PhysxArticulationAPI.Get(stage, "/panda").CreateSolverPositionIterationCountAttr(64)
        PhysxSchema.PhysxArticulationAPI.Get(stage, "/panda").CreateSolverVelocityIterationCountAttr(64)

        self.joint_1 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint1"), "angular")
        self.joint_2 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint2"), "angular")
        self.joint_3 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint3"), "angular")
        self.joint_4 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint4"), "angular")
        self.joint_5 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint5"), "angular")
        self.joint_6 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint6"), "angular")
        self.joint_7 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_joint7"), "angular")
        self.finger_1 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_finger_joint1"), "linear")
        self.finger_2 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/panda/joints/panda_finger_joint2"), "linear")

        # Set the drive mode, target, stiffness, damping and max force for each joint
        set_drive_parameters(self.joint_1, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.joint_2, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.joint_3, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.joint_4, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.joint_5, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.joint_6, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.joint_7, "position", math.degrees(0), math.radians(1e8), math.radians(1e7))
        set_drive_parameters(self.finger_1, "position", 0, 1e7, 1e6)
        set_drive_parameters(self.finger_2, "position", 0, 1e7, 1e6)

    def _on_config_drives(self):
        self._on_config_robot()  # make sure drives are configured first

        # Set the drive mode, target, stiffness, damping and max force for each joint
        set_drive_parameters(self.joint_1, "position", math.degrees(0.012))
        set_drive_parameters(self.joint_2, "position", math.degrees(-0.57))
        set_drive_parameters(self.joint_3, "position", math.degrees(0))
        set_drive_parameters(self.joint_4, "position", math.degrees(-2.81))
        set_drive_parameters(self.joint_5, "position", math.degrees(0))
        set_drive_parameters(self.joint_6, "position", math.degrees(3.037))
        set_drive_parameters(self.joint_7, "position", math.degrees(0.741))
        set_drive_parameters(self.finger_1, "position", 4)
        set_drive_parameters(self.finger_2, "position", 4)
