from ctypes import alignment
from enum import Enum
from functools import partial
from re import I

import omni.ui as ui
from isaacsim.asset.importer.urdf._urdf import (
    UrdfJointDriveType,
    UrdfJointTargetType,
    UrdfJointType,
    acquire_urdf_interface,
)
from isaacsim.asset.importer.urdf.scripts.ui.resetable_widget import ResetableLabelField
from isaacsim.asset.importer.urdf.scripts.ui.style import get_style

ITEM_HEIGHT = 26


class ComboListModel(ui.AbstractItemModel):
    class ComboListItem(ui.AbstractItem):
        def __init__(self, item):
            super().__init__()
            self.model = ui.SimpleStringModel(item)

    def __init__(self, item_list, default_index):
        super().__init__()
        self._default_index = default_index
        self._current_index = ui.SimpleIntModel(default_index)
        self._current_index.add_value_changed_fn(lambda a: self._item_changed(None))
        self._item_list = item_list
        self._items = []
        if item_list:
            for item in item_list:
                self._items.append(ComboListModel.ComboListItem(item))

    def get_item_children(self, item):
        return self._items

    def set_items(self, items):
        self._items = []
        for item in items:
            self._items.append(ComboListModel.ComboListItem(item))

    def get_item_list(self):
        return self._item_list

    def get_item_value_model(self, item=None, column_id=-1):
        if item is None:
            return self._current_index
        return item.model

    def get_current_index(self):
        return self._current_index.get_value_as_int()

    def set_current_index(self, index):
        if int(index) < len(self._items):
            self._current_index.set_value(index)

    def get_value_as_string(self):
        if self._current_index.get_value_as_int() < len(self._items):
            return self._items[self._current_index.get_value_as_int()].model.get_value_as_string()
        return self._items[self._default_index].model.get_value_as_string()

    def is_default(self):
        return self.get_current_index() == self._default_index


class SearchableItemSortPolicy(Enum):
    """Sort policy for stage items."""

    DEFAULT = 0
    """The default sort policy."""

    A_TO_Z = 1
    """Sort by name from A to Z."""

    Z_TO_A = 2
    """Sort by name from Z to A."""


class JointSettingMode(Enum):
    """The mode of setting joint parameters."""

    STIFFNESS = 0
    """Set the joint parameters by stiffness."""

    NATURAL_FREQUENCY = 1
    """Set the joint parameters by natural Frequency."""


class JointDriveType(Enum):
    """The mode of setting joint parameters."""

    ACCELERATION = 0
    """Set the joint parameters by acceleration."""

    FORCE = 1
    """Set the joint parameters by force."""


class UrdfJointItem(ui.AbstractItem):
    target_type = [
        "None",
        "Position",
        "Velocity",
    ]
    target_type_with_mimic = ["None", "Position", "Velocity", "Mimic"]

    def __init__(self, config, urdf_robot, joint, value_changed_fn=None):
        super().__init__()

        self._urdf_interface = acquire_urdf_interface()
        self.urdf_robot = urdf_robot
        self.config = config
        self.joint = joint
        self._value_changed_fn = value_changed_fn
        target_type = (
            UrdfJointItem.target_type_with_mimic if self.joint.mimic.joint != "" else UrdfJointItem.target_type
        )

        self.model_cols = [
            ui.SimpleStringModel(joint.name),
            ComboListModel(target_type, self._get_item_target_type()),
            ui.SimpleFloatModel(self.joint.drive.strength),
            ui.SimpleFloatModel(self.joint.drive.damping),
            ui.SimpleFloatModel(self.joint.drive.natural_frequency),
            ui.SimpleFloatModel(self.joint.drive.damping_ratio),
        ]
        self.model_cols[1].get_item_value_model().add_value_changed_fn(partial(self._on_value_changed, 1))
        # Add Model Update UI callbacks
        for i in range(2, 4):
            self.model_cols[i].add_value_changed_fn(partial(self._on_value_changed, i))
        self.model_cols[4].add_value_changed_fn(partial(self._on_value_changed, 2))
        self.model_cols[5].add_value_changed_fn(partial(self._on_value_changed, 3))

        # Add Config update callbacs
        self.model_cols[2].add_value_changed_fn(
            partial(
                self.on_update_strength,
            )
        )
        self.model_cols[3].add_value_changed_fn(
            partial(
                self.on_update_damping,
            )
        )
        self.model_cols[4].add_value_changed_fn(partial(self.on_update_natural_frequency))
        self.model_cols[5].add_value_changed_fn(
            partial(
                self.on_update_damping_ratio,
            )
        )
        self.value_field = {}
        self.mode = JointSettingMode.STIFFNESS

    def on_update_strength(self, model, *args):
        self.joint.drive.strength = model.get_value_as_float()

    def on_update_damping(self, model, *args):
        self.joint.drive.damping = model.get_value_as_float()

    def on_update_natural_frequency(self, model, *args):
        changed = self.joint.drive.natural_frequency != model.get_value_as_float()
        if changed:
            self.compute_drive_strength()
        self.joint.drive.natural_frequency = model.get_value_as_float()

    def on_update_damping_ratio(self, model, *args):
        m_eq = 1
        if self.joint.drive.drive_type == UrdfJointDriveType.JOINT_DRIVE_FORCE:
            m_eq = self.joint.inertia

        self.damping = 2 * m_eq * self.natural_frequency * self.damping_ratio
        self.joint.drive.damping_ratio = model.get_value_as_float()

    @property
    def natural_frequency(self):
        return self.model_cols[4].get_value_as_float()

    @natural_frequency.setter
    def natural_frequency(self, value: float):
        self.model_cols[4].set_value(value)

    @property
    def damping_ratio(self):
        return self.model_cols[5].get_value_as_float()

    @damping_ratio.setter
    def damping_ratio(self, value: float):
        self.model_cols[5].set_value(value)

    @property
    def strength(self):
        return self.model_cols[2].get_value_as_float()

    @strength.setter
    def strength(self, value: float):
        self.model_cols[2].set_value(value)

    @property
    def damping(self):
        return self.model_cols[3].get_value_as_float()

    @damping.setter
    def damping(self, value: float):
        self.model_cols[3].set_value(value)

    @property
    def drive_type(self):
        return self.joint.drive.drive_type

    @drive_type.setter
    def drive_type(self, value: JointDriveType):
        changed = self.joint.drive.drive_type != value
        self.joint.drive.drive_type = value
        if changed:
            self.compute_drive_strength()

    def set_item_target_type(self, value):
        if self.joint.mimic.joint != "" and self.config.parse_mimic:
            value = 3
        self.model_cols[1].set_current_index(value)

    def _get_item_target_type(self):
        if self.joint.mimic.joint != "":
            return 3
        else:
            return int(self.joint.drive.target_type)

    def _on_value_changed(self, col_id=1, _=None):
        if col_id == 1:
            if self.joint.mimic.joint != "" and self.config.parse_mimic:
                value = 3
            else:
                value = self.model_cols[1].get_item_value_model().get_value_as_int()
            self.model_cols[1].set_current_index(value)
            self.joint.drive.set_target_type(value)

        if self._value_changed_fn:
            self._value_changed_fn(self, col_id)

    def compute_drive_strength(self):
        strength = self._urdf_interface.compute_natural_stiffness(
            self.urdf_robot,
            self.joint.name,
            self.natural_frequency,
        )
        value_changed = self.strength != strength
        if value_changed:
            self.strength = strength
            # print(self.joint.drive.target_type)
            if self.joint.drive.target_type == UrdfJointTargetType.JOINT_DRIVE_POSITION:
                m_eq = 1
                # print("drive type", self.joint.drive.drive_type)
                if self.joint.drive.drive_type == UrdfJointDriveType.JOINT_DRIVE_FORCE:
                    m_eq = self.joint.inertia
                    # print("m_eq", m_eq)
                self.damping = 2 * m_eq * self.natural_frequency * self.damping_ratio
                self.joint.drive.damping = self.damping
            self.strength = strength

    def get_item_value(self, col_id=0):
        if col_id in [0, 1]:
            return self.model_cols[col_id].get_value_as_string()
        elif col_id in [2, 3]:
            if self.mode == JointSettingMode.STIFFNESS:
                return self.model_cols[col_id].get_value_as_float()
            else:
                return self.model_cols[col_id + 2].get_value_as_int()

    def set_item_value(self, col_id, value):
        if col_id == 1:
            self.model_cols[col_id].set_current_index(value)
        elif col_id in [2, 3]:
            if self.mode == JointSettingMode.STIFFNESS:
                self.model_cols[col_id].set_value(value)
            else:
                self.model_cols[col_id + 2].set_value(value)

    def get_value_model(self, col_id=0):
        if col_id in [2, 3]:
            if self.mode == JointSettingMode.STIFFNESS:
                return self.model_cols[col_id]
            else:
                return self.model_cols[col_id + 2]
        else:
            return self.model_cols[col_id]


class UrdfJointItemDelegate(ui.AbstractItemDelegate):
    # TODO: the name is too long for "Natural Frequency", "Damping Ratio"
    header_tooltip = ["Name", "Target", "Strength", "Damping", "Natural Frequency", "Damping Ratio"]
    header = ["Name", "Target", "Strength", "Damping", "Nat. Freq.", "Damping R."]

    def __init__(self, model):
        super().__init__()
        self.__model = model
        self.__name_sort_options_menu = None
        self.__items_sort_policy = [SearchableItemSortPolicy.DEFAULT] * self.__model.get_item_value_model_count(None)
        self.__mode = JointSettingMode.STIFFNESS
        self.column_headers = {}

    def set_mode(self, mode):
        self.__mode = mode
        self.update_mimic()

    def build_branch(self, model, item=None, column_id=0, level=0, expanded=False):
        pass

    def build_header(self, column_id=0):
        if column_id in [0, 1]:
            alignment = ui.Alignment.LEFT
            with ui.HStack():
                with ui.VStack():
                    ui.Spacer(height=ui.Pixel(3))
                    ui.Label(
                        UrdfJointItemDelegate.header[column_id],
                        tooltip=UrdfJointItemDelegate.header_tooltip[column_id],
                        name="header",
                        style_type_name_override="TreeView",
                        elided_text=True,
                        alignment=alignment,
                    )
                ui.Image(
                    name="sort",
                    height=19,
                    width=19,
                    mouse_pressed_fn=lambda x, y, b, a, column_id=column_id: self.sort_button_pressed_fn(b, column_id),
                )
        elif column_id in [2, 3]:
            alignment = ui.Alignment.RIGHT
            self.column_headers[column_id] = ui.HStack()
            with self.column_headers[column_id]:
                ui.Image(
                    name="sort",
                    height=19,
                    width=19,
                    mouse_pressed_fn=lambda x, y, b, a, column_id=column_id: self.sort_button_pressed_fn(b, column_id),
                )
                with ui.VStack():
                    ui.Spacer(height=ui.Pixel(3))
                    if self.__mode == JointSettingMode.STIFFNESS:
                        text = UrdfJointItemDelegate.header[column_id]
                    else:
                        text = UrdfJointItemDelegate.header_tooltip[column_id + 2]
                    ui.Label(
                        text,
                        tooltip=text,
                        elided_text=True,
                        name="header",
                        style_type_name_override="TreeView",
                        alignment=alignment,
                    )

    def update_mimic(self):
        for item in self.__model.get_item_children():
            if 1 in item.value_field:
                if not item.config.parse_mimic:
                    item.value_field[1].model.set_items(UrdfJointItem.target_type)
                    item.value_field[1].model._default_index = 1
                    item.value_field[1].model.set_current_index(1)
                else:
                    if item.joint.mimic.joint != "":
                        item.value_field[1].model.set_items(UrdfJointItem.target_type_with_mimic)
                        item.value_field[1].model._default_index = 3
                        item.value_field[1].model.set_current_index(3)
                self.__on_target_change(item, item.value_field[1].model.get_value_as_string())

    def update_defaults(self):
        for item in self.__model.get_item_children():
            for i in [2, 3, 4, 5]:
                field = item.value_field.get(i)
                if field:
                    field.update_default_value()

    def build_widget(self, model, item=None, index=0, level=0, expanded=False):
        if item:
            drive_mode = self.__model.get_item_value_model(item, 1).get_current_index()
            item.mode = self.__mode
            if index == 0:
                with ui.ZStack(height=ITEM_HEIGHT, style_type_name_override="TreeView"):
                    ui.Rectangle(name="treeview_first_item")
                    with ui.VStack():
                        ui.Spacer()
                        with ui.HStack(height=0):
                            ui.Label(
                                str(model.get_item_value(item, index)),
                                tooltip=model.get_item_value(item, index),
                                name="treeview_item",
                                elided_text=True,
                                height=0,
                            )
                            ui.Spacer(width=1)
                        ui.Spacer()
            elif index == 1:
                with ui.ZStack(height=ITEM_HEIGHT, style_type_name_override="TreeView"):
                    ui.Rectangle(name="treeview_item")
                    with ui.HStack():
                        ui.Spacer(width=2)
                        with ui.VStack():
                            ui.Spacer(height=6)
                            with ui.ZStack():
                                item.value_field[index] = ui.ComboBox(
                                    model.get_item_value_model(item, index), name="treeview_item", height=0
                                )
                                item.value_field[index].model.add_item_changed_fn(
                                    lambda m, i: self.__on_target_change(item, m.get_value_as_string())
                                )
                                self.__on_target_change(item, item.value_field[index].model.get_value_as_string())
                                with ui.HStack():
                                    ui.Spacer()
                                    ui.Rectangle(name="mask", width=15)
                                with ui.HStack():
                                    ui.Spacer()
                                    with ui.VStack(width=0):
                                        ui.Spacer()
                                        ui.Triangle(
                                            name="mask", height=5, width=7, alignment=ui.Alignment.CENTER_BOTTOM
                                        )
                                        ui.Spacer()
                                    ui.Spacer(width=2)
                            ui.Spacer(height=2)
                        ui.Spacer(width=2)
            elif index in [2, 3]:
                with ui.ZStack(height=ITEM_HEIGHT):
                    ui.Rectangle(name="treeview_item")
                    with ui.VStack():
                        ui.Spacer()
                        if self.__mode == JointSettingMode.STIFFNESS:
                            index_offset = 0
                        else:
                            index_offset = 2
                        item.value_field[index + index_offset] = ResetableLabelField(
                            model.get_item_value_model(item, index + index_offset), ui.FloatDrag, ".2f"
                        )
                        if index + index_offset in [2, 3]:
                            item.value_field[index + index_offset].visible = drive_mode != 3
                            item.value_field[index + index_offset].enabled = (
                                drive_mode in [1, 2] if index + index_offset == 2 else drive_mode in [1]
                            )
                        ui.Spacer()
            self.update_mimic()

    def sort_button_pressed_fn(self, b, column_id):
        if b != 0:
            return

        def on_sort_policy_changed(policy, value):
            if self.__items_sort_policy[column_id] != policy:
                self.__items_sort_policy[column_id] = policy
                self.__model.sort_by_name(policy, column_id)

        items_sort_policy = self.__items_sort_policy[column_id]
        self.__name_sort_options_menu = ui.Menu("Sort Options")
        with self.__name_sort_options_menu:
            ui.MenuItem("Sort By", enabled=False)
            ui.Separator()
            ui.MenuItem(
                "Ascending",
                checkable=True,
                checked=items_sort_policy == SearchableItemSortPolicy.A_TO_Z,
                checked_changed_fn=partial(on_sort_policy_changed, SearchableItemSortPolicy.A_TO_Z),
                hide_on_click=False,
            )
            ui.MenuItem(
                "Descending",
                checkable=True,
                checked=items_sort_policy == SearchableItemSortPolicy.Z_TO_A,
                checked_changed_fn=partial(on_sort_policy_changed, SearchableItemSortPolicy.Z_TO_A),
                hide_on_click=False,
            )
        self.__name_sort_options_menu.show()

    def __on_target_change(self, item, current_target: str):
        # None: disables all
        # Position: enables all
        # Velocity: enables strength (2) and natural frequency (4)
        # Mimic: disables all
        for field in item.value_field.values():
            field.enabled = True
            field.visible = True
        if current_target == "Mimic":
            if item.config.parse_mimic:
                for i in [1, 2, 3]:
                    if field := item.value_field.get(i):
                        field.enabled = False
                        if i in [2, 3]:
                            field.visible = False
        if current_target == ["None"]:
            for i in [2, 3, 4, 5]:
                if field := item.value_field.get(i):
                    field.enabled = False
        elif current_target == "Position":
            for field in item.value_field.values():
                field.enabled = True
                field.visible = True
        elif current_target == "Velocity":
            if field := item.value_field.get(3):
                field.enabled = False
            if field := item.value_field.get(5):
                field.enabled = False


class UrdfJointListModel(ui.AbstractItemModel):
    def __init__(self, config, urdf_robot, joints_list, value_changed_fn, **kwargs):
        super().__init__()
        self.config = config
        self._children = [
            UrdfJointItem(config, urdf_robot, j, self._on_joint_changed)
            for j in joints_list
            if j.type not in [UrdfJointType.JOINT_FIXED]
        ]
        self._joint_changed_fn = value_changed_fn
        self._items_sort_func = None
        self._items_sort_reversed = False
        self._mode = JointSettingMode.STIFFNESS

    def _on_joint_changed(self, joint, col_id):
        if self._joint_changed_fn:
            self._joint_changed_fn(joint, col_id)

    def get_item_children(self, item=None):
        """Returns all the children when the widget asks it."""
        if item is not None:
            return []
        else:
            children = self._children
            if self._items_sort_func:
                children = sorted(children, key=self._items_sort_func, reverse=self._items_sort_reversed)

            return children

    def get_item_value(self, item, column_id):
        if item:
            return item.get_item_value(column_id)

    def get_item_value_model_count(self, item):
        """The number of columns"""
        return 4

    def get_item_value_model(self, item, column_id):
        """
        Return value model.
        It's the object that tracks the specific value.
        """
        if item:
            if isinstance(item, UrdfJointItem):
                return item.get_value_model(column_id)

    def sort_by_name(self, policy, column_id):
        if policy == SearchableItemSortPolicy.Z_TO_A:
            self._items_sort_reversed = True
        else:
            self._items_sort_reversed = False
        if column_id in [0, 1]:
            self._items_sort_func = (
                lambda item: self.get_item_value_model(item, column_id).get_value_as_string().lower()
            )
        if column_id in [2, 3]:
            if self._mode == JointSettingMode.STIFFNESS:
                self._items_sort_func = lambda item: self.get_item_value_model(item, column_id).get_value_as_float()
            else:
                self._items_sort_func = lambda item: self.get_item_value_model(item, column_id).get_value_as_int()
        self._item_changed(None)

    def set_mode(self, mode):
        if self._mode != mode:
            self._mode = mode
            for item in self.get_item_children():
                item.mode = mode
                self._item_changed(item)
            self._item_changed(None)

    def set_drive_type(self, drive_type):
        for item in self._children:
            item.joint.drive.set_drive_type(drive_type)
            item.compute_drive_strength()
            self._item_changed(item)
        self._item_changed(None)


class TreeViewIDColumn:
    """
    This is the ID (first) column of the TreeView. It's not part of the treeview delegate, because it's cheaper to do
    item remove in this way. And we dont need to update it when the treeview list is smaller than DEFAULT_ITEM_NUM.
    """

    DEFAULT_ITEM_NUM = 0

    def __init__(self, num=DEFAULT_ITEM_NUM):
        self.frame = ui.Frame()
        self.num = num
        self.frame.set_build_fn(self._build_frame)

    def _build_frame(self):
        self.column = ui.VStack(width=15, spacing=0)
        with self.column:
            ui.Rectangle(name="treeview_id", height=19)
            for i in range(self.num + 1):
                with ui.ZStack(height=ITEM_HEIGHT):
                    ui.Rectangle(name="treeview_id")
                    with ui.VStack():
                        ui.Spacer()
                        ui.Label(str(i + 1), alignment=ui.Alignment.CENTER, height=0)
                        ui.Spacer()

    def add_item(self):
        self.num += 1
        with self.column:
            with ui.ZStack(height=ITEM_HEIGHT):
                ui.Rectangle(name="treeview_id")
                ui.Label(str(self.num), alignment=ui.Alignment.CENTER, height=0)
        if self.num > TreeViewIDColumn.DEFAULT_ITEM_NUM:
            self.frame.rebuild()

    def remove_item(self):
        if self.num > TreeViewIDColumn.DEFAULT_ITEM_NUM:
            self.num -= 1
            self.frame.rebuild()


class UrdfJointWidget:
    def __init__(self, config, urdf_robot, joints, value_changed_fn=None):
        self.config = config
        self.joints = joints
        self.model = UrdfJointListModel(config, urdf_robot, joints, self._on_value_changed)
        self.delegate = UrdfJointItemDelegate(self.model)
        self._enable_bulk_edit = True
        self._value_changed_fn = value_changed_fn
        self.mode = JointSettingMode.STIFFNESS
        self.drive_type = JointDriveType.ACCELERATION
        self._build_ui()

    def update_mimic(self):
        self.delegate.update_mimic()

    def _build_ui(self):

        with ui.ScrollingFrame(
            horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_OFF,
            vertical_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_AS_NEEDED,
            style_type_name_override="TreeView",
            height=ui.Pixel(150),
        ):
            with ui.HStack():
                self.id_column = TreeViewIDColumn(len(self.joints) - 1)
                self.list = ui.TreeView(
                    self.model,
                    delegate=self.delegate,
                    alignment=ui.Alignment.CENTER_TOP,
                    column_widths=[ui.Fraction(1), ui.Pixel(65), ui.Pixel(70), ui.Pixel(70)],
                    # TODO: uncomment this when we could set the default option width
                    min_column_widths=[80, 65, 30, 30],
                    columns_resizable=False,
                    header_visible=True,
                    height=ui.Fraction(1),
                )

    def set_bulk_edit(self, enable_bulk_edit: bool):
        self._enable_bulk_edit = enable_bulk_edit

    def switch_mode(self, switch: JointSettingMode):

        self.delegate.set_mode(switch)
        self.model.set_mode(switch)
        self.mode = switch
        self.update_mimic()

    def switch_drive_type(self, drive_type: JointDriveType):
        self.set_bulk_edit(False)
        urdf_drive_type = UrdfJointDriveType.JOINT_DRIVE_ACCELERATION
        if JointDriveType(drive_type) == JointDriveType.FORCE:
            urdf_drive_type = UrdfJointDriveType.JOINT_DRIVE_FORCE
        self.model.set_drive_type(urdf_drive_type)
        if self.mode == JointSettingMode.STIFFNESS:
            self.delegate.update_defaults()
        self.drive_type = urdf_drive_type
        self.set_bulk_edit(True)

    def _on_value_changed(self, joint_item, col_id=1):
        if self._enable_bulk_edit:
            for item in self.list.selection:
                if item is not joint_item:
                    if col_id == 1:
                        item.set_item_value(col_id, joint_item.joint.drive.target_type)
                    if col_id in range(2, 4):
                        value = joint_item.strength if col_id == 2 else joint_item.damping
                        if self.mode != JointSettingMode.STIFFNESS:
                            value = joint_item.natural_frequency if col_id == 2 else joint_item.damping_ratio
                        if item.get_item_value(1) in ["Position", "Velocity"]:
                            if not (item.get_item_value(1) == "Velocity" and col_id == 3):
                                if item.get_item_value(col_id) != value:
                                    item.set_item_value(col_id, value)

        if self._value_changed_fn:
            self._value_changed_fn(joint_item.joint)
