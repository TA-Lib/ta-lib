from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.files import copy
import os


class TaLibConan(ConanFile):
    name = "ta-lib"
    version = "0.8.1"

    license = "BSD-3-Clause"
    url = "https://github.com/ta-lib/ta-lib"
    description = "TA-Lib provides common functions for the technical analysis of stock/future/commodity market data."
    topics = ("technical-analysis", "finance", "trading")

    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    exports_sources = "CMakeLists.txt", "cmake/*", "include/*", "src/*", "LICENSE", "README.md"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_DEV_TOOLS"] = False
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "ta-lib")
        self.cpp_info.set_property("cmake_target_name", "ta-lib")
        self.cpp_info.set_property("pkg_config_name", "ta-lib")

        # Advertise libm wherever the CMake build links it -- i.e. everywhere
        # except Windows (mirrors the `if(NOT WIN32)` guard in CMakeLists.txt).
        # libm is a separate library on Linux, the BSDs, Android, SunOS, ...;
        # on Apple it resolves to the harmless libSystem stub and on Windows
        # the math functions live in the CRT. Needed so static consumers link -lm.
        if self.settings.os != "Windows":
            self.cpp_info.system_libs.append("m")

        if self.options.shared:
            self.cpp_info.libs = ["ta-lib"]
        else:
            if self.settings.os == "Windows":
                self.cpp_info.libs = ["ta-lib-static"]
                self.cpp_info.defines.append("TA_LIB_STATIC")
            else:
                self.cpp_info.libs = ["ta-lib"]

        if self.settings.os != "Windows":
            self.cpp_info.includedirs = ["include/ta-lib"]
