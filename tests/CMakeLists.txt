find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Test)
foreach(component GuiPrivate QuickPrivate)
  if(NOT TARGET Qt6::${component})
    find_package(Qt6 REQUIRED COMPONENTS ${component})
  endif()
endforeach()
enable_testing()

set(QM_TEST_SOURCES
  carousel/carousel_control.cpp
  carousel/carousel_input.cpp
  carousel/carousel_keyline_mapping.cpp
  carousel/carousel_layout.cpp
  carousel/carousel_position.cpp
  carousel/fullscreen_layout.cpp
  carousel/hero_center_end.cpp
  carousel/hero_start_layout.cpp
  carousel/model_current_item.cpp
  carousel/model_data_changed.cpp
  carousel/model_shrink.cpp
  carousel/multi_browse_layout.cpp
  carousel/set_current_index_sync.cpp
  carousel/uncontained_layout.cpp
  control/action_command.cpp
  control/action_selection.cpp
  control/button_group.cpp
  control/button_presentation.cpp
  control/button_selection.cpp
  control/clock_presets.cpp
  control/clock_source.cpp
  control/color_picker_input.cpp
  control/combo_box_input.cpp
  control/control_bindings.cpp
  control/control_registration.cpp
  control/date_input.cpp
  control/delegate_input.cpp
  control/extended_fab.cpp
  control/fab_menu.cpp
  control/floating_toolbar.cpp
  control/icon_spec.cpp
  control/navigation_rail.cpp
  control/progress_indicator.cpp
  control/range_slider.cpp
  control/range_slider_requests.cpp
  control/scroll_bar_input.cpp
  control/search_view.cpp
  control/selection_requests.cpp
  control/slider_input.cpp
  control/slider_requests.cpp
  control/snake_bar_input.cpp
  control/split_button_input.cpp
  control/standard_drawer_input.cpp
  control/text_field_input.cpp
  control/time_input.cpp
  control/toggle_requests.cpp
  control/value_input.cpp
  core/application_lifetime.cpp
  core/attached_property_propagation.cpp
  core/object_pool.cpp
  core/state_bindings.cpp
  example/clock_widgets.cpp
  example/compositions.cpp
  example/example_navigation.cpp
  example/example_pickers.cpp
  example/example_smoke.cpp
  example/media_card.cpp
  example/motion_example.cpp
  example/notifications.cpp
  example/quick_settings.cpp
  input/controlled_input.cpp
  input/input_host.cpp
  input/interaction_source.cpp
  layout/control_layout.cpp
  layout/icon_layout.cpp
  layout/item_proxy.cpp
  layout/linear_layout.cpp
  layout/lite_layout.cpp
  layout/menu_layout.cpp
  layout/page_layout.cpp
  layout/page_stack_transition.cpp
  layout/split_view.cpp
  layout/text_field_form.cpp
  layout/toolbar_dynamic_visibility.cpp
  layout/viewport_controller.cpp
  model/appstream_release_model.cpp
  model/lazy_data.cpp
  motion/control_motion.cpp
  motion/motion_presets.cpp
  motion/ripple_motion.cpp
  motion/spring_motion.cpp
  popup/adaptive_presenter.cpp
  popup/menu_placement.cpp
  popup/popup_layout.cpp
  popup/popup_lifecycle.cpp
  popup/popup_presenter.cpp
  popup/side_sheet.cpp
  scrollable/nested_scroll.cpp
  scrollable/pull_to_refresh.cpp
  scrollable/scroll_motion.cpp
  scrollable/swipe_to_dismiss.cpp
  shape/material_shapes.cpp
  style/control_feedback.cpp
  style/state_lifetime.cpp
  view/lazy_list.cpp
  view/lazy_list_scroll_bar.cpp
  visual/visual_blur_mask.cpp)

add_executable(qm_tests main.cpp ${QM_TEST_SOURCES})
set_target_properties(qm_tests PROPERTIES AUTOMOC ON)
target_include_directories(qm_tests PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src/view)
target_link_libraries(qm_tests PRIVATE qml_material qml_material_layouts material_color
  Qt6::Test Qt6::QuickPrivate Qt6::GuiPrivate)
target_compile_definitions(qm_tests PRIVATE
  QM_QML_IMPORT_PATH="${QT_QML_OUTPUT_DIRECTORY}"
  QM_ICON_TEST_QML="${CMAKE_CURRENT_SOURCE_DIR}/control/icon_spec.qml"
  QM_LIST_ITEM_STATE_QML="${CMAKE_CURRENT_SOURCE_DIR}/style/list_item_state.qml"
  TOOLBAR_DYNAMIC_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/layout/toolbar_dynamic_visibility.qml")
add_dependencies(qm_tests qml_materialplugin qml_material_layoutsplugin)
if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  target_link_libraries(qm_tests PRIVATE qml_materialplugin qml_material_layoutsplugin)
else()
  # DelegateRecycler remains internal to the view implementation.
  target_sources(qm_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/view/delegate_recycler.cpp)
endif()
qt_import_qml_plugins(qm_tests)

set(QM_APPLESS_TESTS viewport_controller linear_layout application_lifetime state_bindings)
set(QM_CORE_TESTS carousel_keyline_mapping hero_center_end fullscreen_layout multi_browse_layout
  uncontained_layout hero_start_layout appstream_release_model)
set(QM_FIXED_DPI_TESTS control_layout popup_layout state_lifetime icon_layout menu_layout
  carousel_layout lazy_list_scroll_bar)
set(QM_SERIAL_TESTS ${QM_FIXED_DPI_TESTS} popup_lifecycle popup_presenter object_pool
  attached_property_propagation)
set(QM_TEST_GROUPS "")
foreach(source IN LISTS QM_TEST_SOURCES)
  get_filename_component(group "${source}" NAME_WE)
  get_filename_component(category "${source}" DIRECTORY)
  if(group IN_LIST QM_APPLESS_TESTS)
    set(application None)
  elseif(group IN_LIST QM_CORE_TESTS)
    set(application Core)
  else()
    set(application Gui)
  endif()
  string(APPEND QM_TEST_GROUPS "QM_TEST_GROUP(${group}, ${application})\n")
  add_test(NAME ${group} COMMAND qm_tests ${group})
  set(environment "QML_IMPORT_PATH=${QT_QML_OUTPUT_DIRECTORY}")
  if(category STREQUAL "visual")
    set(labels "render;pixel")
  else()
    set(labels "behavior;${category}")
    list(APPEND environment "QT_QPA_PLATFORM=offscreen")
  endif()
  if(group IN_LIST QM_FIXED_DPI_TESTS)
    list(APPEND environment "QT_ENABLE_HIGHDPI_SCALING=0" "QT_SCALE_FACTOR=1")
  endif()
  set_tests_properties(${group} PROPERTIES ENVIRONMENT "${environment}" LABELS "${labels}")
  if(group IN_LIST QM_SERIAL_TESTS)
    set_tests_properties(${group} PROPERTIES RUN_SERIAL ON)
  endif()
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_groups.inc" CONTENT "${QM_TEST_GROUPS}")

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

set(VISUAL_SCENES carousel_focal menu_placement font_alignment floating_toolbar date_modes list_item date_locale text_field_form extended_fab fab_menu button_group material_shapes button action_progress elevation elevation_corners shape ripple ripple_phases loading_indicator_shapes tooltip wave_progress snackbar
    icon_artifacts icon_spec range_slider time_picker time_dialog search_view search_fullscreen search_progress app_bar_scroll pull_to_refresh swipe_to_dismiss
    focus_button focus_iconbutton focus_fab focus_chip chips command_surfaces
    split_button_input colorpicker_input focus_switch focus_checkbox focus_radio
    datepicker colorpicker colorpicker_button
    navigation_rail navigation_drawer bottom_sheet side_sheet scrollable carousel table motion_presets clock_presets clock_locales clock_widgets compositions compositions_details composition_pickers systemui)
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
  set_tests_properties(visual_${scene} PROPERTIES LABELS "render;smoke")
endforeach()
