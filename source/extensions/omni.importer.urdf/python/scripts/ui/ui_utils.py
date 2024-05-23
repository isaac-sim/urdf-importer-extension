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
import os
import subprocess
import sys
from cmath import inf

import carb
import carb.settings
import omni.appwindow
import omni.ext
import omni.ui as ui
from omni.kit.window.extensions import SimpleCheckBox
from omni.kit.window.filepicker import FilePickerDialog
from omni.kit.window.property.templates import LABEL_HEIGHT, LABEL_WIDTH

# from .callbacks import on_copy_to_clipboard, on_docs_link_clicked, on_open_folder_clicked, on_open_IDE_clicked
from .style import BUTTON_WIDTH, COLOR_W, COLOR_X, COLOR_Y, COLOR_Z, get_style

# Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto. Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#


def on_copy_to_clipboard(to_copy: str) -> None:
    """
    Copy text to system clipboard
    """
    try:
        import pyperclip
    except ImportError:
        carb.log_warn("Could not import pyperclip.")
        return
    try:
        pyperclip.copy(to_copy)
    except pyperclip.PyperclipException:
        carb.log_warn(pyperclip.EXCEPT_MSG)
        return


def on_open_IDE_clicked(ext_path: str, file_path: str) -> None:
    """Opens the current directory and file in VSCode"""
    if sys.platform == "win32":
        try:
            subprocess.Popen(["code", os.path.abspath(ext_path), os.path.abspath(file_path)], shell=True)
        except Exception:
            carb.log_warn(
                "Could not open in VSCode. See Troubleshooting help here: https://code.visualstudio.com/docs/editor/command-line#_common-questions"
            )
    else:
        try:
            cmd_string = "code " + ext_path + " " + file_path
            subprocess.run([cmd_string], shell=True, check=True)
            # os.system("code " + ext_path + " " + file_path)
        except Exception:
            carb.log_warn(
                "Could not open in VSCode. See Troubleshooting help here: https://code.visualstudio.com/docs/editor/command-line#_common-questions"
            )


def on_open_folder_clicked(file_path: str) -> None:
    """Opens the current directory in a File Browser"""
    if sys.platform == "win32":
        try:
            subprocess.Popen(["start", os.path.abspath(os.path.dirname(file_path))], shell=True)
        except OSError:
            carb.log_warn("Could not open file browser.")
    else:
        try:
            subprocess.run(["xdg-open", os.path.abspath(file_path.rpartition("/")[0])], check=True)
        except Exception:
            carb.log_warn("could not open file browser")


def on_docs_link_clicked(doc_link: str) -> None:
    """Opens an extension's documentation in a Web Browser"""
    import webbrowser

    try:
        webbrowser.open(doc_link, new=2)
    except Exception as e:
        carb.log_warn(f"Could not open browswer with url: {doc_link}, {e}")


def btn_builder(label="", type="button", text="button", tooltip="", on_clicked_fn=None):
    """Creates a stylized button.

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "button".
        text (str, optional): Text rendered on the button. Defaults to "button".
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (Callable, optional): Call-back function when clicked. Defaults to None.

    Returns:
        ui.Button: Button
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        btn = ui.Button(
            text.upper(),
            name="Button",
            width=BUTTON_WIDTH,
            clicked_fn=on_clicked_fn,
            style=get_style(),
            alignment=ui.Alignment.LEFT_CENTER,
        )
        ui.Spacer(width=5)
        add_line_rect_flourish(True)
        # ui.Spacer(width=ui.Fraction(1))
        # ui.Spacer(width=10)
        # with ui.Frame(width=0):
        #     with ui.VStack():
        #         with ui.Placer(offset_x=0, offset_y=7):
        #             ui.Rectangle(height=5, width=5, alignment=ui.Alignment.CENTER)
        # ui.Spacer(width=5)
    return btn


def state_btn_builder(
    label="", type="state_button", a_text="STATE A", b_text="STATE B", tooltip="", on_clicked_fn=None
):
    """Creates a State Change Button that changes text when pressed.

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "button".
        a_text (str, optional): Text rendered on the button for State A. Defaults to "STATE A".
        b_text (str, optional): Text rendered on the button for State B. Defaults to "STATE B".
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (Callable, optional): Call-back function when clicked. Defaults to None.
    """

    def toggle():
        if btn.text == a_text.upper():
            btn.text = b_text.upper()
            on_clicked_fn(True)
        else:
            btn.text = a_text.upper()
            on_clicked_fn(False)

    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        btn = ui.Button(
            a_text.upper(),
            name="Button",
            width=BUTTON_WIDTH,
            clicked_fn=toggle,
            style=get_style(),
            alignment=ui.Alignment.LEFT_CENTER,
        )
        ui.Spacer(width=5)
        # add_line_rect_flourish(False)
        ui.Spacer(width=ui.Fraction(1))
        ui.Spacer(width=10)
        with ui.Frame(width=0):
            with ui.VStack():
                with ui.Placer(offset_x=0, offset_y=7):
                    ui.Rectangle(height=5, width=5, alignment=ui.Alignment.CENTER)
        ui.Spacer(width=5)
    return btn


def cb_builder(label="", type="checkbox", default_val=False, tooltip="", on_clicked_fn=None):
    """Creates a Stylized Checkbox

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "checkbox".
        default_val (bool, optional): Checked is True, Unchecked is False. Defaults to False.
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (Callable, optional): Call-back function when clicked. Defaults to None.

    Returns:
        ui.SimpleBoolModel: model
    """

    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH - 12, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        model = ui.SimpleBoolModel()
        callable = on_clicked_fn
        if callable is None:
            callable = lambda x: None
        SimpleCheckBox(default_val, callable, model=model)

        add_line_rect_flourish()
        return model


def multi_btn_builder(
    label="", type="multi_button", count=2, text=["button", "button"], tooltip=["", "", ""], on_clicked_fn=[None, None]
):
    """Creates a Row of Stylized Buttons

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "multi_button".
        count (int, optional): Number of UI elements to create. Defaults to 2.
        text (list, optional): List of text rendered on the UI elements. Defaults to ["button", "button"].
        tooltip (list, optional): List of tooltips to display over the UI elements. Defaults to ["", "", ""].
        on_clicked_fn (list, optional): List of call-backs function when clicked. Defaults to [None, None].

    Returns:
        list(ui.Button): List of Buttons
    """
    btns = []
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[0]))
        for i in range(count):
            btn = ui.Button(
                text[i].upper(),
                name="Button",
                width=BUTTON_WIDTH,
                clicked_fn=on_clicked_fn[i],
                tooltip=format_tt(tooltip[i + 1]),
                style=get_style(),
                alignment=ui.Alignment.LEFT_CENTER,
            )
            btns.append(btn)
            if i < count:
                ui.Spacer(width=5)
        add_line_rect_flourish()
    return btns


def multi_cb_builder(
    label="",
    type="multi_checkbox",
    count=2,
    text=[" ", " "],
    default_val=[False, False],
    tooltip=["", "", ""],
    on_clicked_fn=[None, None],
):
    """Creates a Row of Stylized Checkboxes.

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "multi_checkbox".
        count (int, optional): Number of UI elements to create. Defaults to 2.
        text (list, optional): List of text rendered on the UI elements. Defaults to [" ", " "].
        default_val (list, optional): List of default values. Checked is True, Unchecked is False. Defaults to [False, False].
        tooltip (list, optional): List of tooltips to display over the UI elements. Defaults to ["", "", ""].
        on_clicked_fn (list, optional): List of call-backs function when clicked. Defaults to [None, None].

    Returns:
        list(ui.SimpleBoolModel): List of models
    """
    cbs = []
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH - 12, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[0]))
        for i in range(count):
            cb = ui.SimpleBoolModel(default_value=default_val[i])
            callable = on_clicked_fn[i]
            if callable is None:
                callable = lambda x: None
            SimpleCheckBox(default_val[i], callable, model=cb)
            ui.Label(
                text[i], width=BUTTON_WIDTH / 2, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[i + 1])
            )
            if i < count - 1:
                ui.Spacer(width=5)
            cbs.append(cb)
        add_line_rect_flourish()
    return cbs


def str_builder(
    label="",
    type="stringfield",
    default_val=" ",
    tooltip="",
    on_clicked_fn=None,
    use_folder_picker=False,
    read_only=False,
    item_filter_fn=None,
    bookmark_label=None,
    bookmark_path=None,
    folder_dialog_title="Select Output Folder",
    folder_button_title="Select Folder",
    style=get_style(),
):
    """Creates a Stylized Stringfield Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "stringfield".
        default_val (str, optional): Text to initialize in Stringfield. Defaults to " ".
        tooltip (str, optional): Tooltip to display over the UI elements. Defaults to "".
        use_folder_picker (bool, optional): Add a folder picker button to the right. Defaults to False.
        read_only (bool, optional): Prevents editing. Defaults to False.
        item_filter_fn (Callable, optional): filter function to pass to the FilePicker
        bookmark_label (str, optional): bookmark label to pass to the FilePicker
        bookmark_path (str, optional): bookmark path to pass to the FilePicker
    Returns:
        AbstractValueModel: model of Stringfield
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        str_field = ui.StringField(
            name="StringField",
            style=style,
            width=ui.Fraction(1),
            height=0,
            alignment=ui.Alignment.LEFT_CENTER,
            read_only=read_only,
        ).model
        str_field.set_value(default_val)

        if use_folder_picker:

            def update_field(filename, path):
                if filename == "":
                    val = path
                elif filename[0] != "/" and path[-1] != "/":
                    val = path + "/" + filename
                elif filename[0] == "/" and path[-1] == "/":
                    val = path + filename[1:]
                else:
                    val = path + filename
                str_field.set_value(val)

            add_folder_picker_icon(
                update_field,
                item_filter_fn,
                bookmark_label,
                bookmark_path,
                dialog_title=folder_dialog_title,
                button_title=folder_button_title,
            )
        else:
            add_line_rect_flourish(False)
        return str_field


def int_builder(label="", type="intfield", default_val=0, tooltip="", min=sys.maxsize * -1, max=sys.maxsize):
    """Creates a Stylized Intfield Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "intfield".
        default_val (int, optional): Default Value of UI element. Defaults to 0.
        tooltip (str, optional): Tooltip to display over the UI elements. Defaults to "".
        min (int, optional): Minimum limit for int field. Defaults to sys.maxsize * -1
        max (int, optional): Maximum limit for int field. Defaults to sys.maxsize * 1

    Returns:
        AbstractValueModel: model
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        int_field = ui.IntDrag(
            name="Field", height=LABEL_HEIGHT, min=min, max=max, alignment=ui.Alignment.LEFT_CENTER
        ).model
        int_field.set_value(default_val)
        add_line_rect_flourish(False)
    return int_field


def float_builder(label="", type="floatfield", default_val=0, tooltip="", min=-inf, max=inf, step=0.1, format="%.2f"):
    """Creates a Stylized Floatfield Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "floatfield".
        default_val (int, optional): Default Value of UI element. Defaults to 0.
        tooltip (str, optional): Tooltip to display over the UI elements. Defaults to "".

    Returns:
        AbstractValueModel: model
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        float_field = ui.FloatDrag(
            name="FloatField",
            width=ui.Fraction(1),
            height=0,
            alignment=ui.Alignment.LEFT_CENTER,
            min=min,
            max=max,
            step=step,
            format=format,
        ).model
        float_field.set_value(default_val)
        add_line_rect_flourish(False)
        return float_field


def combo_cb_str_builder(
    label="",
    type="checkbox_stringfield",
    default_val=[False, " "],
    tooltip="",
    on_clicked_fn=lambda x: None,
    use_folder_picker=False,
    read_only=False,
    folder_dialog_title="Select Output Folder",
    folder_button_title="Select Folder",
):
    """Creates a Stylized Checkbox + Stringfield Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "checkbox_stringfield".
        default_val (str, optional): Text to initialize in Stringfield. Defaults to [False, " "].
        tooltip (str, optional): Tooltip to display over the UI elements. Defaults to "".
        use_folder_picker (bool, optional): Add a folder picker button to the right. Defaults to False.
        read_only (bool, optional): Prevents editing. Defaults to False.

    Returns:
        Tuple(ui.SimpleBoolModel, AbstractValueModel): (cb_model, str_field_model)
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH - 12, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        cb = ui.SimpleBoolModel(default_value=default_val[0])
        SimpleCheckBox(default_val[0], on_clicked_fn, model=cb)
        str_field = ui.StringField(
            name="StringField", width=ui.Fraction(1), height=0, alignment=ui.Alignment.LEFT_CENTER, read_only=read_only
        ).model
        str_field.set_value(default_val[1])

        if use_folder_picker:

            def update_field(val):
                str_field.set_value(val)

            add_folder_picker_icon(update_field, dialog_title=folder_dialog_title, button_title=folder_button_title)
        else:
            add_line_rect_flourish(False)
        return cb, str_field


def dropdown_builder(
    label="", type="dropdown", default_val=0, items=["Option 1", "Option 2", "Option 3"], tooltip="", on_clicked_fn=None
):
    """Creates a Stylized Dropdown Combobox

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "dropdown".
        default_val (int, optional): Default index of dropdown items. Defaults to 0.
        items (list, optional): List of items for dropdown box. Defaults to ["Option 1", "Option 2", "Option 3"].
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (Callable, optional): Call-back function when clicked. Defaults to None.

    Returns:
        AbstractItemModel: model
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        combo_box = ui.ComboBox(
            default_val, *items, name="ComboBox", width=ui.Fraction(1), alignment=ui.Alignment.LEFT_CENTER
        ).model
        add_line_rect_flourish(False)

        def on_clicked_wrapper(model, val):
            on_clicked_fn(items[model.get_item_value_model().as_int])

        if on_clicked_fn is not None:
            combo_box.add_item_changed_fn(on_clicked_wrapper)

    return combo_box


def combo_intfield_slider_builder(
    label="", type="intfield_stringfield", default_val=0.5, min=0, max=1, step=0.01, tooltip=["", ""]
):
    """Creates a Stylized IntField + Stringfield Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "intfield_stringfield".
        default_val (float, optional): Default Value. Defaults to 0.5.
        min (int, optional): Minimum Value. Defaults to 0.
        max (int, optional): Maximum Value. Defaults to 1.
        step (float, optional): Step. Defaults to 0.01.
        tooltip (list, optional): List of tooltips. Defaults to ["", ""].

    Returns:
        Tuple(AbstractValueModel, IntSlider): (flt_field_model, flt_slider_model)
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[0]))
        ff = ui.IntDrag(
            name="Field", width=BUTTON_WIDTH / 2, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[1])
        ).model
        ff.set_value(default_val)
        ui.Spacer(width=5)
        fs = ui.IntSlider(
            width=ui.Fraction(1), alignment=ui.Alignment.LEFT_CENTER, min=min, max=max, step=step, model=ff
        )

        add_line_rect_flourish(False)
        return ff, fs


def combo_floatfield_slider_builder(
    label="", type="floatfield_stringfield", default_val=0.5, min=0, max=1, step=0.01, tooltip=["", ""]
):
    """Creates a Stylized FloatField + FloatSlider Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "floatfield_stringfield".
        default_val (float, optional): Default Value. Defaults to 0.5.
        min (int, optional): Minimum Value. Defaults to 0.
        max (int, optional): Maximum Value. Defaults to 1.
        step (float, optional): Step. Defaults to 0.01.
        tooltip (list, optional): List of tooltips. Defaults to ["", ""].

    Returns:
        Tuple(AbstractValueModel, IntSlider): (flt_field_model, flt_slider_model)
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[0]))
        ff = ui.FloatField(
            name="Field", width=BUTTON_WIDTH / 2, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip[1])
        ).model
        ff.set_value(default_val)
        ui.Spacer(width=5)
        fs = ui.FloatSlider(
            width=ui.Fraction(1), alignment=ui.Alignment.LEFT_CENTER, min=min, max=max, step=step, model=ff
        )

        add_line_rect_flourish(False)
        return ff, fs


def multi_dropdown_builder(
    label="",
    type="multi_dropdown",
    count=2,
    default_val=[0, 0],
    items=[["Option 1", "Option 2", "Option 3"], ["Option A", "Option B", "Option C"]],
    tooltip="",
    on_clicked_fn=[None, None],
):
    """Creates a Stylized Multi-Dropdown Combobox

    Returns:
        AbstractItemModel: model

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "multi_dropdown".
        count (int, optional): Number of UI elements. Defaults to 2.
        default_val (list(int), optional): List of default indices of dropdown items. Defaults to 0.. Defaults to [0, 0].
        items (list(list), optional): List of list of items for dropdown boxes. Defaults to [["Option 1", "Option 2", "Option 3"], ["Option A", "Option B", "Option C"]].
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (list(Callable), optional): List of call-back function when clicked. Defaults to [None, None].

    Returns:
        list(AbstractItemModel): list(models)
    """
    elems = []
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        for i in range(count):
            elem = ui.ComboBox(
                default_val[i], *items[i], name="ComboBox", width=ui.Fraction(1), alignment=ui.Alignment.LEFT_CENTER
            )

            def on_clicked_wrapper(model, val, index):
                on_clicked_fn[index](items[index][model.get_item_value_model().as_int])

            elem.model.add_item_changed_fn(lambda m, v, index=i: on_clicked_wrapper(m, v, index))
            elems.append(elem)
            if i < count - 1:
                ui.Spacer(width=5)
        add_line_rect_flourish(False)
        return elems


def combo_cb_dropdown_builder(
    label="",
    type="checkbox_dropdown",
    default_val=[False, 0],
    items=["Option 1", "Option 2", "Option 3"],
    tooltip="",
    on_clicked_fn=[lambda x: None, None],
):
    """Creates a Stylized Dropdown Combobox with an Enable Checkbox

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "checkbox_dropdown".
        default_val (list, optional): list(cb_default, dropdown_default). Defaults to [False, 0].
        items (list, optional): List of items for dropdown box. Defaults to ["Option 1", "Option 2", "Option 3"].
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (list, optional): List of callback functions. Defaults to [lambda x: None, None].

    Returns:
        Tuple(ui.SimpleBoolModel, ui.ComboBox): (cb_model, combobox)
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH - 12, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        cb = ui.SimpleBoolModel(default_value=default_val[0])
        SimpleCheckBox(default_val[0], on_clicked_fn[0], model=cb)
        combo_box = ui.ComboBox(
            default_val[1], *items, name="ComboBox", width=ui.Fraction(1), alignment=ui.Alignment.LEFT_CENTER
        )

        def on_clicked_wrapper(model, val):

            on_clicked_fn[1](items[model.get_item_value_model().as_int])

        combo_box.model.add_item_changed_fn(on_clicked_wrapper)

        add_line_rect_flourish(False)

        return cb, combo_box


def scrolling_frame_builder(label="", type="scrolling_frame", default_val="No Data", tooltip=""):
    """Creates a Labeled Scrolling Frame with CopyToClipboard button

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "scrolling_frame".
        default_val (str, optional): Default Text. Defaults to "No Data".
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".

    Returns:
        ui.Label: label
    """

    with ui.VStack(style=get_style(), spacing=5):
        with ui.HStack():
            ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))
            with ui.ScrollingFrame(
                height=LABEL_HEIGHT * 5,
                style_type_name_override="ScrollingFrame",
                alignment=ui.Alignment.LEFT_TOP,
                horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
                vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
            ):
                text = ui.Label(
                    default_val,
                    style_type_name_override="Label::label",
                    word_wrap=True,
                    alignment=ui.Alignment.LEFT_TOP,
                )
            with ui.Frame(width=0, tooltip="Copy To Clipboard"):
                ui.Button(
                    name="IconButton",
                    width=20,
                    height=20,
                    clicked_fn=lambda: on_copy_to_clipboard(to_copy=text.text),
                    style=get_style()["IconButton.Image::CopyToClipboard"],
                    alignment=ui.Alignment.RIGHT_TOP,
                )
    return text


def combo_cb_scrolling_frame_builder(
    label="", type="cb_scrolling_frame", default_val=[False, "No Data"], tooltip="", on_clicked_fn=lambda x: None
):
    """Creates a Labeled, Checkbox-enabled Scrolling Frame with CopyToClipboard button

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "cb_scrolling_frame".
        default_val (list, optional): List of Checkbox and Frame Defaults. Defaults to [False, "No Data"].
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".
        on_clicked_fn (Callable, optional): Callback function when clicked. Defaults to lambda x : None.

    Returns:
        list(SimpleBoolModel, ui.Label): (model, label)
    """

    with ui.VStack(style=get_style(), spacing=5):
        with ui.HStack():
            ui.Label(label, width=LABEL_WIDTH - 12, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))
            with ui.VStack(width=0):
                cb = ui.SimpleBoolModel(default_value=default_val[0])
                SimpleCheckBox(default_val[0], on_clicked_fn, model=cb)
                ui.Spacer(height=18 * 4)
            with ui.ScrollingFrame(
                height=18 * 5,
                style_type_name_override="ScrollingFrame",
                alignment=ui.Alignment.LEFT_TOP,
                horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
                vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
            ):
                text = ui.Label(
                    default_val[1],
                    style_type_name_override="Label::label",
                    word_wrap=True,
                    alignment=ui.Alignment.LEFT_TOP,
                )

            with ui.Frame(width=0, tooltip="Copy to Clipboard"):
                ui.Button(
                    name="IconButton",
                    width=20,
                    height=20,
                    clicked_fn=lambda: on_copy_to_clipboard(to_copy=text.text),
                    style=get_style()["IconButton.Image::CopyToClipboard"],
                    alignment=ui.Alignment.RIGHT_TOP,
                )
    return cb, text


def xyz_builder(
    label="",
    tooltip="",
    axis_count=3,
    default_val=[0.0, 0.0, 0.0, 0.0],
    min=float("-inf"),
    max=float("inf"),
    step=0.001,
    on_value_changed_fn=[None, None, None, None],
):
    """[summary]

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "".
        axis_count (int, optional): Number of Axes to Display. Max 4. Defaults to 3.
        default_val (list, optional): List of default values. Defaults to [0.0, 0.0, 0.0, 0.0].
        min (float, optional): Minimum Float Value. Defaults to float("-inf").
        max (float, optional): Maximum Float Value. Defaults to float("inf").
        step (float, optional): Step. Defaults to 0.001.
        on_value_changed_fn (list, optional): List of callback functions for each axes. Defaults to [None, None, None, None].

    Returns:
        list(AbstractValueModel): list(model)
    """

    # These styles & colors are taken from omni.kit.property.transform_builder.py _create_multi_float_drag_matrix_with_labels
    if axis_count <= 0 or axis_count > 4:
        import builtins

        carb.log_warn("Invalid axis_count: must be in range 1 to 4. Clamping to default range.")
        axis_count = builtins.max(builtins.min(axis_count, 4), 1)

    field_labels = [("X", COLOR_X), ("Y", COLOR_Y), ("Z", COLOR_Z), ("W", COLOR_W)]
    field_tooltips = ["X Value", "Y Value", "Z Value", "W Value"]
    RECT_WIDTH = 13
    # SPACING = 4
    val_models = [None] * axis_count
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        with ui.ZStack():
            with ui.HStack():
                ui.Spacer(width=RECT_WIDTH)
                for i in range(axis_count):
                    val_models[i] = ui.FloatDrag(
                        name="Field",
                        height=LABEL_HEIGHT,
                        min=min,
                        max=max,
                        step=step,
                        alignment=ui.Alignment.LEFT_CENTER,
                        tooltip=field_tooltips[i],
                    ).model
                    val_models[i].set_value(default_val[i])
                    if on_value_changed_fn[i] is not None:
                        val_models[i].add_value_changed_fn(on_value_changed_fn[i])
                    if i != axis_count - 1:
                        ui.Spacer(width=19)
            with ui.HStack():
                for i in range(axis_count):
                    if i != 0:
                        ui.Spacer()  # width=BUTTON_WIDTH - 1)
                    field_label = field_labels[i]
                    with ui.ZStack(width=RECT_WIDTH + 2 * i):
                        ui.Rectangle(name="vector_label", style={"background_color": field_label[1]})
                        ui.Label(field_label[0], name="vector_label", alignment=ui.Alignment.CENTER)
                ui.Spacer()
        add_line_rect_flourish(False)
        return val_models


def color_picker_builder(label="", type="color_picker", default_val=[1.0, 1.0, 1.0, 1.0], tooltip="Color Picker"):
    """Creates a Color Picker Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "color_picker".
        default_val (list, optional): List of (R,G,B,A) default values. Defaults to [1.0, 1.0, 1.0, 1.0].
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "Color Picker".

    Returns:
        AbstractItemModel: ui.ColorWidget.model
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER, tooltip=format_tt(tooltip))
        model = ui.ColorWidget(*default_val, width=BUTTON_WIDTH).model
        ui.Spacer(width=5)
        add_line_rect_flourish()
    return model


def progress_bar_builder(label="", type="progress_bar", default_val=0, tooltip="Progress"):
    """Creates a Progress Bar Widget

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "progress_bar".
        default_val (int, optional): Starting Value. Defaults to 0.
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "Progress".

    Returns:
        AbstractValueModel: ui.ProgressBar().model
    """
    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_CENTER)
        model = ui.ProgressBar().model
        model.set_value(default_val)
        add_line_rect_flourish(False)
    return model


def plot_builder(label="", data=None, min=-1, max=1, type=ui.Type.LINE, value_stride=1, color=None, tooltip=""):
    """Creates a stylized static plot

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        data (list(float), optional): Data to plot. Defaults to None.
        min (int, optional): Minimum Y Value. Defaults to -1.
        max (int, optional): Maximum Y Value. Defaults to 1.
        type (ui.Type, optional): Plot Type. Defaults to ui.Type.LINE.
        value_stride (int, optional): Width of plot stride. Defaults to 1.
        color (int, optional): Plot color. Defaults to None.
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".

    Returns:
        ui.Plot: plot
    """
    with ui.VStack(spacing=5):
        with ui.HStack():
            ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))

            plot_height = LABEL_HEIGHT * 2 + 13
            plot_width = ui.Fraction(1)
            with ui.ZStack():
                ui.Rectangle(width=plot_width, height=plot_height)
                if not color:
                    color = 0xFFDDDDDD
                plot = ui.Plot(
                    type,
                    min,
                    max,
                    *data,
                    value_stride=value_stride,
                    width=plot_width,
                    height=plot_height,
                    style={"color": color, "background_color": 0x0},
                )

            def update_min(model):
                plot.scale_min = model.as_float

            def update_max(model):
                plot.scale_max = model.as_float

            ui.Spacer(width=5)
            with ui.Frame(width=0):
                with ui.VStack(spacing=5):
                    max_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_BOTTOM, tooltip="Max"
                    ).model
                    max_model.set_value(max)
                    min_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_TOP, tooltip="Min"
                    ).model
                    min_model.set_value(min)

                    min_model.add_value_changed_fn(update_min)
                    max_model.add_value_changed_fn(update_max)

            ui.Spacer(width=20)
        add_separator()

    return plot


def xyz_plot_builder(label="", data=[], min=-1, max=1, tooltip=""):
    """Creates a stylized static XYZ plot

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        data (list(float), optional): Data to plot. Defaults to [].
        min (int, optional): Minimum Y Value. Defaults to -1.
        max (int, optional): Maximum Y Value. Defaults to "".
        tooltip (str, optional): Tooltip to display over the Label.. Defaults to "".

    Returns:
        list(ui.Plot): list(x_plot, y_plot, z_plot)
    """
    with ui.VStack(spacing=5):
        with ui.HStack():
            ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))

            plot_height = LABEL_HEIGHT * 2 + 13
            plot_width = ui.Fraction(1)
            with ui.ZStack():
                ui.Rectangle(width=plot_width, height=plot_height)

                plot_0 = ui.Plot(
                    ui.Type.LINE,
                    min,
                    max,
                    *data[0],
                    width=plot_width,
                    height=plot_height,
                    style=get_style()["PlotLabel::X"],
                )
                plot_1 = ui.Plot(
                    ui.Type.LINE,
                    min,
                    max,
                    *data[1],
                    width=plot_width,
                    height=plot_height,
                    style=get_style()["PlotLabel::Y"],
                )
                plot_2 = ui.Plot(
                    ui.Type.LINE,
                    min,
                    max,
                    *data[2],
                    width=plot_width,
                    height=plot_height,
                    style=get_style()["PlotLabel::Z"],
                )

            def update_min(model):
                plot_0.scale_min = model.as_float
                plot_1.scale_min = model.as_float
                plot_2.scale_min = model.as_float

            def update_max(model):
                plot_0.scale_max = model.as_float
                plot_1.scale_max = model.as_float
                plot_2.scale_max = model.as_float

            ui.Spacer(width=5)
            with ui.Frame(width=0):
                with ui.VStack(spacing=5):
                    max_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_BOTTOM, tooltip="Max"
                    ).model
                    max_model.set_value(max)
                    min_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_TOP, tooltip="Min"
                    ).model
                    min_model.set_value(min)

                    min_model.add_value_changed_fn(update_min)
                    max_model.add_value_changed_fn(update_max)
            ui.Spacer(width=20)

        add_separator()
        return [plot_0, plot_1, plot_2]


def combo_cb_plot_builder(
    label="",
    default_val=False,
    on_clicked_fn=lambda x: None,
    data=None,
    min=-1,
    max=1,
    type=ui.Type.LINE,
    value_stride=1,
    color=None,
    tooltip="",
):
    """Creates a Checkbox-Enabled dyanamic plot

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        default_val (bool, optional): Checkbox default. Defaults to False.
        on_clicked_fn (Callable, optional): Checkbox Callback function. Defaults to lambda x: None.
        data (list(), optional): Data to plat. Defaults to None.
        min (int, optional): Min Y Value. Defaults to -1.
        max (int, optional): Max Y Value. Defaults to 1.
        type (ui.Type, optional): Plot Type. Defaults to ui.Type.LINE.
        value_stride (int, optional): Width of plot stride. Defaults to 1.
        color (int, optional): Plot color. Defaults to None.
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".


    Returns:
        list(SimpleBoolModel, ui.Plot): (cb_model, plot)
    """
    with ui.VStack(spacing=5):
        with ui.HStack():
            # Label
            ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))
            # Checkbox
            with ui.Frame(width=0):
                with ui.Placer(offset_x=-10, offset_y=0):
                    with ui.VStack():
                        SimpleCheckBox(default_val, on_clicked_fn)
                        ui.Spacer(height=ui.Fraction(1))
                        ui.Spacer()
            # Plot
            plot_height = LABEL_HEIGHT * 2 + 13
            plot_width = ui.Fraction(1)
            with ui.ZStack():
                ui.Rectangle(width=plot_width, height=plot_height)
                if not color:
                    color = 0xFFDDDDDD
                plot = ui.Plot(
                    type,
                    min,
                    max,
                    *data,
                    value_stride=value_stride,
                    width=plot_width,
                    height=plot_height,
                    style={"color": color, "background_color": 0x0},
                )

            # Min/Max Helpers
            def update_min(model):
                plot.scale_min = model.as_float

            def update_max(model):
                plot.scale_max = model.as_float

            ui.Spacer(width=5)
            with ui.Frame(width=0):
                with ui.VStack(spacing=5):
                    # Min/Max Fields
                    max_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_BOTTOM, tooltip="Max"
                    ).model
                    max_model.set_value(max)
                    min_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_TOP, tooltip="Min"
                    ).model
                    min_model.set_value(min)

                    min_model.add_value_changed_fn(update_min)
                    max_model.add_value_changed_fn(update_max)
            ui.Spacer(width=20)
        with ui.HStack():
            ui.Spacer(width=LABEL_WIDTH + 29)
            # Current Value Field (disabled by default)
            val_model = ui.FloatDrag(
                name="Field",
                width=BUTTON_WIDTH,
                height=LABEL_HEIGHT,
                enabled=False,
                alignment=ui.Alignment.LEFT_CENTER,
                tooltip="Value",
            ).model
        add_separator()
    return plot, val_model


def combo_cb_xyz_plot_builder(
    label="",
    default_val=False,
    on_clicked_fn=lambda x: None,
    data=[],
    min=-1,
    max=1,
    type=ui.Type.LINE,
    value_stride=1,
    tooltip="",
):
    """[summary]

    Args:
        label (str, optional):  Label to the left of the UI element. Defaults to "".
        default_val (bool, optional): Checkbox default. Defaults to False.
        on_clicked_fn (Callable, optional): Checkbox Callback function. Defaults to lambda x: None.
        data list(), optional): Data to plat. Defaults to None.
        min (int, optional): Min Y Value. Defaults to -1.
        max (int, optional): Max Y Value. Defaults to 1.
        type (ui.Type, optional): Plot Type. Defaults to ui.Type.LINE.
        value_stride (int, optional): Width of plot stride. Defaults to 1.
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".

    Returns:
        Tuple(list(ui.Plot), list(AbstractValueModel)): ([plot_0, plot_1, plot_2], [val_model_x, val_model_y, val_model_z])
    """
    with ui.VStack(spacing=5):
        with ui.HStack():
            ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))
            # Checkbox
            with ui.Frame(width=0):
                with ui.Placer(offset_x=-10, offset_y=0):
                    with ui.VStack():
                        SimpleCheckBox(default_val, on_clicked_fn)
                        ui.Spacer(height=ui.Fraction(1))
                        ui.Spacer()
            # Plots
            plot_height = LABEL_HEIGHT * 2 + 13
            plot_width = ui.Fraction(1)
            with ui.ZStack():
                ui.Rectangle(width=plot_width, height=plot_height)

                plot_0 = ui.Plot(
                    type,
                    min,
                    max,
                    *data[0],
                    value_stride=value_stride,
                    width=plot_width,
                    height=plot_height,
                    style=get_style()["PlotLabel::X"],
                )
                plot_1 = ui.Plot(
                    type,
                    min,
                    max,
                    *data[1],
                    value_stride=value_stride,
                    width=plot_width,
                    height=plot_height,
                    style=get_style()["PlotLabel::Y"],
                )
                plot_2 = ui.Plot(
                    type,
                    min,
                    max,
                    *data[2],
                    value_stride=value_stride,
                    width=plot_width,
                    height=plot_height,
                    style=get_style()["PlotLabel::Z"],
                )

            def update_min(model):
                plot_0.scale_min = model.as_float
                plot_1.scale_min = model.as_float
                plot_2.scale_min = model.as_float

            def update_max(model):
                plot_0.scale_max = model.as_float
                plot_1.scale_max = model.as_float
                plot_2.scale_max = model.as_float

            ui.Spacer(width=5)
            with ui.Frame(width=0):
                with ui.VStack(spacing=5):
                    max_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_BOTTOM, tooltip="Max"
                    ).model
                    max_model.set_value(max)
                    min_model = ui.FloatDrag(
                        name="Field", width=40, alignment=ui.Alignment.LEFT_TOP, tooltip="Min"
                    ).model
                    min_model.set_value(min)

                    min_model.add_value_changed_fn(update_min)
                    max_model.add_value_changed_fn(update_max)
            ui.Spacer(width=20)

        # with ui.HStack():
        #     ui.Spacer(width=40)
        #     val_models = xyz_builder()#**{"args":args})

        field_labels = [("X", COLOR_X), ("Y", COLOR_Y), ("Z", COLOR_Z), ("W", COLOR_W)]
        RECT_WIDTH = 13
        # SPACING = 4
        with ui.HStack():
            ui.Spacer(width=LABEL_WIDTH + 29)

            with ui.ZStack():
                with ui.HStack():
                    ui.Spacer(width=RECT_WIDTH)
                    # value_widget = ui.MultiFloatDragField(
                    #     *args, name="multivalue", min=min, max=max, step=step, h_spacing=RECT_WIDTH + SPACING, v_spacing=2
                    # ).model
                    val_model_x = ui.FloatDrag(
                        name="Field",
                        width=BUTTON_WIDTH - 5,
                        height=LABEL_HEIGHT,
                        enabled=False,
                        alignment=ui.Alignment.LEFT_CENTER,
                        tooltip="X Value",
                    ).model
                    ui.Spacer(width=19)
                    val_model_y = ui.FloatDrag(
                        name="Field",
                        width=BUTTON_WIDTH - 5,
                        height=LABEL_HEIGHT,
                        enabled=False,
                        alignment=ui.Alignment.LEFT_CENTER,
                        tooltip="Y Value",
                    ).model
                    ui.Spacer(width=19)
                    val_model_z = ui.FloatDrag(
                        name="Field",
                        width=BUTTON_WIDTH - 5,
                        height=LABEL_HEIGHT,
                        enabled=False,
                        alignment=ui.Alignment.LEFT_CENTER,
                        tooltip="Z Value",
                    ).model
                with ui.HStack():
                    for i in range(3):
                        if i != 0:
                            ui.Spacer(width=BUTTON_WIDTH - 1)
                        field_label = field_labels[i]
                        with ui.ZStack(width=RECT_WIDTH + 1):
                            ui.Rectangle(name="vector_label", style={"background_color": field_label[1]})
                            ui.Label(field_label[0], name="vector_label", alignment=ui.Alignment.CENTER)

        add_separator()
        return [plot_0, plot_1, plot_2], [val_model_x, val_model_y, val_model_z]


def add_line_rect_flourish(draw_line=True):
    """Aesthetic element that adds a Line + Rectangle after all UI elements in the row.

    Args:
        draw_line (bool, optional): Set false to only draw rectangle. Defaults to True.
    """
    if draw_line:
        ui.Line(style={"color": 0x338A8777}, width=ui.Fraction(1), alignment=ui.Alignment.CENTER)
    ui.Spacer(width=10)
    with ui.Frame(width=0):
        with ui.VStack():
            with ui.Placer(offset_x=0, offset_y=7):
                ui.Rectangle(height=5, width=5, alignment=ui.Alignment.CENTER)
    ui.Spacer(width=5)


def add_separator():
    """Aesthetic element to adds a Line Separator."""
    with ui.VStack(spacing=5):
        ui.Spacer()
        with ui.HStack():
            ui.Spacer(width=LABEL_WIDTH)
            ui.Line(style={"color": 0x338A8777}, width=ui.Fraction(1))
            ui.Spacer(width=20)
        ui.Spacer()


def add_folder_picker_icon(
    on_click_fn,
    item_filter_fn=None,
    bookmark_label=None,
    bookmark_path=None,
    dialog_title="Select Output Folder",
    button_title="Select Folder",
):
    def open_file_picker():
        def on_selected(filename, path):
            on_click_fn(filename, path)
            file_picker.hide()

        def on_canceled(a, b):
            file_picker.hide()

        file_picker = FilePickerDialog(
            dialog_title,
            allow_multi_selection=False,
            apply_button_label=button_title,
            click_apply_handler=lambda a, b: on_selected(a, b),
            click_cancel_handler=lambda a, b: on_canceled(a, b),
            item_filter_fn=item_filter_fn,
            enable_versioning_pane=True,
        )
        if bookmark_label and bookmark_path:
            file_picker.toggle_bookmark_from_path(bookmark_label, bookmark_path, True)

    with ui.Frame(width=0, tooltip=button_title):
        ui.Button(
            name="IconButton",
            width=24,
            height=24,
            clicked_fn=open_file_picker,
            style=get_style()["IconButton.Image::FolderPicker"],
            alignment=ui.Alignment.RIGHT_TOP,
        )


def add_folder_picker_btn(on_click_fn):
    def open_folder_picker():
        def on_selected(a, b):
            on_click_fn(a, b)
            folder_picker.hide()

        def on_canceled(a, b):
            folder_picker.hide()

        folder_picker = FilePickerDialog(
            "Select Output Folder",
            allow_multi_selection=False,
            apply_button_label="Select Folder",
            click_apply_handler=lambda a, b: on_selected(a, b),
            click_cancel_handler=lambda a, b: on_canceled(a, b),
        )

    with ui.Frame(width=0):
        ui.Button("SELECT", width=BUTTON_WIDTH, clicked_fn=open_folder_picker, tooltip="Select Folder")


def format_tt(tt):
    import string

    formated = ""
    i = 0
    for w in tt.split():
        if w.isupper():
            formated += w + " "
        elif len(w) > 3 or i == 0:
            formated += string.capwords(w) + " "
        else:
            formated += w.lower() + " "
        i += 1
    return formated


def setup_ui_headers(
    ext_id,
    file_path,
    title="My Custom Extension",
    doc_link="https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/overview.html",
    overview="",
):
    """Creates the Standard UI Elements at the top of each Isaac Extension.

    Args:
        ext_id (str): Extension ID.
        file_path (str): File path to source code.
        title (str, optional): Name of Extension. Defaults to "My Custom Extension".
        doc_link (str, optional): Hyperlink to Documentation. Defaults to "https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/overview.html".
        overview (str, optional): Overview Text explaining the Extension. Defaults to "".
    """
    ext_manager = omni.kit.app.get_app().get_extension_manager()
    extension_path = ext_manager.get_extension_path(ext_id)
    ext_path = os.path.dirname(extension_path) if os.path.isfile(extension_path) else extension_path
    build_header(ext_path, file_path, title, doc_link)
    build_info_frame(overview)


def build_header(
    ext_path,
    file_path,
    title="My Custom Extension",
    doc_link="https://docs.omniverse.nvidia.com/app_isaacsim/app_isaacsim/overview.html",
):
    """Title Header with Quick Access Utility Buttons."""

    def build_icon_bar():
        """Adds the Utility Buttons to the Title Header"""
        with ui.Frame(style=get_style(), width=0):
            with ui.VStack():
                with ui.HStack():
                    icon_size = 24
                    with ui.Frame(tooltip="Open Source Code"):
                        ui.Button(
                            name="IconButton",
                            width=icon_size,
                            height=icon_size,
                            clicked_fn=lambda: on_open_IDE_clicked(ext_path, file_path),
                            style=get_style()["IconButton.Image::OpenConfig"],
                            # style_type_name_override="IconButton.Image::OpenConfig",
                            alignment=ui.Alignment.LEFT_CENTER,
                            # tooltip="Open in IDE",
                        )
                    with ui.Frame(tooltip="Open Containing Folder"):
                        ui.Button(
                            name="IconButton",
                            width=icon_size,
                            height=icon_size,
                            clicked_fn=lambda: on_open_folder_clicked(file_path),
                            style=get_style()["IconButton.Image::OpenFolder"],
                            alignment=ui.Alignment.LEFT_CENTER,
                        )
                    with ui.Placer(offset_x=0, offset_y=3):
                        with ui.Frame(tooltip="Link to Docs"):
                            ui.Button(
                                name="IconButton",
                                width=icon_size - icon_size * 0.25,
                                height=icon_size - icon_size * 0.25,
                                clicked_fn=lambda: on_docs_link_clicked(doc_link),
                                style=get_style()["IconButton.Image::OpenLink"],
                                alignment=ui.Alignment.LEFT_TOP,
                            )

    with ui.ZStack():
        ui.Rectangle(style={"border_radius": 5})
        with ui.HStack():
            ui.Spacer(width=5)
            ui.Label(title, width=0, name="title", style={"font_size": 16})
            ui.Spacer(width=ui.Fraction(1))
            build_icon_bar()
            ui.Spacer(width=5)


def build_info_frame(overview=""):
    """Info Frame with Overview, Instructions, and Metadata for an Extension"""
    frame = ui.CollapsableFrame(
        title="Information",
        height=0,
        collapsed=True,
        horizontal_clipping=False,
        style=get_style(),
        style_type_name_override="CollapsableFrame",
        horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
        vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
    )
    with frame:
        label = "Overview"
        default_val = overview
        tooltip = "Overview"
        with ui.VStack(style=get_style(), spacing=5):
            with ui.HStack():
                ui.Label(label, width=LABEL_WIDTH / 2, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))
                with ui.ScrollingFrame(
                    height=LABEL_HEIGHT * 5,
                    style_type_name_override="ScrollingFrame",
                    alignment=ui.Alignment.LEFT_TOP,
                    horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
                    vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
                ):
                    text = ui.Label(
                        default_val,
                        style_type_name_override="Label::label",
                        word_wrap=True,
                        alignment=ui.Alignment.LEFT_TOP,
                    )
                with ui.Frame(width=0, tooltip="Copy To Clipboard"):
                    ui.Button(
                        name="IconButton",
                        width=20,
                        height=20,
                        clicked_fn=lambda: on_copy_to_clipboard(to_copy=text.text),
                        style=get_style()["IconButton.Image::CopyToClipboard"],
                        alignment=ui.Alignment.RIGHT_TOP,
                    )
    return


# def build_settings_frame(log_filename="extension.log", log_to_file=False, save_settings=False):
#     """Settings Frame for Common Utilities Functions"""
#     frame = ui.CollapsableFrame(
#         title="Settings",
#         height=0,
#         collapsed=True,
#         horizontal_clipping=False,
#         style=get_style(),
#         style_type_name_override="CollapsableFrame",
#     )

#     def on_log_to_file_enabled(val):
#         # TO DO
#         carb.log_info(f"Logging to {model.get_value_as_string()}:", val)

#     def on_save_out_settings(val):
#         # TO DO
#         carb.log_info("Save Out Settings?", val)


#     with frame:
#         with ui.VStack(style=get_style(), spacing=5):

#             # # Log to File Settings
#             # default_output_path = os.path.realpath(os.getcwd())
#             # kwargs = {
#             #     "label": "Log to File",
#             #     "type": "checkbox_stringfield",
#             #     "default_val": [log_to_file, default_output_path + "/" + log_filename],
#             #     "on_clicked_fn": on_log_to_file_enabled,
#             #     "tooltip": "Log Out to File",
#             #     "use_folder_picker": True,
#             # }
#             # model = combo_cb_str_builder(**kwargs)[1]

#             # Save Settings on Exit
#             # kwargs = {
#             #     "label": "Save Settings",
#             #     "type": "checkbox",
#             #     "default_val": save_settings,
#             #     "on_clicked_fn": on_save_out_settings,
#             #     "tooltip": "Save out GUI Settings on Exit.",
#             # }
#             # cb_builder(**kwargs)


class SearchListItem(ui.AbstractItem):
    def __init__(self, text):
        super().__init__()
        self.name_model = ui.SimpleStringModel(text)

    def __repr__(self):
        return f'"{self.name_model.as_string}"'

    def name(self):
        return self.name_model.as_string


class SearchListItemModel(ui.AbstractItemModel):
    """
    Represents the model for lists. It's very easy to initialize it
    with any string list:
        string_list = ["Hello", "World"]
        model = ListModel(*string_list)
        ui.TreeView(model)
    """

    def __init__(self, *args):
        super().__init__()
        self._children = [SearchListItem(t) for t in args]
        self._filtered = [SearchListItem(t) for t in args]

    def get_item_children(self, item):
        """Returns all the children when the widget asks it."""
        if item is not None:
            # Since we are doing a flat list, we return the children of root only.
            # If it's not root we return.
            return []

        return self._filtered

    def filter_text(self, text):
        import fnmatch

        self._filtered = []
        if len(text) == 0:
            for c in self._children:
                self._filtered.append(c)
        else:
            parts = text.split()
            # for i in range(len(parts) - 1, -1, -1):
            #     w = parts[i]

            leftover = " ".join(parts)
            if len(leftover) > 0:
                filter_str = f"*{leftover.lower()}*"
                for c in self._children:
                    if fnmatch.fnmatch(c.name().lower(), filter_str):
                        self._filtered.append(c)

        # This tells the Delegate to update the TreeView
        self._item_changed(None)

    def get_item_value_model_count(self, item):
        """The number of columns"""
        return 1

    def get_item_value_model(self, item, column_id):
        """
        Return value model.
        It's the object that tracks the specific value.
        In our case we use ui.SimpleStringModel.
        """
        return item.name_model


class SearchListItemDelegate(ui.AbstractItemDelegate):
    """
    Delegate is the representation layer. TreeView calls the methods
    of the delegate to create custom widgets for each item.
    """

    def __init__(self, on_double_click_fn=None):
        super().__init__()
        self._on_double_click_fn = on_double_click_fn

    def build_branch(self, model, item, column_id, level, expanded):
        """Create a branch widget that opens or closes subtree"""
        pass

    def build_widget(self, model, item, column_id, level, expanded):
        """Create a widget per column per item"""
        stack = ui.ZStack(height=20, style=get_style())
        with stack:
            with ui.HStack():
                ui.Spacer(width=5)
                value_model = model.get_item_value_model(item, column_id)
                label = ui.Label(value_model.as_string, name="TreeView.Item")

        if not self._on_double_click_fn:
            self._on_double_click_fn = self.on_double_click

        # Set a double click function
        stack.set_mouse_double_clicked_fn(lambda x, y, b, m, l=label: self._on_double_click_fn(b, m, l))

    def on_double_click(self, button, model, label):
        """Called when the user double-clicked the item in TreeView"""
        if button != 0:
            return


def build_simple_search(label="", type="search", model=None, delegate=None, tooltip=""):
    """A Simple Search Bar + TreeView Widget.\n
        Pass a list of items through the model, and a custom on_click_fn through the delegate.\n
        Returns the SearchWidget so user can destroy it on_shutdown.

    Args:
        label (str, optional): Label to the left of the UI element. Defaults to "".
        type (str, optional): Type of UI element. Defaults to "search".
        model (ui.AbstractItemModel, optional): Item Model for Search. Defaults to None.
        delegate (ui.AbstractItemDelegate, optional): Item Delegate for Search. Defaults to None.
        tooltip (str, optional): Tooltip to display over the Label. Defaults to "".

    Returns:
        Tuple(Search Widget, Treeview):
    """

    with ui.HStack():
        ui.Label(label, width=LABEL_WIDTH, alignment=ui.Alignment.LEFT_TOP, tooltip=format_tt(tooltip))

        with ui.VStack(spacing=5):

            def filter_text(item):
                model.filter_text(item)

            from omni.kit.window.extensions.ext_components import SearchWidget

            search_bar = SearchWidget(filter_text)

            with ui.ScrollingFrame(
                height=LABEL_HEIGHT * 5,
                horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_OFF,
                vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_ON,
                style=get_style(),
                style_type_name_override="TreeView.ScrollingFrame",
            ):
                treeview = ui.TreeView(
                    model,
                    delegate=delegate,
                    root_visible=False,
                    header_visible=False,
                    style={
                        "TreeView.ScrollingFrame": {"background_color": 0xFFE0E0E0},
                        "TreeView.Item": {"color": 0xFF535354, "font_size": 16},
                        "TreeView.Item:selected": {"color": 0xFF23211F},
                        "TreeView:selected": {"background_color": 0x409D905C},
                    },
                    # name="TreeView",
                    # style_type_name_override="TreeView",
                )
        add_line_rect_flourish(False)
    return search_bar, treeview
