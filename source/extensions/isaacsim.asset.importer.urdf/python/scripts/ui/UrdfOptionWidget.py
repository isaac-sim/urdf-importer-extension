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

import omni.ui as ui
from isaacsim.asset.importer.urdf.scripts.ui.UrdfJointWidgetWithID import JointSettingMode

from .style import get_option_style
from .ui_utils import add_folder_picker_icon, checkbox_builder, float_field_builder, format_tt, string_filed_builder
from .UrdfJointWidgetWithID import UrdfJointWidget


def option_header(collapsed, title):
    with ui.HStack(height=22):
        ui.Spacer(width=4)
        with ui.VStack(width=10):
            ui.Spacer()
            if collapsed:
                triangle = ui.Triangle(height=7, width=5)
                triangle.alignment = ui.Alignment.RIGHT_CENTER
            else:
                triangle = ui.Triangle(height=5, width=7)
                triangle.alignment = ui.Alignment.CENTER_BOTTOM
            ui.Spacer()
        ui.Spacer(width=4)
        ui.Label(title, name="collapsable_header", width=0)
        ui.Spacer(width=3)
        ui.Line()


def option_frame(title, build_content_fn, collapse_fn=None):
    with ui.CollapsableFrame(
        title, name="option", height=0, collapsed=False, build_header_fn=option_header, collapsed_changed_fn=collapse_fn
    ):
        with ui.HStack():
            ui.Spacer(width=2)
            build_content_fn()
            ui.Spacer(width=2)


class UrdfOptionWidget:
    def __init__(self, models, config):
        self._models = models
        self._config = config
        self._joint_widget = None
        self._joint_widget_container = None

    @property
    def models(self):
        return self._models

    @property
    def config(self):
        return self._config

    def build_options(self):
        with ui.VStack(style=get_option_style()):
            self._build_model_frame()
            self._build_links_frame()
            self._build_joint_drive_frame()
            self._build_colliders_frame()

    def _build_model_frame(self):
        def build_model_content():
            with ui.VStack(spacing=4):
                with ui.HStack(height=26):
                    self._import_collection = ui.RadioCollection()
                    with ui.VStack(width=0, alignment=ui.Alignment.LEFT_CENTER):
                        ui.Spacer()
                        ui.RadioButton(
                            width=20,
                            height=20,
                            radio_collection=self._import_collection,
                            alignment=ui.Alignment.LEFT_CENTER,
                        )
                        ui.Spacer()
                    ui.Spacer(width=4)
                    ui.Label(
                        "Create in Stage",
                        width=90,
                        mouse_pressed_fn=lambda x, y, m, w: self._import_collection.model.set_value(0),
                    )
                    ui.Spacer(width=10)
                    with ui.VStack(width=0):
                        ui.Spacer()
                        ui.RadioButton(
                            width=20,
                            height=20,
                            radio_collection=self._import_collection,
                            alignment=ui.Alignment.LEFT_CENTER,
                        )
                        ui.Spacer()
                    ui.Spacer(width=4)
                    ui.Label(
                        "Referenced Model",
                        mouse_pressed_fn=lambda x, y, m, w: self._import_collection.model.set_value(1),
                    )
                    ui.Spacer(width=50)
                    self._import_collection.model.set_value(1)
                    self._import_collection.model.add_value_changed_fn(lambda m: self._update_import_option(m))
                    self._models["import_as_reference"] = True
                self._add_as_reference_frame = ui.VStack()
                with self._add_as_reference_frame:
                    ui.Label("USD Output")
                    self._models["dst_path"] = string_filed_builder(
                        tooltip="USD file to store instanceable meshes in",
                        default_val="Same as Imported Model(Default)",
                        folder_dialog_title="Select Output File",
                        folder_button_title="Select File",
                        read_only=True,
                    )
                self._add_to_stage_frame = ui.VStack()
                with self._add_to_stage_frame:
                    # TODO: when change this to False, will raise a 'not found default prim' error in _load_robot
                    checkbox_builder(
                        "Set as Default Prim",
                        tooltip="If true, makes imported robot the default prim for the stage",
                        default_val=self._config.make_default_prim,
                        on_clicked_fn=lambda m, config=self._config: config.set_make_default_prim(m),
                    )

                    self._models["clean_stage"] = checkbox_builder(
                        "Clear Stage on Import",
                        tooltip="Check this box to load URDF on a clean stage",
                        default_val=False,
                    )
                self._add_to_stage_frame.visible = False

        option_frame("Model", build_model_content)

    def _build_links_frame(self):
        def build_links_content():
            with ui.VStack(spacing=4):
                with ui.HStack(height=24):
                    ui.Spacer(width=0)
                    self._base_collection = ui.RadioCollection()
                    with ui.VStack(width=0):
                        ui.Spacer()
                        ui.RadioButton(width=20, height=20, radio_collection=self._base_collection)
                        ui.Spacer()
                    ui.Spacer(width=4)
                    ui.Label(
                        "Moveable Base",
                        width=90,
                        mouse_pressed_fn=lambda x, y, m, w: self._base_collection.model.set_value(0),
                    )
                    ui.Spacer(width=10)
                    with ui.VStack(width=0):
                        ui.Spacer()
                        ui.RadioButton(width=20, height=20, radio_collection=self._base_collection)
                        ui.Spacer()
                    ui.Spacer(width=4)
                    ui.Label(
                        "Static Base", mouse_pressed_fn=lambda x, y, m, w: self._base_collection.model.set_value(1)
                    )
                    index = 1 if self._config.fix_base else 0
                    self._base_collection.model.set_value(index)
                    self._base_collection.model.add_value_changed_fn(lambda m: self._update_fix_base(m))
                self._models["density"] = float_field_builder(
                    "Default Density",
                    default_val=self._config.density,
                    tooltip="[kg/stage_units^3] If a link doesn't have mass, use this density as backup, A density of 0.0 results in the physics engine automatically computing a default density",
                )
                self._models["density"].add_value_changed_fn(
                    lambda m, config=self._config: config.set_density(m.get_value_as_float())
                )

        option_frame("Links", build_links_content)

    def _parse_mimic(self, value):
        self._config.set_parse_mimic(value)
        if self._joint_widget:
            self._joint_widget.update_mimic()

    def _build_joint_drive_frame(self):
        def build_joint_drive_content():
            with ui.VStack(spacing=4):
                checkbox_builder(
                    "Ignore Mimic",
                    tooltip="If false, creates a PhysX Mimic Joint to enforce the mimic joint behavior. Otherwise, the joint is treated as a regular joint.",
                    default_val=not self._config.parse_mimic,
                    on_clicked_fn=lambda m,: self._parse_mimic(not m),
                )
                # checkbox_builder(
                #     "bulk edit",
                #     tooltip="If true, it will edit all the selected rows in the same time",
                #     default_val=False,
                #     on_clicked_fn=lambda m: self._set_bulk_edit(m),
                # )
                with ui.VStack(
                    height=24,
                    spacing=4,
                    tooltip="Joint Configuration: Changes the method to tune the joint drive gains\n\nStiffness: Edit the joint drive stiffness and damping directly.\nNatural Frequency: Computes the Joint drive stiffness and Damping ratio \nbased on the desired natural frequency using the formula: \n\nK = m * f^2, D = 2 * r * f * m \n\nwhere f is the natural frequency, r is the damping ratio, \nand total equivalent inertia at the joint.\n\nThe damping ratio is such that \nr = 1.0 is a critically damped system, \nr < 1.0 is underdamped, and \nr > 1.0 is overdamped",
                    tooltip_offset_y=50,
                ):
                    ui.Label("Joint Configuration", name="header", width=120)
                    with ui.HStack():
                        ui.Spacer(width=10)
                        self._edit_mode_collection = ui.RadioCollection()
                        with ui.HStack(width=0):
                            with ui.HStack(width=0):
                                with ui.VStack(width=0):
                                    ui.Spacer()
                                    ui.RadioButton(width=20, height=20, radio_collection=self._edit_mode_collection)
                                    ui.Spacer()
                                ui.Spacer(width=4)
                                ui.Label(
                                    "Stiffness",
                                    width=0,
                                    mouse_pressed_fn=lambda x, y, m, w: self._edit_mode_collection.model.set_value(0),
                                )
                            ui.Spacer(width=10)
                            with ui.HStack(width=0):
                                with ui.VStack(width=0):
                                    ui.Spacer()
                                    ui.RadioButton(width=20, height=20, radio_collection=self._edit_mode_collection)
                                    ui.Spacer()
                                ui.Spacer(width=4)
                                ui.Label(
                                    "Natural Frequency",
                                    mouse_pressed_fn=lambda x, y, m, w: self._edit_mode_collection.model.set_value(1),
                                )
                        self._edit_mode_collection.model.set_value(0)
                        self._edit_mode_collection.model.add_value_changed_fn(lambda m: self._switch_mode(m))
                with ui.VStack(
                    height=24,
                    spacing=4,
                    tooltip="Drive Type: Changes the method to control the joint drive\n\nAcceleration: The joint drive normalizes the inertia before applying the joint effort so it's invariant to \ninertia and mass changes (equivalent to ideal damped oscilator).\n\nForce: Applies effort through forces, so is subject to variations on the body inertia",
                    tooltip_offset_y=50,
                ):
                    ui.Label(
                        "Drive Type",
                        name="header",
                        width=120,
                    )
                    with ui.HStack():
                        ui.Spacer(width=10)
                        self._drive_type_collection = ui.RadioCollection()
                        with ui.HStack(width=0):
                            with ui.HStack(width=0):
                                with ui.VStack(width=0):
                                    ui.Spacer()
                                    ui.RadioButton(width=20, height=20, radio_collection=self._drive_type_collection)
                                    ui.Spacer()
                                ui.Label(
                                    "Acceleration",
                                    width=0,
                                    mouse_pressed_fn=lambda x, y, m, w: self._drive_type_collection.model.set_value(0),
                                )
                            ui.Spacer(width=10)
                            with ui.HStack(width=0):
                                with ui.VStack(width=0):
                                    ui.Spacer()
                                    ui.RadioButton(width=20, height=20, radio_collection=self._drive_type_collection)
                                    ui.Spacer()
                                ui.Label(
                                    "Force",
                                    mouse_pressed_fn=lambda x, y, m, w: self._drive_type_collection.model.set_value(1),
                                )
                        self._drive_type_collection.model.set_value(0)
                        self._drive_type_collection.model.add_value_changed_fn(lambda m: self._switch_drive_type(m))
                # checkbox_builder(
                #     "switch mode",
                #     tooltip="#TODO: add tooltips for switch mode",
                #     default_val=False,
                #     on_clicked_fn=lambda m: self._switch_mode(m),
                # )

                self._joint_widget_container = ui.VStack(height=0)

        option_frame("Joints & Drives", build_joint_drive_content)

    def build_joint_widget(self, urdf_robot, joints_values, on_joint_changed):
        if self._joint_widget_container:
            self._joint_widget_container.clear()
            with self._joint_widget_container:
                self._joint_widget = UrdfJointWidget(self._config, urdf_robot, joints_values, on_joint_changed)

                self._edit_mode_collection.model.set_value(1)
                self._drive_type_collection.model.set_value(1)

    def _build_colliders_frame(self):
        def build_colliders_content():
            with ui.VStack(spacing=4):
                checkbox_builder(
                    "Collision From Visuals",
                    default_val=self._config.collision_from_visuals,
                    tooltip="Creates collision geometry from visual geometry.",
                    on_clicked_fn=lambda m, config=self._config: config.set_collision_from_visuals(m),
                )
                with ui.VStack(height=24, spacing=4):
                    ui.Label("Collider Type", name="header", width=0)
                    with ui.HStack():
                        ui.Spacer(width=10)
                        self._collider_type_collection = ui.RadioCollection()
                        with ui.HStack(width=0):
                            with ui.HStack(width=0):
                                with ui.VStack(width=0):
                                    ui.Spacer()
                                    ui.RadioButton(width=20, height=20, radio_collection=self._collider_type_collection)
                                    ui.Spacer()
                                ui.Label(
                                    "Convex Hull",
                                    width=0,
                                    mouse_pressed_fn=lambda x, y, m, w: self._collider_type_collection.model.set_value(
                                        0
                                    ),
                                )
                            ui.Spacer(width=10)
                            with ui.HStack(width=0):
                                with ui.VStack(width=0):
                                    ui.Spacer()
                                    ui.RadioButton(width=20, height=20, radio_collection=self._collider_type_collection)
                                    ui.Spacer()
                                ui.Label(
                                    "Convex Decomposition",
                                    mouse_pressed_fn=lambda x, y, m, w: self._collider_type_collection.model.set_value(
                                        1
                                    ),
                                )
                        index = 1 if self._config.convex_decomp else 0
                        self._collider_type_collection.model.set_value(index)
                        self._collider_type_collection.model.add_value_changed_fn(
                            lambda m: self._update_convex_decomp(m)
                        )
                checkbox_builder(
                    "Allow Self-Collision",
                    tooltip="Enables self collision between adjacent links.",
                    default_val=self._config.self_collision,
                    on_clicked_fn=lambda m, config=self._config: config.set_self_collision(m),
                )
                checkbox_builder(
                    label="Replace Cylinders with Capsules",
                    tooltip="Replace Cylinder collision bodies by capsules.",
                    default_val=self._config.replace_cylinders_with_capsules,
                    on_clicked_fn=lambda m, config=self._config: config.set_replace_cylinders_with_capsules(m),
                )

        option_frame("Colliders", build_colliders_content)

    def _update_fix_base(self, model):
        value = bool(model.get_value_as_int() == 1)
        if self._config:
            self._config.set_fix_base(value)

    def _update_convex_decomp(self, model):
        value = bool(model.get_value_as_int() == 1)
        if self._config:
            self._config.set_convex_decomp(value)

    def _update_import_option(self, model):
        value = bool(model.get_value_as_int() == 1)
        self._add_as_reference_frame.visible = value
        self._add_to_stage_frame.visible = not value
        self._models["import_as_reference"] = value

    def _set_bulk_edit(self, enable):
        if self._joint_widget:
            self._joint_widget.set_bulk_edit(enable)

    def _switch_mode(self, switch):
        if self._joint_widget:
            self._joint_widget.switch_mode(JointSettingMode(switch.get_value_as_int()))

    def _switch_drive_type(self, switch):
        if self._joint_widget:
            self._joint_widget.switch_drive_type(switch.get_value_as_int())
