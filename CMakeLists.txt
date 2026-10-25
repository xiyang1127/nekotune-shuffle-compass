cmake_minimum_required(VERSION 3.28)
project(NekoTuneShuffleCompass VERSION 2.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
include(CTest)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/nekotune-sdk.json" sdk_info)
string(JSON expected_qt GET "${sdk_info}" qt)
string(REGEX MATCH "^[0-9]+\\.[0-9]+" expected_qt_minor "${expected_qt}")
if(NOT "${Qt6_VERSION_MAJOR}.${Qt6_VERSION_MINOR}" STREQUAL expected_qt_minor)
    message(FATAL_ERROR "This release targets Qt ${expected_qt_minor}.x; found ${Qt6_VERSION}. Update nekotune-sdk.json when upgrading the host baseline.")
endif()
find_package(NekoTuneSDK 3 QUIET)
set(NEKOTUNE_SDK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nekotune-sdk" CACHE PATH "SDK source directory (bundled SDK 3 snapshot by default)")
if(NOT NekoTuneSDK_FOUND)
    if(NOT EXISTS "${NEKOTUNE_SDK_SOURCE_DIR}/include/nekotune/sdk/types.h")
        message(FATAL_ERROR "Requires SDK 3. Set CMAKE_PREFIX_PATH to an SDK package, or NEKOTUNE_SDK_SOURCE_DIR to the host sdk directory.")
    endif()
    add_subdirectory("${NEKOTUNE_SDK_SOURCE_DIR}" nekotune-sdk)
endif()

add_library(compass_algorithm STATIC src/algorithm.cpp)
target_include_directories(compass_algorithm PUBLIC src)
target_link_libraries(compass_algorithm PUBLIC NekoTune::SDK)
set_target_properties(compass_algorithm PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
nekotune_add_extension(nekotune_shuffle_compass
    MANIFEST compass/extension.json
    SOURCES src/plugin.cpp
    FILES compass/qml/Page.qml compass/qml/Controller.qml compass/qml/CompassPanel.qml
          compass/qml/BiasPad.qml compass/qml/CompassButton.qml compass/qml/CompassPalette.qml)
target_link_libraries(nekotune_shuffle_compass PRIVATE compass_algorithm)
add_custom_target(extension_package ALL DEPENDS nekotune_shuffle_compass_package)

set(COMPASS_SOURCE_COMMIT "unknown")
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE compass_git_result OUTPUT_VARIABLE compass_git_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(compass_git_result EQUAL 0)
        set(COMPASS_SOURCE_COMMIT "${compass_git_commit}")
    endif()
endif()
configure_file(build-info.json.in
    "${CMAKE_BINARY_DIR}/packages/nekotune.shuffle-compass-build-info.json" @ONLY)

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test QuickTest QuickControls2)
    add_executable(algorithm_test tests/algorithm_test.cpp)
    target_link_libraries(algorithm_test PRIVATE compass_algorithm Qt6::Test)
    add_test(NAME algorithm COMMAND algorithm_test)
    add_executable(plugin_test tests/plugin_test.cpp)
    target_link_libraries(plugin_test PRIVATE NekoTune::SDK Qt6::Test)
    add_dependencies(plugin_test nekotune_shuffle_compass)
    add_test(NAME plugin_lifecycle COMMAND plugin_test $<TARGET_FILE:nekotune_shuffle_compass>)
    add_executable(compass_quick_test tests/quick_test.cpp)
    target_link_libraries(compass_quick_test PRIVATE Qt6::QuickTest Qt6::QuickControls2)
    target_compile_definitions(compass_quick_test PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/qml")
    add_test(NAME interface COMMAND compass_quick_test)
    set_tests_properties(interface PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Fusion")
endif()
