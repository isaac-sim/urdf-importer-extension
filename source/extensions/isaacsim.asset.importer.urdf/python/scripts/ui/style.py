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

import pathlib

import carb.settings
import omni
import omni.kit.app
import omni.ui as ui
from omni.kit.window.extensions.common import get_icons_path
from omni.ui import color as cl

# Pilaged from omni.kit.widnow.property style.py

LABEL_WIDTH = 120
BUTTON_WIDTH = 120
HORIZONTAL_SPACING = 4
VERTICAL_SPACING = 5
COLOR_X = 0xFF5555AA
COLOR_Y = 0xFF76A371
COLOR_Z = 0xFFA07D4F
COLOR_W = 0xFFAA5555


def get_style():
    extension_manager = omni.kit.app.get_app().get_extension_manager()
    ext_id = extension_manager.get_enabled_extension_id("isaacsim.asset.importer.urdf")

    ext_root = extension_manager.get_extension_path(ext_id) if ext_id else None

    icons_path = get_icons_path()
    custom_icons_path = f"{ext_root}/icons" if ext_root else None

    KIT_GREEN = 0xFF8A8777
    KIT_GREEN_CHECKBOX = 0xFF9A9A9A
    BORDER_RADIUS = 1.5
    FONT_SIZE = 14.0
    TOOLTIP_STYLE = (
        {
            "background_color": 0xFFD1F7FF,
            "color": 0xFF333333,
            "margin_width": 0,
            "margin_height": 0,
            "padding": 0,
            "border_width": 0,
            "border_radius": 1.5,
            "border_color": 0x0,
        },
    )

    style_settings = carb.settings.get_settings().get("/persistent/app/window/uiStyle")
    if not style_settings:
        style_settings = "NvidiaDark"

    if style_settings == "NvidiaLight":
        WINDOW_BACKGROUND_COLOR = 0xFF444444
        BUTTON_BACKGROUND_COLOR = 0xFF545454
        BUTTON_BACKGROUND_HOVERED_COLOR = 0xFF9E9E9E
        BUTTON_BACKGROUND_PRESSED_COLOR = 0xC22A8778
        BUTTON_LABEL_DISABLED_COLOR = 0xFF606060

        FRAME_TEXT_COLOR = 0xFF545454
        FIELD_BACKGROUND = 0xFF545454
        FIELD_SECONDARY = 0xFFABABAB
        FIELD_TEXT_COLOR = 0xFFD6D6D6
        FIELD_TEXT_COLOR_READ_ONLY = 0xFF9C9C9C
        FIELD_TEXT_COLOR_HIDDEN = 0x01000000
        COLLAPSABLEFRAME_BORDER_COLOR = 0x0
        COLLAPSABLEFRAME_BACKGROUND_COLOR = 0x7FD6D6D6
        COLLAPSABLEFRAME_TEXT_COLOR = 0xFF545454

        COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR = 0xFFC9C9C9
        COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR = 0xFFD6D6D6
        COLLAPSABLEFRAME_HOVERED_BACKGROUND_COLOR = 0xFFCCCFBF
        COLLAPSABLEFRAME_PRESSED_BACKGROUND_COLOR = 0xFF2E2E2B
        COLLAPSABLEFRAME_HOVERED_SECONDARY_COLOR = 0xFFD6D6D6
        COLLAPSABLEFRAME_PRESSED_SECONDARY_COLOR = 0xFFE6E6E6
        LABEL_VECTORLABEL_COLOR = 0xFFDDDDDD
        LABEL_MIXED_COLOR = 0xFFD6D6D6
        LIGHT_FONT_SIZE = 14.0
        LIGHT_BORDER_RADIUS = 3

        style = {
            "Window": {"background_color": WINDOW_BACKGROUND_COLOR},
            "Button": {"background_color": BUTTON_BACKGROUND_COLOR, "margin": 0, "padding": 3, "border_radius": 2},
            "Button:hovered": {"background_color": BUTTON_BACKGROUND_HOVERED_COLOR},
            "Button:pressed": {"background_color": BUTTON_BACKGROUND_PRESSED_COLOR},
            "Button.Label:disabled": {"color": 0xFFD6D6D6},
            "Button.Label": {"color": 0xFFD6D6D6},
            "Button.Image::accept": {"image_url": f"{custom_icons_path}/check_square.svg", "color": 0xFF535354},
            "Button.Image::autotune": {"image_url": f"{custom_icons_path}/config.svg", "color": 0xFF535354},
            "Button.Image::cancel": {"image_url": f"{custom_icons_path}/times_circle.svg", "color": 0xFF535354},
            "Field::models": {
                "background_color": FIELD_BACKGROUND,
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": LIGHT_BORDER_RADIUS,
                "secondary_color": FIELD_SECONDARY,
            },
            "Field::models_mixed": {
                "background_color": FIELD_BACKGROUND,
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": LIGHT_BORDER_RADIUS,
            },
            "Field::models_readonly": {
                "background_color": FIELD_BACKGROUND,
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR_READ_ONLY,
                "border_radius": LIGHT_BORDER_RADIUS,
                "secondary_color": FIELD_SECONDARY,
            },
            "Field::models_readonly_mixed": {
                "background_color": FIELD_BACKGROUND,
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": LIGHT_BORDER_RADIUS,
            },
            "Field::models:pressed": {"background_color": 0xFFCECECE},
            "Field": {"background_color": 0xFF535354, "color": 0xFFCCCCCC},
            "Label": {"font_size": 12, "color": FRAME_TEXT_COLOR},
            "Label::label:disabled": {"color": BUTTON_LABEL_DISABLED_COLOR},
            "Label::label": {
                "font_size": LIGHT_FONT_SIZE,
                "background_color": FIELD_BACKGROUND,
                "color": FRAME_TEXT_COLOR,
            },
            "Label::title": {
                "font_size": LIGHT_FONT_SIZE,
                "background_color": FIELD_BACKGROUND,
                "color": FRAME_TEXT_COLOR,
            },
            "Label::mixed_overlay": {
                "font_size": LIGHT_FONT_SIZE,
                "background_color": FIELD_BACKGROUND,
                "color": FRAME_TEXT_COLOR,
            },
            "Label::mixed_overlay_normal": {
                "font_size": LIGHT_FONT_SIZE,
                "background_color": FIELD_BACKGROUND,
                "color": FRAME_TEXT_COLOR,
            },
            "ComboBox::choices": {
                "font_size": 12,
                "color": 0xFFD6D6D6,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": FIELD_BACKGROUND,
                "border_radius": LIGHT_BORDER_RADIUS * 2,
            },
            "ComboBox::xform_op": {
                "font_size": 10,
                "color": 0xFF333333,
                "background_color": 0xFF9C9C9C,
                "secondary_color": 0x0,
                "selected_color": 0xFFACACAF,
                "border_radius": LIGHT_BORDER_RADIUS * 2,
            },
            "ComboBox::xform_op:hovered": {"background_color": 0x0},
            "ComboBox::xform_op:selected": {"background_color": 0xFF545454},
            "ComboBox": {
                "font_size": 10,
                "color": 0xFFE6E6E6,
                "background_color": 0xFF545454,
                "secondary_color": 0xFF545454,
                "selected_color": 0xFFACACAF,
                "border_radius": LIGHT_BORDER_RADIUS * 2,
            },
            # "ComboBox": {"background_color": 0xFF535354, "selected_color": 0xFFACACAF, "color": 0xFFD6D6D6},
            "ComboBox:hovered": {"background_color": 0xFF545454},
            "ComboBox:selected": {"background_color": 0xFF545454},
            "ComboBox::choices_mixed": {
                "font_size": LIGHT_FONT_SIZE,
                "color": 0xFFD6D6D6,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": FIELD_BACKGROUND,
                "secondary_selected_color": FIELD_TEXT_COLOR,
                "border_radius": LIGHT_BORDER_RADIUS * 2,
            },
            "ComboBox:hovered:choices": {"background_color": FIELD_BACKGROUND, "secondary_color": FIELD_BACKGROUND},
            "Slider": {
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": LIGHT_BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": WINDOW_BACKGROUND_COLOR,
                "draw_mode": ui.SliderDrawMode.FILLED,
            },
            "Slider::value": {
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR,  # COLLAPSABLEFRAME_TEXT_COLOR
                "border_radius": LIGHT_BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": KIT_GREEN,
            },
            "Slider::value_mixed": {
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": LIGHT_BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": KIT_GREEN,
            },
            "Slider::multivalue": {
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": LIGHT_BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": KIT_GREEN,
                "draw_mode": ui.SliderDrawMode.HANDLE,
            },
            "Slider::multivalue_mixed": {
                "font_size": LIGHT_FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": LIGHT_BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": KIT_GREEN,
                "draw_mode": ui.SliderDrawMode.HANDLE,
            },
            "Checkbox": {
                "margin": 0,
                "padding": 0,
                "radius": 0,
                "font_size": 10,
                "background_color": 0xFFA8A8A8,
                "background_color": 0xFFA8A8A8,
            },
            "CheckBox::greenCheck": {"font_size": 10, "background_color": KIT_GREEN, "color": 0xFF23211F},
            "CheckBox::greenCheck_mixed": {
                "font_size": 10,
                "background_color": KIT_GREEN,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": LIGHT_BORDER_RADIUS,
            },
            "CollapsableFrame": {
                "background_color": COLLAPSABLEFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_BACKGROUND_COLOR,
                "color": COLLAPSABLEFRAME_TEXT_COLOR,
                "border_radius": LIGHT_BORDER_RADIUS,
                "border_color": 0x0,
                "border_width": 1,
                "font_size": LIGHT_FONT_SIZE,
                "padding": 6,
                "Tooltip": TOOLTIP_STYLE,
            },
            "CollapsableFrame::groupFrame": {
                "background_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "border_radius": BORDER_RADIUS * 2,
                "padding": 6,
            },
            "CollapsableFrame::groupFrame:hovered": {
                "background_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
            },
            "CollapsableFrame::groupFrame:pressed": {
                "background_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
            },
            "CollapsableFrame::subFrame": {
                "background_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
            },
            "CollapsableFrame::subFrame:hovered": {
                "background_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_HOVERED_BACKGROUND_COLOR,
            },
            "CollapsableFrame::subFrame:pressed": {
                "background_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_PRESSED_BACKGROUND_COLOR,
            },
            "CollapsableFrame.Header": {
                "font_size": LIGHT_FONT_SIZE,
                "background_color": FRAME_TEXT_COLOR,
                "color": FRAME_TEXT_COLOR,
            },
            "CollapsableFrame:hovered": {"secondary_color": COLLAPSABLEFRAME_HOVERED_SECONDARY_COLOR},
            "CollapsableFrame:pressed": {"secondary_color": COLLAPSABLEFRAME_PRESSED_SECONDARY_COLOR},
            "ScrollingFrame": {"margin": 0, "padding": 3, "border_radius": LIGHT_BORDER_RADIUS},
            "TreeView": {
                "background_color": 0xFFE0E0E0,
                "background_selected_color": 0x109D905C,
                "secondary_color": 0xFFACACAC,
            },
            "TreeView.ScrollingFrame": {"background_color": 0xFFE0E0E0},
            "TreeView.Header": {"color": 0xFFCCCCCC},
            "TreeView.Header::background": {
                "background_color": 0xFF535354,
                "border_color": 0xFF707070,
                "border_width": 0.5,
            },
            "TreeView.Header::columnname": {"margin": 3},
            "TreeView.Image::object_icon_grey": {"color": 0x80FFFFFF},
            "TreeView.Item": {"color": 0xFF535354, "font_size": 16},
            "TreeView.Item::object_name": {"margin": 3},
            "TreeView.Item::object_name_grey": {"color": 0xFFACACAC},
            "TreeView.Item::object_name_missing": {"color": 0xFF6F72FF},
            "TreeView.Item:selected": {"color": 0xFF2A2825},
            "TreeView:selected": {"background_color": 0x409D905C},
            "Label::vector_label": {"font_size": 14, "color": LABEL_VECTORLABEL_COLOR},
            "Rectangle::vector_label": {"border_radius": BORDER_RADIUS * 2, "corner_flag": ui.CornerFlag.LEFT},
            "Rectangle::mixed_overlay": {
                "border_radius": LIGHT_BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "border_width": 3,
            },
            "Rectangle": {
                "border_radius": LIGHT_BORDER_RADIUS,
                "color": 0xFFC2C2C2,
                "background_color": 0xFFC2C2C2,
            },  # FIELD_BACKGROUND},
            "Rectangle::xform_op:hovered": {"background_color": 0x0},
            "Rectangle::xform_op": {"background_color": 0x0},
            # text remove
            "Button::remove": {"background_color": FIELD_BACKGROUND, "margin": 0},
            "Button::remove:hovered": {"background_color": FIELD_BACKGROUND},
            "Button::options": {"background_color": 0x0, "margin": 0},
            "Button.Image::options": {"image_url": f"{icons_path}/options.svg", "color": 0xFF989898},
            "Button.Image::options:hovered": {"color": 0xFFC2C2C2},
            "IconButton": {"margin": 0, "padding": 0, "background_color": 0x0},
            "IconButton:hovered": {"background_color": 0x0},
            "IconButton:checked": {"background_color": 0x0},
            "IconButton:pressed": {"background_color": 0x0},
            "IconButton.Image": {"color": 0xFFA8A8A8},
            "IconButton.Image:hovered": {"color": 0xFF929292},
            "IconButton.Image:pressed": {"color": 0xFFA4A4A4},
            "IconButton.Image:checked": {"color": 0xFFFFFFFF},
            "IconButton.Tooltip": {"color": 0xFF9E9E9E},
            "IconButton.Image::OpenFolder": {
                "image_url": f"{icons_path}/open-folder.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::OpenConfig": {
                "image_url": f"{icons_path}/open-config.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::OpenLink": {
                "image_url": "resources/glyphs/link.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::OpenDocs": {
                "image_url": "resources/glyphs/docs.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::CopyToClipboard": {
                "image_url": "resources/glyphs/copy.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::Export": {
                "image_url": f"{icons_path}/export.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::Sync": {
                "image_url": "resources/glyphs/sync.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::Upload": {
                "image_url": "resources/glyphs/upload.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::FolderPicker": {
                "image_url": "resources/glyphs/folder.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "ItemButton": {"padding": 2, "background_color": 0xFF444444, "border_radius": 4},
            "ItemButton.Image::add": {"image_url": f"{icons_path}/plus.svg", "color": 0xFF06C66B},
            "ItemButton.Image::remove": {"image_url": f"{icons_path}/trash.svg", "color": 0xFF1010C6},
            "ItemButton:hovered": {"background_color": 0xFF333333},
            "ItemButton:pressed": {"background_color": 0xFF222222},
            "Tooltip": TOOLTIP_STYLE,
        }
    else:
        LABEL_COLOR = 0xFF8F8E86
        FIELD_BACKGROUND = 0xFF23211F
        FIELD_TEXT_COLOR = 0xFFD5D5D5
        FIELD_TEXT_COLOR_READ_ONLY = 0xFF5C5C5C
        FIELD_TEXT_COLOR_HIDDEN = 0x01000000
        FRAME_TEXT_COLOR = 0xFFCCCCCC
        WINDOW_BACKGROUND_COLOR = 0xFF444444
        BUTTON_BACKGROUND_COLOR = 0xFF292929
        BUTTON_BACKGROUND_HOVERED_COLOR = 0xFF9E9E9E
        BUTTON_BACKGROUND_PRESSED_COLOR = 0xC22A8778
        BUTTON_LABEL_DISABLED_COLOR = 0xFF606060
        LABEL_LABEL_COLOR = 0xFF9E9E9E
        LABEL_TITLE_COLOR = 0xFFAAAAAA
        LABEL_MIXED_COLOR = 0xFFE6B067
        LABEL_VECTORLABEL_COLOR = 0xFFDDDDDD
        COLORWIDGET_BORDER_COLOR = 0xFF1E1E1E
        COMBOBOX_HOVERED_BACKGROUND_COLOR = 0xFF33312F
        COLLAPSABLEFRAME_BORDER_COLOR = 0x0
        COLLAPSABLEFRAME_BACKGROUND_COLOR = 0xFF343432
        COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR = 0xFF23211F
        COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR = 0xFF343432
        COLLAPSABLEFRAME_HOVERED_BACKGROUND_COLOR = 0xFF2E2E2B
        COLLAPSABLEFRAME_PRESSED_BACKGROUND_COLOR = 0xFF2E2E2B

        style = {
            "Window": {"background_color": WINDOW_BACKGROUND_COLOR},
            "Button": {"background_color": BUTTON_BACKGROUND_COLOR, "margin": 0, "padding": 3, "border_radius": 2},
            "Button:hovered": {"background_color": BUTTON_BACKGROUND_HOVERED_COLOR},
            "Button:pressed": {"background_color": BUTTON_BACKGROUND_PRESSED_COLOR},
            "Button.Label:disabled": {"color": BUTTON_LABEL_DISABLED_COLOR},
            "Button.Image::accept": {
                "image_url": f"{custom_icons_path}/check_square.svg",
                "color": 0x88FFFFDD,
                "background_color": 0x0,
                "margin": 0,
                "padding": 0,
            },
            "Button.Image::autotune": {
                "image_url": f"{custom_icons_path}/config.svg",
                "color": 0x88FFFFDD,
                "background_color": 0x0,
                "margin": 0,
                "padding": 0,
            },
            "Button.Image::cancel": {
                "image_url": f"{custom_icons_path}/times_circle.svg",
                "color": 0x88FFFFDD,
                "background_color": 0x0,
                "margin": 0,
                "padding": 0,
            },
            "StringField": {
                "background_color": FIELD_BACKGROUND,
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
            },
            "Field::models": {
                "background_color": FIELD_BACKGROUND,
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
            },
            "Field::models_mixed": {
                "background_color": FIELD_BACKGROUND,
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": BORDER_RADIUS,
            },
            "Field::models_readonly": {
                "background_color": FIELD_BACKGROUND,
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR_READ_ONLY,
                "border_radius": BORDER_RADIUS,
            },
            "Field::models_readonly_mixed": {
                "background_color": FIELD_BACKGROUND,
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": BORDER_RADIUS,
            },
            "Label": {"font_size": FONT_SIZE, "color": LABEL_COLOR},
            "Label::label": {"font_size": FONT_SIZE, "color": LABEL_LABEL_COLOR},
            "Label::label:disabled": {"color": BUTTON_LABEL_DISABLED_COLOR},
            "Label::title": {"font_size": FONT_SIZE, "color": LABEL_TITLE_COLOR},
            "Label::mixed_overlay": {"font_size": FONT_SIZE, "color": LABEL_MIXED_COLOR},
            "Label::mixed_overlay_normal": {"font_size": FONT_SIZE, "color": FIELD_TEXT_COLOR},
            "Label::path_label": {"font_size": FONT_SIZE, "color": LABEL_LABEL_COLOR},
            "Label::stage_label": {"font_size": FONT_SIZE, "color": LABEL_LABEL_COLOR},
            "ComboBox::choices": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": FIELD_BACKGROUND,
                "secondary_selected_color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
            },
            "ComboBox::choices_mixed": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": FIELD_BACKGROUND,
                "secondary_selected_color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
            },
            "ComboBox:hovered:choices": {
                "background_color": COMBOBOX_HOVERED_BACKGROUND_COLOR,
                "secondary_color": COMBOBOX_HOVERED_BACKGROUND_COLOR,
            },
            "Slider": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": WINDOW_BACKGROUND_COLOR,
                "draw_mode": ui.SliderDrawMode.FILLED,
            },
            "Slider::value": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": WINDOW_BACKGROUND_COLOR,
            },
            "Slider::value_mixed": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": WINDOW_BACKGROUND_COLOR,
            },
            "Slider::multivalue": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": WINDOW_BACKGROUND_COLOR,
                "draw_mode": ui.SliderDrawMode.HANDLE,
            },
            "Slider::multivalue_mixed": {
                "font_size": FONT_SIZE,
                "color": FIELD_TEXT_COLOR,
                "border_radius": BORDER_RADIUS,
                "background_color": FIELD_BACKGROUND,
                "secondary_color": WINDOW_BACKGROUND_COLOR,
                "draw_mode": ui.SliderDrawMode.HANDLE,
            },
            "CheckBox::greenCheck": {
                "font_size": 12,
                "background_color": KIT_GREEN_CHECKBOX,
                "color": FIELD_BACKGROUND,
                "border_radius": BORDER_RADIUS,
            },
            "CheckBox::greenCheck_mixed": {
                "font_size": 12,
                "background_color": KIT_GREEN_CHECKBOX,
                "color": FIELD_TEXT_COLOR_HIDDEN,
                "border_radius": BORDER_RADIUS,
            },
            "CollapsableFrame": {
                "background_color": COLLAPSABLEFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_BACKGROUND_COLOR,
                "border_radius": BORDER_RADIUS * 2,
                "border_color": COLLAPSABLEFRAME_BORDER_COLOR,
                "border_width": 1,
                "padding": 6,
            },
            "CollapsableFrame::groupFrame": {
                "background_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "border_radius": BORDER_RADIUS * 2,
                "padding": 6,
            },
            "CollapsableFrame::groupFrame:hovered": {
                "background_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
            },
            "CollapsableFrame::groupFrame:pressed": {
                "background_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_GROUPFRAME_BACKGROUND_COLOR,
            },
            "CollapsableFrame::subFrame": {
                "background_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
            },
            "CollapsableFrame::subFrame:hovered": {
                "background_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_HOVERED_BACKGROUND_COLOR,
            },
            "CollapsableFrame::subFrame:pressed": {
                "background_color": COLLAPSABLEFRAME_SUBFRAME_BACKGROUND_COLOR,
                "secondary_color": COLLAPSABLEFRAME_PRESSED_BACKGROUND_COLOR,
            },
            "CollapsableFrame.Header": {
                "font_size": FONT_SIZE,
                "background_color": FRAME_TEXT_COLOR,
                "color": FRAME_TEXT_COLOR,
            },
            "CollapsableFrame:hovered": {"secondary_color": COLLAPSABLEFRAME_HOVERED_BACKGROUND_COLOR},
            "CollapsableFrame:pressed": {"secondary_color": COLLAPSABLEFRAME_PRESSED_BACKGROUND_COLOR},
            "ScrollingFrame": {"margin": 0, "padding": 3, "border_radius": BORDER_RADIUS},
            "TreeView": {
                "background_color": 0xFF23211F,
                "background_selected_color": 0x664F4D43,
                "secondary_color": 0xFF403B3B,
            },
            "TreeView.ScrollingFrame": {"background_color": 0xFF23211F},
            "TreeView.Header": {"background_color": 0xFF343432, "color": 0xFFCCCCCC, "font_size": 12},
            "TreeView.Image::object_icon_grey": {"color": 0x80FFFFFF},
            "TreeView.Image:disabled": {"color": 0x60FFFFFF},
            "TreeView.Item": {"color": 0xFF8A8777},
            "TreeView.Item:disabled": {"color": 0x608A8777},
            "TreeView.Item::object_name_grey": {"color": 0xFF4D4B42},
            "TreeView.Item::object_name_missing": {"color": 0xFF6F72FF},
            "TreeView.Item:selected": {"color": 0xFF23211F},
            "TreeView:selected": {"background_color": 0xFF8A8777},
            "ColorWidget": {
                "border_radius": BORDER_RADIUS,
                "border_color": COLORWIDGET_BORDER_COLOR,
                "border_width": 0.5,
            },
            "Label::vector_label": {"font_size": 16, "color": LABEL_VECTORLABEL_COLOR},
            "PlotLabel::X": {"color": 0xFF1515EA, "background_color": 0x0},
            "PlotLabel::Y": {"color": 0xFF5FC054, "background_color": 0x0},
            "PlotLabel::Z": {"color": 0xFFC5822A, "background_color": 0x0},
            "PlotLabel::W": {"color": 0xFFAA5555, "background_color": 0x0},
            "Rectangle::vector_label": {"border_radius": BORDER_RADIUS * 2, "corner_flag": ui.CornerFlag.LEFT},
            "Rectangle::mixed_overlay": {
                "border_radius": BORDER_RADIUS,
                "background_color": LABEL_MIXED_COLOR,
                "border_width": 3,
            },
            "Rectangle": {
                "border_radius": BORDER_RADIUS,
                "background_color": FIELD_TEXT_COLOR_READ_ONLY,
            },  # FIELD_BACKGROUND},
            "Rectangle::xform_op:hovered": {"background_color": 0xFF444444},
            "Rectangle::xform_op": {"background_color": 0xFF333333},
            # text remove
            "Button::remove": {"background_color": FIELD_BACKGROUND, "margin": 0},
            "Button::remove:hovered": {"background_color": FIELD_BACKGROUND},
            "Button::options": {"background_color": 0x0, "margin": 0},
            "Button.Image::options": {"image_url": f"{icons_path}/options.svg", "color": 0xFF989898},
            "Button.Image::options:hovered": {"color": 0xFFC2C2C2},
            "IconButton": {"margin": 0, "padding": 0, "background_color": 0x0},
            "IconButton:hovered": {"background_color": 0x0},
            "IconButton:checked": {"background_color": 0x0},
            "IconButton:pressed": {"background_color": 0x0},
            "IconButton.Image": {"color": 0xFFA8A8A8},
            "IconButton.Image:hovered": {"color": 0xFFC2C2C2},
            "IconButton.Image:pressed": {"color": 0xFFA4A4A4},
            "IconButton.Image:checked": {"color": 0xFFFFFFFF},
            "IconButton.Tooltip": {"color": 0xFF9E9E9E},
            "IconButton.Image::OpenFolder": {
                "image_url": f"{icons_path}/open-folder.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::OpenConfig": {
                "tooltip": TOOLTIP_STYLE,
                "image_url": f"{icons_path}/open-config.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::OpenLink": {
                "image_url": "resources/glyphs/link.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::OpenDocs": {
                "image_url": "resources/glyphs/docs.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
                "tooltip": TOOLTIP_STYLE,
            },
            "IconButton.Image::CopyToClipboard": {
                "image_url": "resources/glyphs/copy.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::Export": {
                "image_url": f"{icons_path}/export.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::Sync": {
                "image_url": "resources/glyphs/sync.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::Upload": {
                "image_url": "resources/glyphs/upload.svg",
                "background_color": 0x0,
                "color": 0xFFA8A8A8,
            },
            "IconButton.Image::FolderPicker": {
                "image_url": "resources/glyphs/folder.svg",
                "background_color": 0x0,
                "color": 0xFF929292,
            },
            "ItemButton": {"padding": 2, "background_color": 0xFF444444, "border_radius": 4},
            "ItemButton.Image::add": {"image_url": f"{icons_path}/plus.svg", "color": 0xFF06C66B},
            "ItemButton.Image::remove": {"image_url": f"{icons_path}/trash.svg", "color": 0xFF1010C6},
            "ItemButton:hovered": {"background_color": 0xFF333333},
            "ItemButton:pressed": {"background_color": 0xFF222222},
            "Tooltip": TOOLTIP_STYLE,
        }

    return style


## -------------------------------new options start--------------------------------

EXTENSION_FOLDER_PATH = pathlib.Path(
    omni.kit.app.get_app().get_extension_manager().get_extension_path_by_module(__name__)
)

## colors
BUTTON_BG_COLOR = 0xFF24211F
FRAME_BG_COLOR = 0xFF343433
FRAME_HEAD_COLOR = 0xFF8F8F8F
STRING_FIELD_LABEL_COLOR = 0xFF8F8F8F
LABEL_COLOR = 0xFFD8D8D8
LABEL_TITLE_COLOR = 0xFFCCCCCC
DISABLED_LABEL_COLOR = 0xFF6E6E6E
UNIT_COLOR = 0xFF6E6E6E
LINE_COLOR = 0xFF8F8F8F
TRIANGLE_COLOR = 0xFF8F8F8F
TREEVIEW_BG_COLOR = 0xFF23211F
TREEVIEW_SELECTED_COLOR = 0xFF4B4A42
TREEVIEW_ITEM_COLOR = 0xFF343432
TREEVIEW_HEADER_BG_COLOR = 0xFF2D2D2D
TREEVIEW_ITEM_FONT = 14
HEADER_FONT_SIZE = 16
FONT_SIZE = 14


def get_option_style():
    style = {
        "Button::reset": {"background_color": 0x0, "border_radius": 1},
        "Button::reset:disabled": {"background_color": 0x0, "color": 0x0, "border_radius": 1},
        "Button::reset:hovered": {"background_color": 0x0, "border_radius": 1},
        "Button::reset:pressed": {"background_color": 0x0, "border_radius": 1},
        "CheckBox": {"border_radius": 2, "font_size": 12},
        "CollapsableFrame": {"background_color": FRAME_BG_COLOR, "secondary_color": FRAME_BG_COLOR},
        "CollapsableFrame:hovered": {"background_color": FRAME_BG_COLOR, "secondary_color": FRAME_BG_COLOR},
        "ComboBox::treeview_item": {
            "color": LABEL_COLOR,
            "background_selected_color": 0x00,
            "background_color": TREEVIEW_BG_COLOR,
            "secondary_color": TREEVIEW_BG_COLOR,
            "border_radius": 0,
            "padding": 0,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "ComboBox::treeview_item:selected": {
            "color": LABEL_COLOR,
            "background_selected_color": 0x0,  # TREEVIEW_BG_COLOR,
            "background_color": TREEVIEW_BG_COLOR,
            "secondary_color": TREEVIEW_BG_COLOR,
            "border_radius": 0,
            "padding": 0,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "ComboBox::treeview_item:hovered": {
            "color": LABEL_COLOR,
            "background_selected_color": 0x0,  # TREEVIEW_BG_COLOR,
            "background_color": TREEVIEW_BG_COLOR,
            "secondary_color": TREEVIEW_BG_COLOR,
            "border_radius": 0,
            "padding": 0,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "ComboBox::treeview_item:disabled": {
            "color": DISABLED_LABEL_COLOR,
            "background_selected_color": 0x0,  # TREEVIEW_BG_COLOR,
            "background_color": TREEVIEW_BG_COLOR,
            "secondary_color": TREEVIEW_BG_COLOR,
            "border_radius": 0,
            "padding": 0,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Field::StringField": {
            "background_color": BUTTON_BG_COLOR,
            "color": STRING_FIELD_LABEL_COLOR,
            "font_size": FONT_SIZE,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_It.ttf",
        },
        "Field::FloatField": {
            "color": LABEL_COLOR,
            "font_size": FONT_SIZE,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Field::resetable": {
            "background_color": 0x00,
            "color": LABEL_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "padding": 4,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Field::resetable:disabled": {
            "background_color": 0x00,
            "color": DISABLED_LABEL_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "padding": 4,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Field::resetable:hovered": {
            "background_color": 0x00,
            "color": LABEL_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "padding": 4,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Field::resetable:pressed": {
            "background_color": 0x00,
            "color": LABEL_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "padding": 4,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Image::sort": {"image_url": f"{EXTENSION_FOLDER_PATH}/icons/sort_icon.svg", "margin": 4},
        "Line": {"color": LINE_COLOR},
        "Label": {
            "color": LABEL_COLOR,
            "font_size": FONT_SIZE,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Md.ttf",
        },
        "Label::header": {
            "color": FRAME_HEAD_COLOR,
            "font_size": FONT_SIZE,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Md.ttf",
        },
        "Label::collapsable_header": {
            "color": FRAME_HEAD_COLOR,
            "font_size": HEADER_FONT_SIZE,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Label::treeview_item": {
            "color": LABEL_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Label::index": {
            "color": LABEL_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "Label::density": {
            "color": UNIT_COLOR,
            "font_size": FONT_SIZE,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Lt.ttf",
        },
        "Label::exponent": {
            "color": UNIT_COLOR,
            "font_size": 8,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Lt.ttf",
        },
        "RadioButton": {"background_color": cl.transparent, "padding": 0},
        "RadioButton:checked": {"background_color": cl.transparent, "padding": 0},
        "RadioButton:hovered": {"background_color": cl.transparent, "padding": 0},
        "RadioButton.Image": {
            "image_url": f"{EXTENSION_FOLDER_PATH}/icons/radio_off.svg",
            "color": LABEL_COLOR,
        },
        "RadioButton.Image:hovered": {
            "image_url": f"{EXTENSION_FOLDER_PATH}/icons/radio_off.svg",
            "color": LABEL_COLOR,
        },
        "RadioButton.Image:checked": {"image_url": f"{EXTENSION_FOLDER_PATH}/icons/radio_on.svg", "color": LABEL_COLOR},
        "RadioButton:pressed": {"background_color": cl.transparent},
        "Triangle": {"background_color": TRIANGLE_COLOR, "color": TRIANGLE_COLOR},
        "Triangle::mask": {"color": LABEL_COLOR},
        "Rectangle::mask": {"background_color": TREEVIEW_BG_COLOR, "border_radius": 1},
        "Rectangle::reset_invalid": {"background_color": 0xFF505050, "border_radius": 1},
        "Rectangle::reset": {"background_color": 0xFFA07D4F, "border_radius": 1},
        "Rectangle::reset:disabled": {"background_color": 0x0, "border_radius": 1},
        "Rectangle::treeview_item": {
            "background_color": TREEVIEW_BG_COLOR,
            "border_width": 1,
            "border_color": 0xFF505050,
        },
        "Rectangle::treeview_first_item": {"background_color": 0x0, "border_width": 1, "border_color": 0xFF505050},
        "Rectangle::treeview_first_item:hovered": {
            "background_color": TREEVIEW_SELECTED_COLOR,
            "border_width": 1,
            "border_color": 0xFF505050,
        },
        "Rectangle::treeview_first_item:pressed": {
            "background_color": TREEVIEW_SELECTED_COLOR,
            "border_width": 1,
            "border_color": 0xFF505050,
        },
        "Rectangle::treeview_id": {"margin": 1, "background_color": TREEVIEW_ITEM_COLOR},
        "ScrollingFrame": {"background_color": FRAME_BG_COLOR},
        "ScrollingFrame::treeview": {"background_color": TREEVIEW_BG_COLOR},
        "TreeView": {
            "background_selected_color": TREEVIEW_SELECTED_COLOR,
            "background_color": TREEVIEW_BG_COLOR,
            "secondary_color": TREEVIEW_BG_COLOR,
        },  # the hover color of the TreeView selected item
        "TreeView.Header": {
            "background_color": TREEVIEW_BG_COLOR,
            "font_size": TREEVIEW_ITEM_FONT,
            "font": f"{EXTENSION_FOLDER_PATH}/data/fonts/NVIDIASans_Rg.ttf",
        },
        "TreeView.Header::background": {"margin": 1, "background_color": TREEVIEW_HEADER_BG_COLOR},
        "TreeView:selected": {
            "background_color": TREEVIEW_SELECTED_COLOR
        },  # selected margin color, set to scrollingFrame background color
    }
    return style
