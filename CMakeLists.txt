# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(test-task-led)

# ---------------------------------------------------------------------------
# Pre-build: update FIRMWARE_VERSION in source before each compilation.
# The script increments the build index (XX) when the date part matches today,
# or resets it to 00 when the date changes. See update_version.py for details.
# add_dependencies ensures the version is bumped before the ELF is linked.
# ---------------------------------------------------------------------------
add_custom_target(firmware_version_update
    COMMAND python3 ${CMAKE_SOURCE_DIR}/update_version.py ${CMAKE_SOURCE_DIR}
    COMMENT "Updating firmware version..."
    VERBATIM
)
add_dependencies(${CMAKE_PROJECT_NAME}.elf firmware_version_update)
