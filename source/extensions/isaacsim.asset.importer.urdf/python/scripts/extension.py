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
from collections import namedtuple
from functools import partial
from pathlib import Path

import carb
import omni.client
import omni.ext
import omni.kit.app
import omni.kit.tool.asset_importer as ai
import omni.ui as ui
from isaacsim.asset.importer.urdf import _urdf
from isaacsim.asset.importer.urdf.scripts.ui import (
    btn_builder,
    cb_builder,
    dropdown_builder,
    float_builder,
    get_option_style,
    setup_ui_headers,
    str_builder,
    string_filed_builder,
)
from isaacsim.asset.importer.urdf.scripts.ui.UrdfJointWidgetWithID import UrdfJointWidget
from isaacsim.asset.importer.urdf.scripts.ui.UrdfOptionWidget import UrdfOptionWidget
from omni.kit.helper.file_utils import asset_types
from omni.kit.notification_manager import NotificationStatus, post_notification
from pxr import Sdf, Usd, UsdGeom, UsdPhysics, UsdUtils

# from .menu import make_menu_item_description
# from .ui_utils import (
#     btn_builder,
#     cb_builder,
#     dropdown_builder,
#     float_builder,
#     get_option_style,
#     setup_ui_headers,
#     str_builder,
# )


EXTENSION_NAME = "URDF Importer"


def is_urdf_file(path: str):
    _, ext = os.path.splitext(path.lower())
    result, entry = omni.client.stat(path)
    if result == omni.client.Result.OK:
        return ext in [".urdf", ".URDF"] and not entry.flags & omni.client.ItemFlags.CAN_HAVE_CHILDREN
    return False


def on_filter_item(item) -> bool:
    if not item or item.is_folder:
        return not (item.name == "Omniverse" or item.path.startswith("omniverse:"))
    return is_urdf_file(item.path)


def on_filter_folder(item) -> bool:
    if item and item.is_folder:
        return True
    else:
        return False


async def dir_exists(path: str, timeout: float = 10.0) -> bool:

    result, stat = await asyncio.wait_for(omni.client.stat_async(path), timeout)
    return result != omni.client.Result.ERROR_NOT_FOUND


def Singleton(class_):
    """A singleton decorator"""
    instances = {}

    def getinstance(*args, **kwargs):
        if class_ not in instances:
            instances[class_] = class_(*args, **kwargs)
        return instances[class_]

    return getinstance


class Extension(omni.ext.IExt):
    def on_startup(self, ext_id):
        self._ext_id = ext_id

        self.window = UrdfImporter(ext_id)

        self._delegate = UrdfImporterDelegate(
            "Urdf Importer", ["(.*\\.urdf$)|(.*\\.URDF$)"], ["Urdf Files (*.urdf, *.URDF)"], ext_id
        )
        ai.register_importer(self._delegate)

    def _menu_callback(self):
        self.window._window.visible = not self.window._window.visible

    def on_shutdown(self):
        self.window.on_shutdown()

        self._delegate.destroy()
        ai.remove_importer(self._delegate)


@Singleton
class UrdfImporter(object):
    def __init__(self, ext_id=None):
        self._ext_id = ext_id
        self._urdf_interface = _urdf.acquire_urdf_interface()
        self._usd_context = omni.usd.get_context()
        self._window = omni.ui.Window(
            EXTENSION_NAME, width=400, height=500, visible=False, dockPreference=ui.DockPreference.LEFT_BOTTOM
        )
        self._window.set_visibility_changed_fn(self._on_window)
        self._file_picker = None

        self._models = {}
        result, self._config = omni.kit.commands.execute("URDFCreateImportConfig")
        self._filepicker = None
        self._last_folder = None
        self._content_browser = None
        self._extension_path = omni.kit.app.get_app().get_extension_manager().get_extension_path(ext_id)
        self._imported_robot = None
        self._robot_model = None
        self._option_builder = UrdfOptionWidget(self._models, self._config)
        self.reset_config()

        # Additional UI that can be incorporated in the URDF Importer for Extended Workflow
        self.extra_frames = {}
        self.extra_frames_dict = {}

        self.build_ui()

    def build_new_optons(self):
        self._option_builder.build_options()

    def reset_config(self):
        # Set defaults
        self._config.set_merge_fixed_joints(True)
        self._config.set_replace_cylinders_with_capsules(False)
        self._config.set_convex_decomp(False)
        self._config.set_import_inertia_tensor(True)
        self._config.set_fix_base(True)
        self._config.set_self_collision(False)
        self._config.set_density(0.0)
        self._config.set_distance_scale(1.0)
        self._config.set_default_drive_type(1)
        self._config.set_default_drive_strength(1e3)
        self._config.set_default_position_drive_damping(1e2)
        self._config.set_up_vector(0, 0, 1)
        self._config.set_make_default_prim(True)
        self._config.set_parse_mimic(True)
        self._config.set_create_physics_scene(True)
        self._config.set_collision_from_visuals(False)

    def add_ui_frame(self, frame_location, frame_id):
        self.extra_frames_dict[frame_id] = (ui.Frame(style=get_option_style()), frame_location)
        if self.extra_frames[frame_location]:
            self.extra_frames[frame_location].add_child(self.extra_frames_dict[frame_id][0])

        return self.extra_frames_dict[frame_id][0]

    def remove_ui_frame(self, frame_id):
        if frame_id in self.extra_frames_dict:
            frame, frame_location = self.extra_frames_dict[frame_id]
            frame.visible = False
        self.extra_frames_dict.pop(frame_id)

    def get_frame_locations(self):
        return self.extra_frames.keys()

    def check_file_type(self, model=None):
        path = model.get_value_as_string()
        self.check_file_type_with_path(path)

    def check_file_type_with_path(self, path):
        if not path:
            return
        if is_urdf_file(path) and "omniverse:" not in path.lower():
            self._models["refresh_btn"].enabled = True
            result, self._robot_model = omni.kit.commands.execute(
                "URDFParseFile", urdf_path=path, import_config=self._config
            )
            if result:
                self._on_urdf_loaded()

    @property
    def config(self):
        return self._config

    @config.setter
    def config(self, value):
        self._config = value

    def update_robot_model(self, robot_model=None):
        if robot_model:
            self._robot_model = robot_model
        self._on_urdf_loaded()

    def build_ui(self):
        with self._window.frame:
            self._scrolling_frame = ui.ScrollingFrame(vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED)
        with self._scrolling_frame:
            with ui.VStack(spacing=2, height=0):

                self._build_source_ui()

                self.options_frame = ui.CollapsableFrame("Options", height=0, collapsed=False, style=get_option_style())
                with self.options_frame:
                    with ui.VStack():
                        self.build_new_optons()
                        ui.Spacer(height=5)

                self.extra_frames["extra"] = ui.VStack(height=0)
                for frame in self.extra_frames_dict:
                    self.extra_frames["extra"].add_child(self.extra_frames_dict[frame])
                self._build_import_ui()

        stage = self._usd_context.get_stage()
        if stage:
            if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.y:
                self._config.set_up_vector(0, 1, 0)
            if UsdGeom.GetStageUpAxis(stage) == UsdGeom.Tokens.z:
                self._config.set_up_vector(0, 0, 1)
            # units_per_meter = 1.0 / UsdGeom.GetStageMetersPerUnit(stage)
            # self._models["scale"].set_value(units_per_meter)

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

    def _on_urdf_loaded(self):
        self._models["import_btn"].enabled = True
        # self.robot_frame.visible = True
        # self.option_robot_frame.visible = True
        # with self.option_robot_frame:
        #     with ui.VStack():
        #         joint_frame = ui.CollapsableFrame(
        #             title="Joints",
        #             collapsed=True,
        #             height=0,
        #             style=get_option_style(),
        #             style_type_name_override="CollapsableFrame",
        #             horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
        #             vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
        #         )
        #         ui.Spacer(height=5)
        #         with joint_frame:
        #             with ui.VStack(height=0):
        #                 self._joint_widget = UrdfJointWidget(
        #                     self.config, self._robot_model.joints.values(), self._on_joint_changed
        #                 )
        self._option_builder.build_joint_widget(
            self._robot_model, self._robot_model.joints.values(), self._on_joint_changed
        )

    def _parse_mimic(self, value):
        self._config.set_parse_mimic(value)
        if self._joint_widget:
            self._joint_widget.update_mimic()

    def _on_joint_changed(self, joint):
        pass
        # self._robot_model.joints[joint.name] = joint
        # self._robot_model.joints[joint.name].drive = joint.drive

    def _build_source_ui(self):
        frame = ui.CollapsableFrame(
            title="Input",
            height=0,
            collapsed=False,
            style=get_option_style(),
            style_type_name_override="CollapsableFrame",
            horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
            vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
        )
        with frame:
            kwargs = {
                "default_val": "",
                "tooltip": "Click the Folder Icon to Set Filepath",
                "use_folder_picker": True,
                "item_filter_fn": on_filter_item,
                "bookmark_label": "Built In URDF Files",
                "bookmark_path": f"{self._extension_path}/data/urdf",
                "folder_dialog_title": "Select URDF File",
                "folder_button_title": "Select URDF",
            }
            with ui.VStack():
                self.extra_frames["input"] = ui.VStack(style=get_option_style())
                self._file_input_frame = ui.Frame(style=get_option_style())
                with self._file_input_frame:
                    with ui.VStack():
                        ui.Label("Input File", width=90)
                        with ui.HStack():
                            self._models["input_file"] = string_filed_builder(**kwargs)
                            self._models["input_file"].add_value_changed_fn(self.check_file_type)
                            ui.Spacer(width=2)
                            self._models["refresh_btn"] = ui.Button(
                                "Refresh",
                                clicked_fn=partial(self.check_file_type, self._models["input_file"]),
                                enabled=False,
                                width=ui.Pixel(30),
                            )
                            ui.Spacer(width=2)
                        ui.Spacer(height=5)
                self.robot_frame = ui.Frame(height=0)

    def set_file_input_visible(self, value):
        self._file_input_frame.visible = value

    def _build_import_ui(self):
        frame = ui.Frame(
            title="Import",
            # height=0,
            collapsed=False,
            style=get_option_style(),
            style_type_name_override="CollapsableFrame",
            horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
            vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
        )
        with frame:
            with ui.VStack(style=get_option_style(), spacing=5, height=0):

                # kwargs = {
                #     "label": "Output Directory",
                #     "type": "stringfield",
                #     "default_val": self.get_dest_folder(),
                #     "tooltip": "Click the Folder Icon to Set Filepath",
                #     "use_folder_picker": True,
                # }
                # self._models["dst_path"] = str_builder(**kwargs)

                # btn_builder("Import URDF", text="Select and Import", on_clicked_fn=self._parse_urdf)

                self._models["import_btn"] = btn_builder("", text="Import", on_clicked_fn=self.start_import)
                self._models["import_btn"].enabled = False

    def start_import(self, checked=False, path=None):
        basename = self._robot_model.name
        base_path = ""
        dest_path = self._models["dst_path"].get_value_as_string()
        if path == None:
            path = self._models["input_file"].get_value_as_string()
        if path:
            dest_path = self._models["dst_path"].get_value_as_string()
            base_path = path[: path.rfind("/")]
            basename = path[path.rfind("/") + 1 :]
            basename = basename[: basename.rfind(".")]
            if path.rfind("/") < 0:
                base_path = path[: path.rfind("\\")]
                basename = path[path.rfind("\\") + 1]

        import_as_reference = self._models.get("import_as_reference", True)
        if import_as_reference and dest_path == "Same as Imported Model(Default)" and base_path == "":
            no_path_window = ui.Window(
                "URDF Needs a destination Path",
                width=400,
                height=120,
                flags=ui.WINDOW_FLAGS_NO_SCROLLBAR
                | ui.WINDOW_FLAGS_MODAL
                | ui.WINDOW_FLAGS_NO_MOVE
                | ui.WINDOW_FLAGS_NO_RESIZE
                | ui.WINDOW_FLAGS_NO_CLOSE
                | ui.WINDOW_FLAGS_NO_COLLAPSE,
            )
            no_path_window.visible = True
            with no_path_window.frame:
                with ui.VStack():
                    ui.Label("No file source detected. Destination Path is required.")

                    def close_window():
                        no_path_window.visible = False

                    ui.Button("OK", clicked_fn=partial(close_window))
                return
        if import_as_reference and not checked and dest_path == "Same as Imported Model(Default)":
            dest_path = f"{base_path}/{basename}"
            label_txt = f"The model {basename}.usd and all additional import files will be saved at"
            label = ui.Label(label_txt)
            check_path_window = ui.Window(
                "URDF Confirm Path",
                width=max(len(label_txt), len(dest_path)) * 6,
                height=120,
                flags=ui.WINDOW_FLAGS_NO_SCROLLBAR | ui.WINDOW_FLAGS_MODAL,
            )

            def check_callback(do_continue, *args):
                if do_continue:
                    self.start_import(checked=True, path=path)
                check_path_window.visible = False

            with check_path_window.frame:
                with ui.VStack():
                    ui.Label(label_txt, alignment=ui.Alignment.CENTER)
                    ui.Label(f"{dest_path}", alignment=ui.Alignment.CENTER)
                    ui.Label(f"Would You Like to continue?", alignment=ui.Alignment.CENTER)
                    with ui.HStack():
                        ui.Button("Yes", clicked_fn=partial(check_callback, True))
                        ui.Button("No", clicked_fn=partial(check_callback, False))
            return
        else:
            if import_as_reference:
                if dest_path != "Same as Imported Model(Default)":
                    base_path = dest_path
                if base_path[-1] == "/":
                    base_path = base_path[:-1]
                dest_path = f"{base_path}/{basename}/{basename}.usd"
                task = asyncio.ensure_future(dir_exists(dest_path))
                if omni.client.stat(dest_path)[0] != omni.client.Result.ERROR_NOT_FOUND:
                    overwrite_window = ui.Window(
                        "URDF Confirm Overwrite",
                        width=300,
                        height=90,
                        flags=ui.WINDOW_FLAGS_NO_SCROLLBAR
                        | ui.WINDOW_FLAGS_NO_RESIZE
                        | ui.WINDOW_FLAGS_NO_MOVE
                        | ui.WINDOW_FLAGS_MODAL,
                    )

                    def overwrite_callback(do_continue, *args):
                        if do_continue:
                            self._load_robot(dest_path, path)
                        overwrite_window.visible = False

                    with overwrite_window.frame:
                        with ui.VStack():
                            ui.Label(f"The model already exists in the provided folder.", alignment=ui.Alignment.CENTER)
                            ui.Label(f"Overwrite?", alignment=ui.Alignment.CENTER)
                            with ui.HStack():
                                ui.Button("Yes", clicked_fn=partial(overwrite_callback, True))
                                ui.Button("No", clicked_fn=partial(overwrite_callback, False))

                    return

            self._load_robot(dest_path, path)

    def get_dest_folder(self):
        stage = omni.usd.get_context().get_stage()
        if stage:
            path = stage.GetRootLayer().identifier
            if not path.startswith("anon"):
                basepath = path[: path.rfind("/")]
                if path.rfind("/") < 0:
                    basepath = path[: path.rfind("\\")]
                return basepath
        return "Same as Imported Model(Default)"

    def _on_window(self, visible):
        if self._window.visible:
            # self.build_ui()
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
            # self._models["scale"].set_value(units_per_meter)
            self._models["dst_path"].set_value(self.get_dest_folder())

    def _load_robot(self, dst_path, path=None, **kargs):
        export_folder = str(dst_path)
        if str(export_folder) == "Same as Imported Model(Default)":
            export_folder = ""
        import_as_reference = self._models.get("import_as_reference", True)
        if not path:
            path = self._models["input_file"].get_value_as_string()
        else:
            if not self._robot_model:
                if is_urdf_file(path) and "omniverse:" not in path.lower():
                    result, self._robot_model = omni.kit.commands.execute(
                        "URDFParseFile", urdf_path=path, import_config=self._config
                    )
        base_path = ""
        if self._robot_model:
            dest_path = self._models["dst_path"].get_value_as_string()
            if export_folder:
                dest_path = export_folder
            if path:
                base_path = path[: path.rfind("/")]
                basename = path[path.rfind("/") + 1 :]
                basename = basename[: basename.rfind(".")]

                if path.rfind("/") < 0:
                    base_path = path[: path.rfind("\\")]
                    basename = path[path.rfind("\\") + 1]
            else:
                basename = self._robot_model.name
            if dest_path != "Same as Imported Model(Default)":
                base_path = dest_path  # + "/" + basename
            if base_path[-1] == "/":
                base_path = base_path[:-1]
            # if the path is not a usd file, add the usd extension
            if dst_path[-4:] != ".usd":
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
            if not import_as_reference:

                def add_robot_to_stage():
                    all_cache_stage = UsdUtils.StageCache.Get().GetAllStages()
                    # only 'one cache stage' could trigger import to current stage in cpp code
                    if len(all_cache_stage) == 1:
                        omni.kit.commands.execute(
                            "URDFImportRobot",
                            urdf_path=path,
                            urdf_robot=self._robot_model,
                            import_config=self._config,
                            dest_path="",
                        )
                        return all_cache_stage[0].GetRootLayer().identifier
                    else:
                        carb.log_warn("Cannot determine the 'active' USD stage, will import as reference")
                        import_as_reference = True

                async def import_with_clean_stage():
                    stage = omni.usd.get_context().get_stage()
                    for prim in stage.GetPrimAtPath("/").GetChildren():
                        stage.RemovePrim(prim.GetPath())
                    await omni.kit.app.get_app().next_update_async()
                    add_robot_to_stage()
                    await omni.kit.app.get_app().next_update_async()

                if self._models["clean_stage"].get_value_as_bool():
                    asyncio.ensure_future(import_with_clean_stage())
                else:
                    return add_robot_to_stage()
            if import_as_reference:
                self._config.set_make_default_prim(True)
                omni.kit.commands.execute(
                    "URDFImportRobot",
                    urdf_path=path,
                    urdf_robot=self._robot_model,
                    import_config=self._config,
                    dest_path=dest_path,
                )
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
                return dest_path
        else:
            return None

    def on_shutdown(self):
        _urdf.release_urdf_interface(self._urdf_interface)
        if self._window:
            self._window = None
        gc.collect()


class UrdfImporterDelegate(ai.AbstractImporterDelegate):
    """
    Urdf importer delegate implementation for Asset Importer AbstractImporterDelegate.
    """

    def __init__(self, name, filters, descriptions, ext_id):
        super().__init__()
        self._name = name
        self._filters = filters
        self._descriptions = descriptions
        self._progress_dialog = None
        self._importer = UrdfImporter(ext_id)
        # register the urdf icon to asset types
        ext_path = omni.kit.app.get_app().get_extension_manager().get_extension_path_by_module(__name__)
        icon_path = Path(ext_path).joinpath("icons").absolute()
        AssetTypeDef = namedtuple("AssetTypeDef", "glyph thumbnail matching_exts")
        known_asset_types = asset_types.known_asset_types()
        known_asset_types["urdf"] = AssetTypeDef(
            f"{icon_path}/icoFileURDF.png",
            f"{icon_path}/icoFileURDF.png",
            [".urdf", ".URDF"],
        )

    def show_destination_frame(self):
        return False

    def destroy(self):
        self._importer = None

    def _on_import_complete(self, file_paths):
        pass

    @property
    def name(self):
        return self._name

    @property
    def filter_regexes(self):
        return self._filters

    @property
    def filter_descriptions(self):
        return self._descriptions

    def build_options(self, paths):
        self._importer.reset_config()
        self._importer.build_new_optons()
        # self._importer.build_options_frame(paths)
        for path in paths:
            if self.is_supported_format(path):
                # only upate the joint info for the first supported file
                self._importer.check_file_type_with_path(path)
                return

    def supports_usd_stage_cache(self):
        return False

    async def convert_assets(self, paths, **kargs):
        if not paths:
            post_notification(
                "No file selected",
                "Please select a file to import",
                NotificationStatus.ERROR,
            )
            return None
        for path in paths:
            self._importer.start_import(path=path)
        # Don't need to return dest path here, _load_robot do the insertion to stage
        return {}
