cmake_minimum_required(VERSION 3.20)
project(WarehouseTransportSim VERSION 1.0.0 LANGUAGES CXX)

option(BUILD_GUI "构建 Windows/EasyX 图形界面" OFF)
set(EASYX_ROOT "" CACHE PATH "EasyX 根目录，应包含 include 与 lib 子目录")

add_library(warehouse_core
    src/context.cpp
    src/geometry.cpp
    src/scenario.cpp
    src/motion.cpp
    src/station.cpp
    src/dispatch.cpp
    src/logging.cpp
    src/simulation.cpp
    src/run_config.cpp
)
target_include_directories(warehouse_core PUBLIC include)
target_compile_features(warehouse_core PUBLIC cxx_std_17)
set_target_properties(warehouse_core PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(warehouse_core PUBLIC /utf-8)
    target_compile_definitions(warehouse_core PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(warehouse_core PRIVATE -Wall -Wextra)
endif()

add_executable(WarehouseTransportCLI app/cli_main.cpp)
target_link_libraries(WarehouseTransportCLI PRIVATE warehouse_core)
if(WIN32)
    target_link_libraries(WarehouseTransportCLI PRIVATE shell32)
endif()

if(BUILD_GUI)
    if(NOT WIN32 OR NOT MSVC)
        message(FATAL_ERROR "图形界面需要 Windows、MSVC 和 EasyX；核心可用 BUILD_GUI=OFF 构建。")
    endif()
    # 优先使用用户指定路径，也识别官方安装器的 Visual Studio 安装位置。
    file(GLOB _easyx_vs_roots "C:/Program Files/Microsoft Visual Studio/2022/*/VC/Auxiliary/VS")
    set(_easyx_roots "${EASYX_ROOT}" "$ENV{VCINSTALLDIR}/Auxiliary/VS" ${_easyx_vs_roots})
    set(_easyx_includes)
    set(_easyx_libraries)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(_easyx_arch x64)
    else()
        set(_easyx_arch x86)
    endif()
    foreach(_root IN LISTS _easyx_roots)
        if(_root)
            list(APPEND _easyx_includes "${_root}/include" "${_root}")
            list(APPEND _easyx_libraries "${_root}/lib/${_easyx_arch}" "${_root}/lib" "${_root}")
        endif()
    endforeach()
    find_path(EASYX_INCLUDE_DIR graphics.h HINTS ${_easyx_includes})
    find_library(EASYX_LIBRARY NAMES EasyXa HINTS ${_easyx_libraries})
    if(NOT EASYX_INCLUDE_DIR OR NOT EASYX_LIBRARY)
        message(FATAL_ERROR "未找到 EasyX。请按 docs/USAGE.md 安装，或传入 -DEASYX_ROOT=<目录>。")
    endif()
    add_executable(WarehouseTransportSystem
        app/gui_main.cpp
        src/rendering.cpp
        src/effects.cpp
        src/menu.cpp
        src/gui_application.cpp
    )
    target_include_directories(WarehouseTransportSystem PRIVATE "${EASYX_INCLUDE_DIR}")
    get_filename_component(_easyx_lib_dir "${EASYX_LIBRARY}" DIRECTORY)
    target_link_directories(WarehouseTransportSystem PRIVATE "${_easyx_lib_dir}")
    target_link_libraries(WarehouseTransportSystem PRIVATE warehouse_core "${EASYX_LIBRARY}" winmm msimg32 gdi32 user32)
    target_compile_definitions(WarehouseTransportSystem PRIVATE WINVER=0x0601 _WIN32_WINNT=0x0601 _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(warehouse_core_tests tests/core_tests.cpp)
    target_link_libraries(warehouse_core_tests PRIVATE warehouse_core)
    add_test(NAME core_invariants COMMAND warehouse_core_tests)
    add_executable(warehouse_parity_probe tests/parity_probe.cpp)
    target_include_directories(warehouse_parity_probe PRIVATE tests)
    target_link_libraries(warehouse_parity_probe PRIVATE warehouse_core)
    add_test(NAME cli_help COMMAND WarehouseTransportCLI --help)
    add_test(NAME cli_invalid_mode COMMAND WarehouseTransportCLI --mode unknown)
    set_tests_properties(cli_invalid_mode PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_invalid_step COMMAND WarehouseTransportCLI --time-step nan)
    set_tests_properties(cli_invalid_step PROPERTIES WILL_FAIL TRUE)
    add_test(NAME cli_patrol_three_only COMMAND WarehouseTransportCLI --mode patrol --cars 5)
    set_tests_properties(cli_patrol_three_only PROPERTIES WILL_FAIL TRUE)
    foreach(_cars IN ITEMS 3 5 7)
        add_test(NAME schedule_${_cars}_cars COMMAND WarehouseTransportCLI --mode schedule --cars ${_cars}
            --seed 42 --max-seconds 10800 --output "${CMAKE_CURRENT_BINARY_DIR}/test-output/schedule-${_cars}" --no-logs)
    endforeach()
    add_test(NAME patrol_3_cars COMMAND WarehouseTransportCLI --mode patrol --cars 3
        --seed 42 --max-seconds 600 --output "${CMAKE_CURRENT_BINARY_DIR}/test-output/patrol-3" --no-logs)
endif()
