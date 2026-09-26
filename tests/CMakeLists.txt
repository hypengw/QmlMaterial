find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Test GuiPrivate QuickPrivate)
enable_testing()

set(QM_TEST_SOURCES
  integration/lazy_list.cpp
  layout/viewport_controller.cpp
  layout/linear_layout.cpp
  integration/scroll_motion.cpp
  integration/nested_scroll.cpp
  integration/lazy_data.cpp
  integration/icon_spec.cpp
  carousel/hero_center_end.cpp
  carousel/fullscreen_layout.cpp
  carousel/multi_browse_layout.cpp
  carousel/uncontained_layout.cpp
  carousel/hero_start_layout.cpp
  carousel/carousel_engine_defaults.cpp
  model/appstream_release_model.cpp
  carousel/set_current_index_sync.cpp
  carousel/model_shrink.cpp
  carousel/model_data_changed.cpp
  carousel/model_current_item.cpp
  layout/toolbar_dynamic_visibility.cpp
  layout/lite_layout.cpp
  layout/split_view.cpp
  layout/item_proxy.cpp
  integration/adaptive_presenter.cpp
  layout/control_layout.cpp
  integration/dynamic_creation.cpp
  integration/attached_property_propagation.cpp
  layout/page_layout.cpp)

add_executable(qm_tests main.cpp ${QM_TEST_SOURCES})
set_target_properties(qm_tests PROPERTIES AUTOMOC ON)
target_include_directories(qm_tests PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/view)
target_link_libraries(qm_tests PRIVATE qml_material qml_material_layouts material_color
  Qt6::Test Qt6::QuickPrivate Qt6::GuiPrivate)
target_compile_definitions(qm_tests PRIVATE
  QM_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
  QM_ICON_TEST_QML="${CMAKE_CURRENT_SOURCE_DIR}/integration/icon_spec.qml"
  TOOLBAR_DYNAMIC_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/layout/toolbar_dynamic_visibility.qml")
add_dependencies(qm_tests qml_materialplugin qml_material_layoutsplugin)
if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  target_link_libraries(qm_tests PRIVATE qml_materialplugin qml_material_layoutsplugin)
else()
  # These internal implementations are not exported by the shared library.
  target_sources(qm_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/carousel/carousel_view.cpp
    ${PROJECT_SOURCE_DIR}/src/carousel/carousel_attached.cpp
    ${PROJECT_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
    ${PROJECT_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
    ${PROJECT_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp
    ${PROJECT_SOURCE_DIR}/include/qml_material/carousel/carousel_view.hpp
    ${PROJECT_SOURCE_DIR}/include/qml_material/carousel/carousel_attached.hpp
    ${PROJECT_SOURCE_DIR}/src/view/delegate_recycler.cpp)
endif()
qt_import_qml_plugins(qm_tests)

set(QM_APPLESS_TESTS viewport_controller linear_layout carousel_engine_defaults)
set(QM_CORE_TESTS hero_center_end fullscreen_layout multi_browse_layout
  uncontained_layout hero_start_layout appstream_release_model)
set(QM_TEST_GROUPS "")
foreach(source IN LISTS QM_TEST_SOURCES)
  get_filename_component(group "${source}" NAME_WE)
  if(group IN_LIST QM_APPLESS_TESTS)
    set(application None)
  elseif(group IN_LIST QM_CORE_TESTS)
    set(application Core)
  else()
    set(application Gui)
  endif()
  string(APPEND QM_TEST_GROUPS "QM_TEST_GROUP(${group}, ${application})\n")
  add_test(NAME ${group} COMMAND qm_tests ${group})
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_groups.inc" CONTENT "${QM_TEST_GROUPS}")

set_tests_properties(lazy_list scroll_motion lazy_data icon_spec toolbar_dynamic_visibility
  PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
set_tests_properties(lite_layout control_layout dynamic_creation attached_property_propagation
  page_layout split_view item_proxy nested_scroll adaptive_presenter PROPERTIES ENVIRONMENT
  "QT_QPA_PLATFORM=offscreen;QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}")
set_tests_properties(control_layout PROPERTIES ENVIRONMENT
  "QT_QPA_PLATFORM=offscreen;QT_ENABLE_HIGHDPI_SCALING=0;QT_SCALE_FACTOR=1;QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}")
set_tests_properties(control_layout dynamic_creation attached_property_propagation
  PROPERTIES RUN_SERIAL ON)

add_executable(qm_grab visual/qm_grab.cpp)
target_link_libraries(qm_grab PRIVATE Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Quick Qt6::Qml)
set_target_properties(qm_grab PROPERTIES AUTOMOC ON)

add_executable(lazy_list_benchmark EXCLUDE_FROM_ALL benchmark/lazy_list.cpp)
target_compile_definitions(lazy_list_benchmark PRIVATE QM_BENCHMARK_BUILD_TYPE="$<CONFIG>")
target_link_libraries(lazy_list_benchmark PRIVATE qml_material Qt6::Quick Qt6::Qml)
add_dependencies(lazy_list_benchmark qml_materialplugin qml_material_layoutsplugin)
if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  target_link_libraries(lazy_list_benchmark PRIVATE qml_materialplugin qml_material_layoutsplugin)
endif()
qt_import_qml_plugins(lazy_list_benchmark)

set(VISUAL_SCENES button action_progress elevation shape ripple ripple_phases loading_indicator_shapes tooltip wave_progress snackbar
    icon_artifacts icon_spec
    focus_button focus_iconbutton focus_fab focus_chip
    focus_switch focus_checkbox focus_radio
    datepicker colorpicker colorpicker_button
    navigation_rail navigation_drawer bottom_sheet scrollable carousel table)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out)

foreach(scene ${VISUAL_SCENES})
  add_test(
    NAME visual_${scene}
    COMMAND bash -c
      "LIBGL_ALWAYS_SOFTWARE=1 \
       QT_QPA_PLATFORM=\${QT_QPA_PLATFORM:-eglfs} \
       QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/qml_modules \
       $<TARGET_FILE:qm_grab> \
         ${CMAKE_CURRENT_SOURCE_DIR}/visual/scenes/${scene}.qml \
         ${CMAKE_CURRENT_BINARY_DIR}/out/visual_${scene}.png \
       && test -s ${CMAKE_CURRENT_BINARY_DIR}/out/visual_${scene}.png"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endforeach()
