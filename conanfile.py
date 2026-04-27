from conan import ConanFile


class OpenRVRecipe(ConanFile):
    name = "openrv"
    version = "1.0.0"
    python_requires = "openrvcore/1.0.0"
    python_requires_extend = "openrvcore.OpenRVBase"

    settings = "os", "compiler", "build_type", "arch"

    options = {
        "vfx_platform": ["CY2023", "CY2024"],
    }
    default_options = {
        "vfx_platform": "CY2024",
        "openjph/*:with_tiff": False,
    }

    def layout(self):
        super().layout()

    def build_requirements(self):
        super().build_requirements()

    def requirements(self):
        super().requirements()

    def generate(self):
        super().generate()

    def build(self):
        super().build()
