# Optional integration project loaded by the NekoTune host's NEKOTUNE_EXTERNAL_TESTS_DIR.
if(NOT TARGET nekotune_core OR NOT TARGET nekotune_frontend)
    message(FATAL_ERROR "Configure this test through the NekoTune host")
endif()
set(NEKOTUNE_COMPASS_PACKAGE "" CACHE FILEPATH "Built Shuffle Compass ZIP")
if(NOT EXISTS "${NEKOTUNE_COMPASS_PACKAGE}")
    message(FATAL_ERROR "Set NEKOTUNE_COMPASS_PACKAGE to the built extension ZIP")
endif()
add_executable(compass_host_ui_test host_ui_test.cpp
    "${CMAKE_SOURCE_DIR}/frontend/qt-qml/resources.qrc")
target_link_libraries(compass_host_ui_test PRIVATE nekotune_core nekotune_frontend
    Qt6::Test Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(compass_host_ui_test PRIVATE COMPASS_PACKAGE="${NEKOTUNE_COMPASS_PACKAGE}")
add_test(NAME compass_host_ui COMMAND compass_host_ui_test)
set_tests_properties(compass_host_ui PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;QT_QUICK_CONTROLS_STYLE=Fusion")
