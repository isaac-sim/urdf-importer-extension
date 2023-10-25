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
import gc
import os
import weakref

import carb
import omni.client
import omni.ext
import omni.ui as ui
from omni.importer.urdf import _urdf
from omni.importer.urdf.scripts.ui import (
    btn_builder,
    cb_builder,
    dropdown_builder,
    float_builder,
    get_style,
    make_menu_item_description,
    setup_ui_headers,
    str_builder,
)
from omni.kit.menu.utils import MenuItemDescription, add_menu_items, remove_menu_items
from pxr import Sdf, Usd, UsdGeom, UsdPhysics

# from .menu import make_menu_item_description
# from .ui_utils import (
#     btn_builder,
#     cb_builder,
#     dropdown_builder,
#     float_builder,
#     get_style,
#     setup_ui_headers,
#     str_builder,
# )


EXTENSION_NAME = "URDF Importer"


def is_urdf_file(path: str):
    _, ext = os.path.splitext(path.lower())
    return ext in [".urdf", ".URDF"]


def on_filter_item(item) -> bool:
    if not item or item.is_folder:
        return not (item.name == "Omniverse" or item.path.startswith("omniverse:"))
    return is_urdf_file(item.path)


def on_filter_folder(item) -> bool:
    if item and item.is_folder:
        return True
    else:
        return False


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id):
        self._ext_id = ext_id
        self._urdf_interface = _urdf.acquire_urdf_interface()
        self._usd_context = omni.usd.get_context()
        self._window = omni.ui.Window(
            EXTENSION_NAME, width=400, height=500, visible=False, dockPreference=ui.DockPreference.LEFT_BOTTOM
        )
        self._window.set_visibility_changed_fn(self._on_window)

        menu_items = [
            make_menu_item_description(ext_id, EXTENSION_NAME, lambda a=weakref.proxy(self): a._menu_callback())
        ]
        self._menu_items = [MenuItemDescription(name="Workflows", sub_menu=menu_items)]
        add_menu_items(self._menu_items, "Isaac Utils")
        self._file_picker = None

        self._models = {}
        result, self._config = omni.kit.commands.execute("URDFCreateImportConfig")
        self._filepicker = None
        self._last_folder = None
        self._content_browser = None
        self._extension_path = omni.kit.app.get_app().get_extension_manager().get_extension_path(ext_id)
        self._imported_robot = None

        # Set defaults
        self._config.set_merge_fixed_joints(False)
        self._config.set_replace_cylinders_with_capsules(False)
        self._config.set_convex_decomp(False)
        self._config.set_fix_base(True)
        self._config.set_import_inertia_tensor(False)
        self._config.set_distance_scale(1.0)
        self._config.set_density(0.0)
        self._config.set_default_drive_type(1)
        self._config.set_default_drive_strength(1e7)
        self._config.set_default_position_drive_damping(1e5)
        self._config.set_self_collision(False)
        self._config.set_up_vector(0, 0, 1)
        self._config.set_make_default_prim(True)
        self._config.set_create_physics_scene(True)
        self._config.set_collision_from_visuals(False)

    def build_ui(self):
        with self._window.frame:
            with ui.VStack(spacing=5, height=0):

                self._build_info_ui()

                self._build_options_ui()

                self._build_import_ui()

        stage = self._usd_context.get_stage()
        if stage:
            if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.y:
                self._config.set_up_vector(0, 1, 0)
            if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.z:
                self._config.set_up_vector(0, 0, 1)
            units_per_meter = 1.0 / UsdGeom.GetStageMetersPerUnit(stage)
            self._models["scale"].set_value(units_per_meter)

        async def dock_window():
            await omni.kit.app.get_app().next_update_async()

            def dock(space, name, location, pos=0.5):
                window = omni.ui.Workspace.get_window(name)
                if window and space:
                    window.dock_in(space, location, pos)
                return window

            tgt = ui.Workspace.get_window("Viewport")
            dock(tgt, EXTENSION_NAME, omni.ui.DockPosition.LEFT, 0.33)
            await omni.kit.app.get_app().next_update_async()

        self._task = asyncio.ensure_future(dock_window())

    def _build_info_ui(self):
        title = EXTENSION_NAME
        doc_link = "https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/ext_omni_isaac_urdf.html"

        overview = "This utility is used to import URDF representations of robots into Isaac Sim. "
        overview += "URDF is an XML format for representing a robot model in ROS."
        overview += "\n\nPress the 'Open in IDE' button to view the source code."

        setup_ui_headers(self._ext_id, __file__, title, doc_link, overview)

    def _build_options_ui(self):
        frame = ui.CollapsableFrame(
            title="Import Options",
            height=0,
            collapsed=False,
            style=get_style(),
            style_type_name_override="CollapsableFrame",
            horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
            vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
        )
        with frame:
            with ui.VStack(style=get_style(), spacing=5, height=0):
                cb_builder(
                    label="Merge Fixed Joints",
                    tooltip="Consolidate links that are connected by fixed joints.",
                    on_clicked_fn=lambda m, config=self._config: config.set_merge_fixed_joints(m),
                )
                cb_builder(
                    label="Replace Cylinders with Capsules",
                    tooltip="Replace Cylinder collision bodies by capsules.",
                    on_clicked_fn=lambda m, config=self._config: config.set_replace_cylinders_with_capsules(m),
                )
                cb_builder(
                    "Fix Base Link",
                    tooltip="Fix the robot base robot to where it's imported in world coordinates.",
                    default_val=True,
                    on_clicked_fn=lambda m, config=self._config: config.set_fix_base(m),
                )
                cb_builder(
                    "Import Inertia Tensor",
                    tooltip="Load inertia tensor directly from the URDF.",
                    on_clicked_fn=lambda m, config=self._config: config.set_import_inertia_tensor(m),
                )
                self._models["scale"] = float_builder(
                    "Stage Units Per Meter",
                    default_val=1.0,
                    tooltip="Sets the scaling factor to match the units used in the URDF. Default Stage units are (cm).",
                )
                self._models["scale"].add_value_changed_fn(
                    lambda m, config=self._config: config.set_distance_scale(m.get_value_as_float())
                )
                self._models["density"] = float_builder(
                    "Link Density",
                    default_val=0.0,
                    tooltip="Density value to compute mass based on link volume. Use 0.0 to automatically compute density.",
                )
                self._models["density"].add_value_changed_fn(
                    lambda m, config=self._config: config.set_density(m.get_value_as_float())
                )
                dropdown_builder(
                    "Joint Drive Type",
                    items=["None", "Position", "Velocity"],
                    default_val=1,
                    on_clicked_fn=lambda i, config=self._config: config.set_default_drive_type(
                        0 if i == "None" else (1 if i == "Position" else 2)
                    ),
                    tooltip="Default Joint drive type.",
                )
                self._models["drive_strength"] = float_builder(
                    "Joint Drive Strength",
                    default_val=1e4,
                    tooltip="Joint stiffness for position drive, or damping for velocity driven joints. Set to -1 to prevent this parameter from getting used.",
                )
                self._models["drive_strength"].add_value_changed_fn(
                    lambda m, config=self._config: config.set_default_drive_strength(m.get_value_as_float())
                )
                self._models["position_drive_damping"] = float_builder(
                    "Joint Position Damping",
                    default_val=1e3,
                    tooltip="Default damping value when drive type is set to Position. Set to -1 to prevent this parameter from getting used.",
                )
                self._models["position_drive_damping"].add_value_changed_fn(
                    lambda m, config=self._config: config.set_default_position_drive_damping(m.get_value_as_float())
                )
                self._models["clean_stage"] = cb_builder(
                    label="Clear Stage", tooltip="Clear the Stage prior to loading the URDF."
                )
                dropdown_builder(
                    "Normals Subdivision",
                    items=["catmullClark", "loop", "bilinear", "none"],
                    default_val=2,
                    on_clicked_fn=lambda i, dict={
                        "catmullClark": 0,
                        "loop": 1,
                        "bilinear": 2,
                        "none": 3,
                    }, config=self._config: config.set_subdivision_scheme(dict[i]),
                    tooltip="Mesh surface normal subdivision scheme. Use `none` to avoid overriding authored values.",
                )
                cb_builder(
                    "Convex Decomposition",
                    tooltip="Decompose non-convex meshes into convex collision shapes. If false, convex hull will be used.",
                    on_clicked_fn=lambda m, config=self._config: config.set_convex_decomp(m),
                )
                cb_builder(
                    "Self Collision",
                    tooltip="Enables self collision between adjacent links.",
                    on_clicked_fn=lambda m, config=self._config: config.set_self_collision(m),
                )
                cb_builder(
                    "Collision From Visuals",
                    tooltip="Creates collision geometry from visual geometry.",
                    on_clicked_fn=lambda m, config=self._config: config.set_collision_from_visuals(m),
                )
                cb_builder(
                    "Create Physics Scene",
                    tooltip="Creates a default physics scene on the stage on import.",
                    default_val=True,
                    on_clicked_fn=lambda m, config=self._config: config.set_create_physics_scene(m),
                )
                cb_builder(
                    "Create Instanceable Asset",
                    tooltip="If true, creates an instanceable version of the asset. Meshes will be saved in a separate USD file",
                    default_val=False,
                    on_clicked_fn=lambda m, config=self._config: config.set_make_instanceable(m),
                )
                self._models["instanceable_usd_path"] = str_builder(
                    "Instanceable USD Path",
                    tooltip="USD file to store instanceable meshes in",
                    default_val="./instanceable_meshes.usd",
                    use_folder_picker=True,
                    folder_dialog_title="Select Output File",
                    folder_button_title="Select File",
                )
                self._models["instanceable_usd_path"].add_value_changed_fn(
                    lambda m, config=self._config: config.set_instanceable_usd_path(m.get_value_as_string())
                )

    def _build_import_ui(self):
        frame = ui.CollapsableFrame(
            title="Import",
            height=0,
            collapsed=False,
            style=get_style(),
            style_type_name_override="CollapsableFrame",
            horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
            vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
        )
        with frame:
            with ui.VStack(style=get_style(), spacing=5, height=0):

                def check_file_type(model=None):
                    path = model.get_value_as_string()
                    if is_urdf_file(path) and "omniverse:" not in path.lower():
                        self._models["import_btn"].enabled = True
                    else:
                        carb.log_warn(f"Invalid path to URDF: {path}")

                kwargs = {
                    "label": "Input File",
                    "default_val": "",
                    "tooltip": "Click the Folder Icon to Set Filepath",
                    "use_folder_picker": True,
                    "item_filter_fn": on_filter_item,
                    "bookmark_label": "Built In URDF Files",
                    "bookmark_path": f"{self._extension_path}/data/urdf",
                    "folder_dialog_title": "Select URDF File",
                    "folder_button_title": "Select URDF",
                }
                self._models["input_file"] = str_builder(**kwargs)
                self._models["input_file"].add_value_changed_fn(check_file_type)

                kwargs = {
                    "label": "Output Directory",
                    "type": "stringfield",
                    "default_val": self.get_dest_folder(),
                    "tooltip": "Click the Folder Icon to Set Filepath",
                    "use_folder_picker": True,
                }
                self.dest_model = str_builder(**kwargs)

                # btn_builder("Import URDF", text="Select and Import", on_clicked_fn=self._parse_urdf)

                self._models["import_btn"] = btn_builder("Import", text="Import", on_clicked_fn=self._load_robot)
                self._models["import_btn"].enabled = False

    def get_dest_folder(self):
        stage = omni.usd.get_context().get_stage()
        if stage:
            path = stage.GetRootLayer().identifier
            if not path.startswith("anon"):
                basepath = path[: path.rfind("/")]
                if path.rfind("/") < 0:
                    basepath = path[: path.rfind("\\")]
                return basepath
        return "(same as source)"

    def _menu_callback(self):
        self._window.visible = not self._window.visible

    def _on_window(self, visible):
        if self._window.visible:
            self.build_ui()
            self._events = self._usd_context.get_stage_event_stream()
            self._stage_event_sub = self._events.create_subscription_to_pop(
                self._on_stage_event, name="urdf importer stage event"
            )
        else:
            self._events = None
            self._stage_event_sub = None

    def _on_stage_event(self, event):
        stage = self._usd_context.get_stage()
        if event.type == int(omni.usd.StageEventType.OPENED) and stage:
            if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.y:
                self._config.set_up_vector(0, 1, 0)
            if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.z:
                self._config.set_up_vector(0, 0, 1)
            units_per_meter = 1.0 / UsdGeom.GetStageMetersPerUnit(stage)
            self._models["scale"].set_value(units_per_meter)
            self.dest_model.set_value(self.get_dest_folder())

    def _load_robot(self, path=None):
        path = self._models["input_file"].get_value_as_string()
        if path:
            dest_path = self.dest_model.get_value_as_string()
            base_path = path[: path.rfind("/")]
            basename = path[path.rfind("/") + 1 :]
            basename = basename[: basename.rfind(".")]
            if path.rfind("/") < 0:
                base_path = path[: path.rfind("\\")]
                basename = path[path.rfind("\\") + 1]

            if dest_path != "(same as source)":
                base_path = dest_path  # + "/" + basename

            dest_path = "{}/{}/{}.usd".format(base_path, basename, basename)
            # counter = 1
            # while result[0] == Result.OK:
            #     dest_path = "{}/{}_{:02}.usd".format(base_path, basename, counter)
            #     result = omni.client.read_file(dest_path)
            #     counter +=1
            # result = omni.client.read_file(dest_path)
            # if
            #     stage = Usd.Stage.Open(dest_path)
            # else:
            # stage = Usd.Stage.CreateNew(dest_path)
            # UsdGeom.SetStageUpAxis(stage, UsdGeom.Tokens.z)
            omni.kit.commands.execute(
                "URDFParseAndImportFile", urdf_path=path, import_config=self._config, dest_path=dest_path
            )
            # print("Created file, instancing it now")
            stage = Usd.Stage.Open(dest_path)
            prim_name = str(stage.GetDefaultPrim().GetName())
            # print(prim_name)
            # stage.Save()
            def add_reference_to_stage():
                current_stage = omni.usd.get_context().get_stage()
                if current_stage:
                    prim_path = omni.usd.get_stage_next_free_path(
                        current_stage, str(current_stage.GetDefaultPrim().GetPath()) + "/" + prim_name, False
                    )
                    robot_prim = current_stage.OverridePrim(prim_path)
                    if "anon:" in current_stage.GetRootLayer().identifier:
                        robot_prim.GetReferences().AddReference(dest_path)
                    else:
                        robot_prim.GetReferences().AddReference(
                            omni.client.make_relative_url(current_stage.GetRootLayer().identifier, dest_path)
                        )
                    if self._config.create_physics_scene:
                        UsdPhysics.Scene.Define(current_stage, Sdf.Path("/physicsScene"))

            async def import_with_clean_stage():
                await omni.usd.get_context().new_stage_async()
                await omni.kit.app.get_app().next_update_async()
                add_reference_to_stage()
                await omni.kit.app.get_app().next_update_async()

            if self._models["clean_stage"].get_value_as_bool():
                asyncio.ensure_future(import_with_clean_stage())
            else:
                add_reference_to_stage()

    def on_shutdown(self):
        _urdf.release_urdf_interface(self._urdf_interface)
        remove_menu_items(self._menu_items, "Isaac Utils")
        if self._window:
            self._window = None
        gc.collect()
