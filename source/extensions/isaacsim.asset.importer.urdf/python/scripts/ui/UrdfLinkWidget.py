import omni.ui as ui


class UrdfLinkWidget:
    def __init__(self, link):
        self.link = link
        self._build_ui()

    def _build_ui(self):
        self.frame = ui.Frame()
        with self.frame:
            with ui.HGrid():
                ui.Label(self.link.name)
