from functools import partial

import omni.ui as ui
from omni.importer.urdf._urdf import UrdfJointType
from omni.importer.urdf.scripts.ui.style import get_style
from omni.importer.urdf.scripts.ui.ui_utils import dropdown_builder


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

    def get_item_list(self):
        return self._item_list

    def get_item_value_model(self, item=None, column_id=-1):
        if item is None:
            return self._current_index
        return item.model

    def get_current_index(self):
        return self._current_index.get_value_as_int()

    def set_current_index(self, index):
        self._current_index.set_value(index)

    def get_current_string(self):
        return self._items[self._current_index.get_value_as_int()].model.get_value_as_string()

    def is_default(self):
        return self.get_current_index() == self._default_index


class UrdfJointModel(ui.AbstractItem):
    target_type = ["None", "Position", "Velocity"]
    drive_type = ["Acceleration", "Force"]

    def __init__(self, joint, value_changed_fn=None):
        super().__init__()
        self.joint = joint
        self._value_changed_fn = value_changed_fn
        self.model_cols = [
            ui.SimpleStringModel(joint.name),
            ComboListModel(UrdfJointModel.target_type, int(joint.drive.target_type)),
            ComboListModel(UrdfJointModel.drive_type, 1 if int(joint.drive.drive_type) else 0),
        ]
        for i in range(1, 3):
            self.model_cols[i].get_item_value_model().add_value_changed_fn(partial(self._on_value_changed, i))

    def _on_value_changed(self, col_id=1, _=None):
        if col_id == 1:
            value = self.model_cols[1].get_item_value_model().get_value_as_int()
            self.joint.drive.set_target_type(value)
        elif col_id == 2:
            drive_type = self.model_cols[2].get_item_value_model().get_value_as_int()
            value = 0 if drive_type == 0 else 2
            self.joint.drive.set_drive_type(value)

        if self._value_changed_fn:
            self._value_changed_fn(self.joint, col_id)

    def get_item_value(self, col_id=0):
        return self.model_cols[col_id].get_value_as_string()

    def get_value_model(self, col_id=0):
        return self.model_cols[col_id]


class UrdfJointItemDelegate(ui.AbstractItemDelegate):
    header = [
        "Name",
        "Target type",
        "Drive type",
    ]

    def __init__(self):
        super().__init__()

    def build_branch(self, model, item=None, column_id=0, level=0, expanded=False):
        pass

    def build_header(self, column_id=0):
        with ui.VStack():
            ui.Label(UrdfJointItemDelegate.header[column_id])
            ui.Spacer(height=ui.Pixel(3))

    def build_widget(self, model, item=None, index=0, level=0, expanded=False):
        if item:
            if index == 0:
                ui.Label(str(model.get_item_value(item, index)))
            elif index in [1, 2]:
                ui.ComboBox(model.get_item_value_model(item, index))


class UrdfJointListModel(ui.AbstractItemModel):
    def __init__(self, joints_list, value_changed_fn, **kwargs):
        super().__init__()
        self._children = [
            UrdfJointModel(j, self._on_joint_changed) for j in joints_list if j.type not in [UrdfJointType.JOINT_FIXED]
        ]
        self._joint_changed_fn = value_changed_fn

    def _on_joint_changed(self, joint, col_id):
        if self._joint_changed_fn:
            self._joint_changed_fn(joint, col_id)

    def get_item_children(self, item=None):
        """Returns all the children when the widget asks it."""
        if item is not None:
            return []
        else:
            return self._children

    def get_item_value(self, item, column_id):
        if item:
            return item.get_item_value(column_id)

    def get_item_value_model_count(self, item):
        """The number of columns"""
        return 3

    def get_item_value_model(self, item, column_id):
        """
        Return value model.
        It's the object that tracks the specific value.
        """
        if item:
            if isinstance(item, UrdfJointModel):
                return item.get_value_model(column_id)


class UrdfJointWidget:
    def __init__(self, joints, value_changed_fn=None):
        self.joints = joints
        self.model = UrdfJointListModel(joints, self._on_value_changed)
        self.delegate = UrdfJointItemDelegate()
        self._build_ui()
        self._value_changed_fn = value_changed_fn

    def set_value_changed_fn(self, value_changed_fn):
        self._value_changed_fn = value_changed_fn

    def _build_ui(self):
        with ui.ScrollingFrame(
            horizontal_scrollbar_policy=ui.ScrollBarPolicy.SCROLLBAR_ALWAYS_OFF,
            style_type_name_override="TreeView",
            height=ui.Pixel(150),
        ):
            self.list = ui.TreeView(
                self.model,
                delegate=self.delegate,
                alignment=ui.Alignment.CENTER_TOP,
                columns_widths=[ui.Fraction(0.5), ui.Fraction(0.25), ui.Fraction(0.25)],
                columns_resizable=False,
                header_visible=True,
                height=ui.Fraction(1),
            )
            # self.list.set_selection_changed_fn(self._on_selection_changed)

    def _on_value_changed(self, joint, col_id=1):
        item = None
        for item in self.list.selection:
            if col_id == 1:
                item.model_cols[col_id].set_current_index(joint.drive.target_type)
                # item.joint.drive.target_type = joint.drive.target_type
            elif col_id == 2:
                # item.joint.drive.drive_type = joint.drive.drive_type
                item.model_cols[col_id].set_current_index(1 if int(joint.drive.drive_type) else 0)
            # item.model_cols[col_id]._item_changed(item)

        if self._value_changed_fn:
            self._value_changed_fn(joint)
