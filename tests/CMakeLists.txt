if(NOT TARGET Qt6::Quick)
  find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml)
endif()
if(NOT TARGET Qt6::GuiPrivate)
  find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
endif()

enable_testing()

add_executable(hero_center_end hero_center_end.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp)
target_include_directories(hero_center_end PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hero_center_end PRIVATE Qt6::Core)
add_test(NAME hero_center_end COMMAND hero_center_end)

add_executable(fullscreen_layout fullscreen_layout.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp)
target_include_directories(fullscreen_layout PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(fullscreen_layout PRIVATE Qt6::Core)
add_test(NAME fullscreen_layout COMMAND fullscreen_layout)

add_executable(multi_browse_layout multi_browse_layout.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp)
target_include_directories(multi_browse_layout PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(multi_browse_layout PRIVATE Qt6::Core)
add_test(NAME multi_browse_layout COMMAND multi_browse_layout)

add_executable(uncontained_layout uncontained_layout.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp)
target_include_directories(uncontained_layout PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(uncontained_layout PRIVATE Qt6::Core)
add_test(NAME uncontained_layout COMMAND uncontained_layout)

add_executable(hero_start_layout hero_start_layout.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp)
target_include_directories(hero_start_layout PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(hero_start_layout PRIVATE Qt6::Core)
add_test(NAME hero_start_layout COMMAND hero_start_layout)

add_executable(carousel_engine_defaults carousel_engine_defaults.cpp)
target_include_directories(carousel_engine_defaults PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(carousel_engine_defaults PRIVATE Qt6::Core)
add_test(NAME carousel_engine_defaults COMMAND carousel_engine_defaults)

set(CAROUSEL_ENGINE_SOURCES
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_view.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_attached.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_strategies.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_keylines.cpp
  ${CMAKE_SOURCE_DIR}/src/carousel/carousel_scroll_strategy.cpp)
set(CAROUSEL_ENGINE_HEADERS
  ${CMAKE_SOURCE_DIR}/include/qml_material/carousel/carousel_view.hpp
  ${CMAKE_SOURCE_DIR}/include/qml_material/carousel/carousel_attached.hpp)

add_executable(set_current_index_sync
  set_current_index_sync.cpp
  ${CAROUSEL_ENGINE_SOURCES}
  ${CAROUSEL_ENGINE_HEADERS})
target_include_directories(set_current_index_sync PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(set_current_index_sync PRIVATE Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Qml Qt6::Quick Qt6::QuickPrivate)
set_target_properties(set_current_index_sync PROPERTIES AUTOMOC ON)
add_test(NAME set_current_index_sync COMMAND set_current_index_sync)

add_executable(model_shrink
  model_shrink.cpp
  ${CAROUSEL_ENGINE_SOURCES}
  ${CAROUSEL_ENGINE_HEADERS})
target_include_directories(model_shrink PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(model_shrink PRIVATE Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Qml Qt6::Quick Qt6::QuickPrivate)
set_target_properties(model_shrink PROPERTIES AUTOMOC ON)
add_test(NAME model_shrink COMMAND model_shrink)

add_executable(model_data_changed
  model_data_changed.cpp
  ${CAROUSEL_ENGINE_SOURCES}
  ${CAROUSEL_ENGINE_HEADERS})
target_include_directories(model_data_changed PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(model_data_changed PRIVATE Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Qml Qt6::Quick Qt6::QuickPrivate)
set_target_properties(model_data_changed PROPERTIES AUTOMOC ON)
add_test(NAME model_data_changed COMMAND model_data_changed)

add_executable(qm_grab qm_grab.cpp)
target_link_libraries(qm_grab PRIVATE Qt6::Core Qt6::Gui Qt6::GuiPrivate Qt6::Quick Qt6::Qml)
set_target_properties(qm_grab PROPERTIES AUTOMOC ON)

add_executable(toolbar_dynamic_visibility
  toolbar_dynamic_visibility.cpp
  ${CMAKE_SOURCE_DIR}/src/layout/toolbar_layout.cpp
  ${CMAKE_SOURCE_DIR}/src/layout/toolbar_layout_delegate.cpp
  ${CMAKE_SOURCE_DIR}/src/layout/toolbar_layout_delegate.hpp
  ${CMAKE_SOURCE_DIR}/include/qml_material/layout/toolbar_layout.hpp)
target_include_directories(toolbar_dynamic_visibility PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/layout)
target_compile_definitions(toolbar_dynamic_visibility PRIVATE
  TOOLBAR_DYNAMIC_QML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/toolbar_dynamic_visibility.qml")
target_link_libraries(toolbar_dynamic_visibility PRIVATE
  Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
set_target_properties(toolbar_dynamic_visibility PROPERTIES AUTOMOC ON)
add_test(NAME toolbar_dynamic_visibility COMMAND toolbar_dynamic_visibility)
set_tests_properties(toolbar_dynamic_visibility PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

set(VISUAL_SCENES button action_progress elevation shape ripple ripple_phases loading_indicator_shapes tooltip wave_progress
    focus_button focus_iconbutton focus_fab focus_chip
    focus_switch focus_checkbox focus_radio
    datepicker colorpicker colorpicker_button
    navigation_rail bottom_sheet flickable2 carousel table)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/out)
foreach(scene ${VISUAL_SCENES})
  add_test(
    NAME visual_${scene}
    COMMAND bash -c
      "LIBGL_ALWAYS_SOFTWARE=1 \
       QT_QPA_PLATFORM=\${QT_QPA_PLATFORM:-eglfs} \
       QML_IMPORT_PATH=${CMAKE_BINARY_DIR}/qml_modules \
       $<TARGET_FILE:qm_grab> \
         ${CMAKE_CURRENT_SOURCE_DIR}/scenes/${scene}.qml \
         ${CMAKE_CURRENT_BINARY_DIR}/out/visual_${scene}.png \
       && test -s ${CMAKE_CURRENT_BINARY_DIR}/out/visual_${scene}.png"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endforeach()
