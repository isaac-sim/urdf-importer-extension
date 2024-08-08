import omni.ui as ui


class ResetButton:
    def __init__(self, init_value, on_reset_fn):
        self._init_value = init_value
        self._on_reset_fn = on_reset_fn
        self._enable = True
        self._build_ui()

    @property
    def enable(self):
        return self._enable

    @enable.setter
    def enable(self, enable):
        self._enable = enable
        self._reset_button.enabled = enable

    def refresh(self, new_value):
        self._reset_button.visible = bool(self._init_value != new_value)

    def _build_ui(self):
        with ui.VStack(width=0, height=0):
            ui.Spacer()
            with ui.ZStack(width=15, height=15):
                with ui.HStack():
                    ui.Spacer()
                    with ui.VStack(width=0):
                        ui.Spacer()
                        ui.Rectangle(width=5, height=5, name="reset_invalid")
                        ui.Spacer()
                    ui.Spacer()
                with ui.HStack(width=15, height=15):
                    ui.Spacer(width=2)
                    with ui.VStack(height=15):
                        ui.Spacer()
                        self._reset_real_button = ui.Button(
                            " ", width=10, height=10, name="reset", clicked_fn=lambda: self._restore_defaults()
                        )
                        ui.Spacer()
                    ui.Spacer(width=2)
                with ui.HStack(width=15, height=15):
                    ui.Spacer(width=2)
                    with ui.VStack(height=15):
                        ui.Spacer(height=5)
                        self._reset_button = ui.Rectangle(
                            width=10, height=10, name="reset", tooltip="Click to reset value"
                        )
                        ui.Spacer(height=5)
                    ui.Spacer(width=2)
                self._reset_button.visible = False
            # self._reset_button.set_mouse_pressed_fn(lambda x, y, m, w: self._restore_defaults())
            ui.Spacer()

    def _restore_defaults(self):
        if not self._enable:
            return
        self._reset_button.visible = False
        if self._on_reset_fn:
            self._on_reset_fn()


class ResetableLabelField:
    def __init__(self, value_model, field_type, format, alignment=ui.Alignment.RIGHT_CENTER):
        self._value_model = value_model
        self._init_value = self.get_model_value(value_model)
        self._field_type = field_type
        self._alignment = alignment
        self._enable = True
        self._frame = ui.HStack(height=26, spacing=2)
        self._format = format
        self._build_ui()

    @property
    def enabled(self):
        return self._enable

    def update_default_value(self):
        self._init_value = self.get_model_value(self._value_model)
        self._reset_button._init_value = self._init_value
        self._reset_button.refresh(self._init_value)

    @property
    def visible(self):
        return self._frame.visible

    @visible.setter
    def visible(self, value):
        self._frame.visible = value

    @enabled.setter
    def enabled(self, enable):
        self._enable = enable
        self._field.enabled = enable
        self._reset_button.enable = enable

    def get_model_value(self, model):
        if isinstance(model, ui.SimpleStringModel):
            return model.get_value_as_string()
        if isinstance(model, ui.SimpleIntModel):
            return model.get_value_as_int()
        if isinstance(model, ui.SimpleFloatModel):
            return model.get_value_as_float()
        return ""

    def _build_ui(self):
        with self._frame:
            ui.Spacer(width=1)
            with ui.ZStack():
                with ui.VStack(height=0):
                    ui.Spacer(height=2)
                    self._field = self._field_type(
                        name="resetable", style_type_name_override="Field", alignment=self._alignment, height=18
                    )
                    ui.Spacer(height=2)
            self._field.model.set_value(self._init_value)
            self._field.model.add_value_changed_fn(lambda m: self._update_value(m))
            # it used to bulk edit, we need the field hook with the value model' value
            self._value_model.add_value_changed_fn(lambda m: self._update_field(m))
            self.subscription = self._field.model.subscribe_end_edit_fn(lambda m: self._end_edit(m))
            with ui.VStack(width=8):
                ui.Spacer()
                self._reset_button = ResetButton(self._init_value, self._on_reset_fn)
                ui.Spacer()

    def _on_reset_fn(self):
        current_value = self.get_model_value(self._field.model)
        if current_value != self._init_value:
            self._field.model.set_value(self._init_value)
            self._value_model.set_value(self._init_value)

    def _update_value(self, model):
        new_value = self.get_model_value(model)
        current_value = self.get_model_value(self._value_model)
        if new_value != current_value:
            self._value_model.set_value(new_value)
            self._reset_button.refresh(new_value)

    def _update_field(self, model):
        new_value = self.get_model_value(model)
        current_value = self.get_model_value(self._field.model)
        if new_value != current_value:
            self._field.model.set_value(new_value)
            self._reset_button.refresh(new_value)

    def _end_edit(self, model):
        pass

    def _begin_edit(self):
        if not self._enable:
            return
