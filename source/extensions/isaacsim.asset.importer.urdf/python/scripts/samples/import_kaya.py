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
import omni.kit.commands
import omni.ui as ui
from isaacsim.examples.browser import get_instance as get_browser_instance
from isaacsim.gui.components.ui_utils import btn_builder, get_style, setup_ui_headers
from omni.kit.viewport.utility.camera_state import ViewportCameraState
from pxr import Gf, PhysicsSchemaTools, Sdf, UsdLux, UsdPhysics

from .common import set_drive_parameters

EXTENSION_NAME = "Import Kaya"


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id: str):
        ext_manager = omni.kit.app.get_app().get_extension_manager()
        self._ext_id = ext_id
        self._extension_path = ext_manager.get_extension_path(ext_id)
        self._window = None

        self.example_name = "Kaya URDF"
        self.category = "Import Robots"

        get_browser_instance().register_example(
            name=self.example_name,
            execute_entrypoint=self._build_window,
            ui_hook=self._build_ui,
            category=self.category,
        )

    def _build_window(self):
        self._window = omni.ui.Window(
            EXTENSION_NAME, width=0, height=0, visible=False, dockPreference=ui.DockPreference.LEFT_BOTTOM
        )

    def _build_ui(self):

        with ui.VStack(spacing=5, height=0):

            title = "Import a Kaya Robot via URDF"
            doc_link = "https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/ext_omni_isaac_urdf.html"

            overview = "This Example shows you import an NVIDIA Kaya robot via URDF.\n\nPress the 'Open in IDE' button to view the source code."

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
                        "label": "Spin Robot",
                        "type": "button",
                        "text": "move",
                        "tooltip": "Spin the Robot in Place",
                        "on_clicked_fn": self._on_config_drives,
                    }
                    btn_builder(**dict)

    def on_shutdown(self):
        get_browser_instance().deregister_example(name=self.example_name, category=self.category)
        self._window = None

    def _menu_callback(self):
        if self._window is None:
            self._build_ui()
        self._window.visible = not self._window.visible

    def _on_load_robot(self):
        load_stage = asyncio.ensure_future(omni.usd.get_context().new_stage_async())
        asyncio.ensure_future(self._load_kaya(load_stage))

    async def _load_kaya(self, task):
        done, pending = await asyncio.wait({task})
        if task in done:
            status, import_config = omni.kit.commands.execute("URDFCreateImportConfig")
            import_config.merge_fixed_joints = True
            import_config.import_inertia_tensor = False
            # import_config.distance_scale = 1.0
            import_config.fix_base = False
            import_config.make_default_prim = True
            import_config.create_physics_scene = True
            omni.kit.commands.execute(
                "URDFParseAndImportFile",
                urdf_path=self._extension_path + "/data/urdf/robots/kaya/urdf/kaya.urdf",
                import_config=import_config,
            )

            camera_state = ViewportCameraState("/OmniverseKit_Persp")
            camera_state.set_position_world(Gf.Vec3d(-1.0, 1.5, 0.5), True)
            camera_state.set_target_world(Gf.Vec3d(0.0, 0.0, 0.0), True)

            stage = omni.usd.get_context().get_stage()
            scene = UsdPhysics.Scene.Define(stage, Sdf.Path("/physicsScene"))
            scene.CreateGravityDirectionAttr().Set(Gf.Vec3f(0.0, 0.0, -1.0))
            scene.CreateGravityMagnitudeAttr().Set(9.81)

            plane_path = "/groundPlane"
            PhysicsSchemaTools.addGroundPlane(
                stage, plane_path, "Z", 1500.0, Gf.Vec3f(0, 0, -0.25), Gf.Vec3f([0.5, 0.5, 0.5])
            )

            # make sure the ground plane is under root prim and not robot
            omni.kit.commands.execute(
                "MovePrimCommand", path_from=plane_path, path_to="/groundPlane", keep_world_transform=True
            )

            distantLight = UsdLux.DistantLight.Define(stage, Sdf.Path("/DistantLight"))
            distantLight.CreateIntensityAttr(500)

    def _on_config_robot(self):
        stage = omni.usd.get_context().get_stage()
        # Make all rollers spin freely by removing extra drive API
        for axle in range(0, 2 + 1):
            for ring in range(0, 1 + 1):
                for roller in range(0, 4 + 1):
                    prim_path = "/kaya/joints/roller_" + str(axle) + "_" + str(ring) + "_" + str(roller) + "_joint"
                    prim = stage.GetPrimAtPath(prim_path)
                    # omni.kit.commands.execute(
                    #     "UnapplyAPISchemaCommand",
                    #     api=UsdPhysics.DriveAPI,
                    #     prim=prim,
                    #     api_prefix="drive",
                    #     multiple_api_token="angular",
                    # )
                    prim.RemoveAPI(UsdPhysics.DriveAPI, "angular")

    def _on_config_drives(self):
        self._on_config_robot()  # make sure drives are configured first
        stage = omni.usd.get_context().get_stage()
        # set each axis to spin at a rate of 1 rad/s
        axle_0 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/kaya/joints/axle_0_joint"), "angular")
        axle_1 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/kaya/joints/axle_1_joint"), "angular")
        axle_2 = UsdPhysics.DriveAPI.Get(stage.GetPrimAtPath("/kaya/joints/axle_2_joint"), "angular")

        set_drive_parameters(axle_0, "velocity", math.degrees(1), 0, math.radians(1e7))
        set_drive_parameters(axle_1, "velocity", math.degrees(1), 0, math.radians(1e7))
        set_drive_parameters(axle_2, "velocity", math.degrees(1), 0, math.radians(1e7))
