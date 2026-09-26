cmake_minimum_required(VERSION 3.14)

set(QM_VERSION 0.4.1)
project(
  QmlMaterial
  VERSION ${QM_VERSION}
  LANGUAGES CXX)

# Enable MSVC multi-processor compilation
if(MSVC)
  add_compile_options(/MP)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)

option(QM_BUILD_EXAMPLE "" ${PROJECT_IS_TOP_LEVEL})
option(QM_BUILD_TESTS "" ${PROJECT_IS_TOP_LEVEL})

set(QM_ICON_FONT_URL
    ""
    CACHE
      STRING
      "default empty, there is an already embed file: qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2"
)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml_modules")
list(APPEND QML_DIRS "${QT_QML_OUTPUT_DIRECTORY}")

if(UNIX)
  include(GNUInstallDirs)

  if("${QT_INSTALL_QML}" STREQUAL "")
    set(QT_INSTALL_QML "${CMAKE_INSTALL_LIBDIR}/qt6/qml")
  endif()
endif()

set(QML_INSTALL_DIR
    "${QT_INSTALL_QML}"
    CACHE STRING "")
message("QML_INSTALL_DIR: ${QML_INSTALL_DIR}")

if(UNIX AND NOT APPLE)
  set(LINUX CACHE INTERNAL TRUE "")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Quick Gui Qml ShaderTools)
# Qt < ~6.9 often exposes Qt6::QuickPrivate via Quick; Qt 6.9+ needs an
# explicit private package (aqt 6.8.3 CI has no Qt6QuickPrivateConfig).
if(NOT TARGET Qt6::QuickPrivate)
  find_package(Qt6 REQUIRED COMPONENTS QuickPrivate)
endif()

if(UNIX AND NOT ANDROID AND NOT EMSCRIPTEN)
  find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

find_package(PkgConfig REQUIRED)
if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0004 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBNAME qml_material)

set(SOURCES
    include/qml_material/core/attached_property.hpp
    include/qml_material/core/helper.hpp
    include/qml_material/token/badge.hpp
    include/qml_material/token/duration.hpp
    include/qml_material/token/combo_box.hpp
    include/qml_material/token/text_field.hpp
    include/qml_material/util/loggingcategory.hpp
    include/qml_material/math/scalar.hpp
    include/qml_material/math/gaussian.hpp
    include/qml_material/math/rrect.hpp
    include/qml_material/math/shadow_light.hpp
    include/qml_material/scenegraph/geometry.h
    include/qml_material/scenegraph/rectangle_material.h
    include/qml_material/scenegraph/shadow_material.h
    include/qml_material/scenegraph/blur_mask_material.h
    include/qml_material/scenegraph/texture_cache.h
    include/qml_material/scenegraph/mesh/quad.hpp
    include/qml_material/scenegraph/mesh/nine_patch.hpp
    include/qml_material/scenegraph/mesh/corner_fan.hpp
    src/math/gaussian.cpp
    src/math/shadow_light.cpp
    src/scenegraph/geometry.cpp
    src/scenegraph/skia/skia_shadow.cpp
    src/scenegraph/rectangle_material.cpp
    src/scenegraph/shadow_material.cpp
    src/scenegraph/blur_mask_material.cpp
    src/scenegraph/texture_cache.cpp
    src/core/enum.cpp
    src/core/attached_property.cpp
    src/core/helper.cpp
    src/style/theme.cpp
    src/anim/circular_indicator_updator.cpp
    src/anim/linear_indicator_updator.cpp
    src/anim/loading_indicator_updator.cpp
    src/util/corner.cpp
    src/util/pool.cpp
    src/util/image.cpp
    src/util/qml_util.cpp
    src/util/hct.cpp
    src/control/page_context.cpp
    src/control/color_picker.cpp
    src/control/application_window.cpp
    include/qml_material/control/application_window.hpp
    src/control/control.cpp
    src/control/slider.cpp
    src/control/scroll_indicator.cpp
    src/control/scroll_bar.cpp
    include/qml_material/control/scroll_bar.hpp
    include/qml_material/control/scroll_indicator.hpp
    include/qml_material/control/slider.hpp
    src/control/label.cpp
    src/control/combo_box.cpp
    include/qml_material/control/combo_box.hpp
    src/control/text_field.cpp
    include/qml_material/control/text_field.hpp
    include/qml_material/control/label.hpp
    src/util/text_control_background.cpp
    include/qml_material/util/text_control_background.hpp
    src/control/progress_bar.cpp
    src/control/page_indicator.cpp
    include/qml_material/control/page_indicator.hpp
    include/qml_material/control/progress_bar.hpp
    include/qml_material/control/busy_indicator.hpp
    include/qml_material/control/tool_separator.hpp
    src/control/abstract_button.cpp
    src/control/check_box.cpp
    src/control/switch.cpp
    src/control/tab_bar.cpp
    src/control/container.cpp
    src/control/split_view.cpp
    src/control/split_layout.cpp
    include/qml_material/control/split_view.hpp
    src/control/segmented_button_group.cpp
    include/qml_material/control/check_box.hpp
    include/qml_material/control/radio_button.hpp
    include/qml_material/control/switch.hpp
    include/qml_material/control/item_delegate.hpp
    include/qml_material/control/tab_button.hpp
    include/qml_material/control/tab_bar.hpp
    include/qml_material/control/container.hpp
    include/qml_material/control/segmented_button_group.hpp
    src/control/icon_spec.cpp
    src/control/action.cpp
    src/control/action_group.cpp
    src/control/button_group.cpp
    src/control/dialog_button_box.cpp
    include/qml_material/control/dialog_button_box.hpp
    include/qml_material/control/action.hpp
    include/qml_material/control/action_group.hpp
    include/qml_material/control/button_group.hpp
    include/qml_material/control/abstract_button.hpp
    include/qml_material/control/button.hpp
    include/qml_material/control/icon_spec.hpp
    src/control/panel.cpp
    src/control/page.cpp
    src/control/page_stack.cpp
    src/control/page_stack_entries.cpp
    src/control/page_stack_entries_p.hpp
    include/qml_material/control/page_stack.hpp
    include/qml_material/control/page.hpp
    include/qml_material/control/panel.hpp
    include/qml_material/control/tool_bar.hpp
    include/qml_material/control/control.hpp
    src/control/popup.cpp
    src/control/dialog.cpp
    src/control/menu.cpp
    include/qml_material/control/menu.hpp
    include/qml_material/control/dialog.hpp
    include/qml_material/control/popup.hpp
    src/control/overlay.cpp
    include/qml_material/control/overlay.hpp
    src/control/popup_presenter.cpp
    include/qml_material/control/popup_presenter.hpp
    src/control/adaptive_presenter.cpp
    include/qml_material/control/adaptive_presenter.hpp
    src/control/tool_tip.cpp
    src/control/tool_tip_popup.cpp
    include/qml_material/control/tool_tip_popup.hpp
    src/control/drawer.cpp
    include/qml_material/control/drawer.hpp
    include/qml_material/control/tool_tip.hpp
    src/layout/width_provider.cpp
    src/layout/toolbar_layout.cpp
    src/layout/toolbar_layout_delegate.cpp
    src/layout/toolbar_layout_delegate.hpp
    include/qml_material/layout/toolbar_layout.hpp
    src/path/path_wave.cpp
    src/path/path_arc_wave.cpp
    include/qml_material/path/path_wave.hpp
    include/qml_material/path/path_arc_wave.hpp
    src/scrollable/flickable.cpp
    include/qml_material/scrollable/flickable.hpp
    include/qml_material/scrollable/scroll_motion.hpp
    src/input/wheel_handler.cpp
    src/input/nested_scroll.cpp
    src/input/scroll_participant.cpp
    include/qml_material/input/nested_scroll.hpp
    include/qml_material/input/wheel_handler.hpp
    src/carousel/carousel_attached.cpp
    src/carousel/carousel_view.cpp
    src/carousel/carousel_strategies.cpp
    src/carousel/carousel_keylines.cpp
    src/carousel/carousel_scroll_strategy.cpp
    include/qml_material/carousel/carousel_attached.hpp
    include/qml_material/carousel/carousel_keylines.hpp
    include/qml_material/carousel/carousel_scroll_strategy.hpp
    include/qml_material/carousel/carousel_strategy.hpp
    include/qml_material/carousel/carousel_view.hpp
    src/model/palette_model.cpp
    src/model/item_source.cpp
    include/qml_material/model/item_source.hpp
    src/model/item_model_source.cpp
    include/qml_material/model/item_model_source.hpp
    src/model/lazy_row.cpp
    include/qml_material/model/lazy_row.hpp
    src/view/delegate_recycler.cpp
    src/view/lazy_list.cpp
    include/qml_material/view/lazy_list.hpp
    src/model/combo_box_model.cpp
    include/qml_material/model/combo_box_model.hpp
    src/model/calendar_month_model.cpp
    include/qml_material/model/calendar_month_model.hpp
    src/model/header_data_model.cpp
    include/qml_material/model/header_data_model.hpp
    src/model/header_list_model.cpp
    include/qml_material/model/header_list_model.hpp
    src/model/header_model.cpp
    include/qml_material/model/header_model.hpp
    src/control/header_view.cpp
    include/qml_material/control/header_view.hpp
    src/model/appstream_release_model.cpp
    src/model/snake_model.cpp
    src/token/color.cpp
    src/token/icon.cpp
    src/token/token.cpp
    src/token/type_scale.cpp
    src/item/item_proxy.cpp
    src/item/round_item.cpp
    src/item/state.cpp
    src/item/rectangle.cpp
    src/item/rrect_shadow.cpp
    src/item/blur_mask.cpp
    src/item/input_block.cpp
    src/dialog/file_dialog.cpp
    src/dialog/file_dialog_p.hpp
    include/qml_material/dialog/file_dialog.hpp)

set(QML_FILES
    qml/Token.qml
    qml/Util.qml
    qml/control/Button.qml
    qml/control/Control.qml
    qml/control/Container.qml
    qml/control/SegmentedButton.qml
    qml/control/SegmentedButtonGroup.qml
    qml/control/ComboBox.qml
    qml/control/CheckBox.qml
    qml/control/RadioButton.qml
    qml/control/Dialog.qml
    qml/control/DialogButtonBox.qml
    qml/control/Drawer.qml
    qml/control/PageStack.qml
    qml/control/SplitView.qml
    qml/control/Slider.qml
    qml/control/SliderM2.qml
    qml/control/Menu.qml
    qml/control/MenuItem.qml
    qml/control/Pane.qml
    qml/control/Page.qml
    qml/control/PageIndicator.qml
    qml/control/Carousel.qml
    qml/control/MenuSeparator.qml
    qml/control/ApplicationWindow.qml
    qml/control/Popup.qml
    qml/control/BottomSheet.qml
    qml/control/Switch.qml
    qml/control/PlainToolTip.qml
    qml/control/TabBar.qml
    qml/control/TabButton.qml
    qml/control/TableViewDelegate.qml
    qml/control/ScrollBar.qml
    qml/control/ScrollIndicator.qml
    qml/control/ItemDelegate.qml
    qml/control/HorizontalHeaderView.qml
    qml/control/HorizontalHeaderViewDelegate.qml
    qml/control/VerticalHeaderView.qml
    qml/control/VerticalHeaderViewDelegate.qml
    qml/control/Label.qml
    qml/control/BusyIndicator.qml
    qml/control/BusyIndicatorShape.qml
    qml/component/MState.qml
    qml/component/Image.qml
    qml/component/Loader.qml
    qml/component/FontMetrics.qml
    qml/component/Text.qml
    qml/component/TextEdit.qml
    qml/component/TextInput.qml
    qml/component/TextMetrics.qml
    qml/component/TextField.qml
    qml/component/appbar/ActionMenu.qml
    qml/component/appbar/SearchBar.qml
    qml/component/appbar/ActionToolBar.qml
    qml/component/appbar/SnakeBar.qml
    qml/component/appbar/AppBar.qml
    qml/component/appbar/BarItem.qml
    qml/component/button/BusyIconButton.qml
    qml/component/button/BusyButton.qml
    qml/component/button/FAB.qml
    qml/component/button/IconButton.qml
    qml/component/button/StandardIconButton.qml
    qml/component/button/SmallIconButton.qml
    qml/component/button/RadioIndicatorShape.qml
    qml/component/button/SplitButton.qml
    qml/component/button/SplitButtonIndicator.qml
    qml/component/bottomsheet/SheetActionBar.qml
    qml/component/checkbox/CheckShape.qml
    qml/component/card/Card.qml
    qml/component/carousel/CarouselItem.qml
    qml/component/carousel/CarouselImageDelegate.qml
    qml/component/chip/AssistChip.qml
    qml/component/chip/EmbedChip.qml
    qml/component/chip/FilterChip.qml
    qml/component/chip/InputChip.qml
    qml/component/chip/SuggestionChip.qml
    qml/component/navigation/RailItem.qml
    qml/component/navigation/NavigationRail.qml
    qml/component/navigation/StandardDrawer.qml
    qml/component/navigation/DrawerItem.qml
    qml/component/navigation/DrawerSubheader.qml
    qml/component/progressindicator/CircularIndicator.qml
    qml/component/progressindicator/CircleIndicatorShape.qml
    qml/component/progressindicator/CircleIndicatorWaveShape.qml
    qml/component/progressindicator/LinearIndicator.qml
    qml/component/progressindicator/LinearIndicatorShape.qml
    qml/component/progressindicator/LinearIndicatorWaveShape.qml
    qml/component/textfield/FloatingPlaceholderText.qml
    qml/component/textfield/FilledTextFieldShape.qml
    qml/component/textfield/OutlineTextFieldShape.qml
    qml/component/extra/ActionLabel.qml
    qml/component/extra/Badge.qml
    qml/component/extra/ColorRadio.qml
    qml/component/extra/Divider.qml
    qml/component/extra/AutoDivider.qml
    qml/component/extra/DragHandle.qml
    qml/component/extra/IconLabel.qml
    qml/component/extra/Icon.qml
    qml/component/extra/IconView.qml
    qml/component/extra/IconSvg.qml
    qml/component/extra/Space.qml
    qml/component/extra/SdfRectangle.qml
    qml/component/extra/SnakeView.qml
    qml/component/extra/DialogHeader.qml
    qml/component/datepicker/DatePicker.qml
    qml/component/datepicker/DatePickerDialog.qml
    qml/component/datepicker/DateTextField.qml
    qml/component/colorpicker/ColorWheel.qml
    qml/component/colorpicker/ColorTriangle.qml
    qml/component/colorpicker/HueChannelSlider.qml
    qml/component/colorpicker/ChannelSlider.qml
    qml/component/colorpicker/ColorChannelRow.qml
    qml/component/colorpicker/ColorPicker.qml
    qml/component/colorpicker/ColorPickerDialog.qml
    qml/component/colorpicker/ColorPickerButton.qml
    qml/component/extra/NumberStepper.qml
    qml/component/extra/StepperInput.qml
    qml/component/extra/RichToolTip.qml
    qml/component/extra/FocusIndicator.qml
    qml/delegate/ListItem.qml
    qml/delegate/ListGridBaseDelegate.qml
    qml/scrollable/Flickable.qml
    qml/scrollable/VerticalFlickable.qml
    qml/scrollable/PageVerticalFlickable.qml
    qml/scrollable/PageVerticalListView.qml
    qml/scrollable/PageHorizontalListView.qml
    qml/scrollable/VerticalGridView.qml
    qml/scrollable/VerticalListView.qml
    qml/scrollable/HorizontalListView.qml
    qml/scrollable/FlickablePane.qml
    qml/scrollable/TableView.qml
    qml/scrollable/GridView.qml
    qml/scrollable/ListView.qml
    qml/motion/FadeInMotion.qml
    qml/motion/FadeOutMotion.qml
    qml/motion/FadeInThroughMotion.qml
    qml/motion/FadeOutThroughMotion.qml
    qml/state/StateAppBar.qml
    qml/state/StateBottomSheet.qml
    qml/state/StateSnakeBar.qml
    qml/state/StateButton.qml
    qml/state/StateSplitButton.qml
    qml/state/StateSplitButtonIndicator.qml
    qml/state/StateSegmentedButton.qml
    qml/state/StateCard.qml
    qml/state/StateCarouselItem.qml
    qml/state/StateDragHandle.qml
    qml/state/StateDrawerItem.qml
    qml/state/StateTabButton.qml
    qml/state/StateTableViewDelegate.qml
    qml/state/StateListItem.qml
    qml/state/StateSlider.qml
    qml/state/StateSliderM2.qml
    qml/state/StateSwitch.qml
    qml/state/StateIconButton.qml
    qml/state/StateStandardIconButton.qml
    qml/state/StateFAB.qml
    qml/state/StateAssistChip.qml
    qml/state/StateFilterChip.qml
    qml/state/StateInputChip.qml
    qml/state/StateSuggestionChip.qml
    qml/state/StateBarItem.qml
    qml/state/StateSearchBar.qml
    qml/state/StateTextField.qml
    qml/state/StateRailItem.qml
    qml/state/StateEmbedChip.qml
    qml/state/StateComboBox.qml
    qml/state/StateMenuItem.qml
    qml/state/StateRadioButton.qml
    qml/state/StateCheckBox.qml
    qml/component/basic/Ripple.qml
    qml/component/basic/RippleShape.qml
    qml/component/basic/RippleSkia.qml
    qml/delegate/CursorDelegate.qml
    qml/delegate/SliderHandle.qml
    qml/delegate/SliderHandleM2.qml
    qml/component/TextFieldEmbed.qml
    qml/component/basic/RoundClip.qml
    qml/component/basic/Elevation.qml
    qml/component/Rectangle.qml
    qml/component/basic/ElevationRectangle.qml
    qml/component/Shape.qml
    qml/component/path/RoundPath.qml
    qml/component/path/RectPath.qml
    qml/component/ListBusyFooter.qml
    qml/component/Changelog.qml
    qml/component/ChangelogDialog.qml
    qml/component/helper/ItemHolder.qml
    qml/container/PageContainer.qml)

set(RESOURCES assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2)

set(QML_REG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/view"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/anim"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/style"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/control"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/layout"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/path"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/scrollable"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/input"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/item"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/token"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/model"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/util"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/dialog"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/carousel")

set(LAYOUT_SOURCES
    include/qml_material/layout/layout.hpp
    include/qml_material/layout/layout_container.hpp
    src/layout/layout.cpp
    src/layout/layout_container.cpp
    src/layout/layout_item_adapter.cpp
    src/layout/layout_item_adapter_p.hpp)

set(LAYOUT_REG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/layout")

set(SHADER_DEFINES)

set(QML_MATERIAL_BUILD_TYPE
    SHARED
    CACHE STRING "")

set(QM_ICON_FILL_0_FONT_FILE)
set(QM_ICON_FILL_1_FONT_FILE)

if(EMSCRIPTEN)
  list(APPEND SOURCES src/platform/wasm/wasm.cpp)
  list(APPEND SHADER_DEFINES "GLES=1")
  set(QM_ICON_FILL_0_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.ttf)
  set(QM_ICON_FILL_1_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.ttf)

  set(QML_MATERIAL_BUILD_TYPE STATIC)
  add_compile_options(-flto -oz)
  add_link_options("-sEXPORT_NAME=qml_material" "-sFULL_ES3=1" -flto -lembind)
else()

  set(QM_ICON_FILL_0_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.woff2)
  set(QM_ICON_FILL_1_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.woff2)

  if(UNIX)
    if(ANDROID)
      list(APPEND SOURCES src/platform/android/android.cpp)
      list(APPEND SHADER_DEFINES "GLES=1")
      list(APPEND SOURCES src/dialog/file_dialog_stub.cpp)
    else()
      list(APPEND SOURCES src/platform/linux/xdp.cpp
           src/platform/linux/file_dialog_xdp.cpp
           src/platform/linux/file_dialog_xdp.hpp
           src/platform/linux/open_uri_xdp.cpp)
    endif()
  elseif(WIN32)
    set(QM_ICON_FILL_0_FONT_FILE
        assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.ttf)
    set(QM_ICON_FILL_1_FONT_FILE
        assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.ttf)

    set(QML_MATERIAL_BUILD_TYPE STATIC)
    list(APPEND SOURCES src/platform/win/win.cpp)
    list(APPEND SOURCES src/dialog/file_dialog_stub.cpp)
  endif()
endif()

if(EMSCRIPTEN)
  list(APPEND SOURCES src/dialog/file_dialog_stub.cpp)
endif()

list(APPEND RESOURCES "${QM_ICON_FILL_0_FONT_FILE}"
     "${QM_ICON_FILL_1_FONT_FILE}")

set_source_files_properties(qml/Token.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/Util.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
include_directories(include)

set(LAYOUT_LIBNAME qml_material_layouts)
qt_add_qml_module(
  ${LAYOUT_LIBNAME}
  URI
  Qcm.Material.Layouts
  ${QML_MATERIAL_BUILD_TYPE}
  DEPENDENCIES
  QtQuick
  OUTPUT_TARGETS
  qml_material_layout_targets
  RESOURCE_PREFIX
  /
  SOURCES
  ${LAYOUT_SOURCES}
  NO_PLUGIN_OPTIONAL)
add_library("${LIBNAME}::${LAYOUT_LIBNAME}" ALIAS ${LAYOUT_LIBNAME})

target_compile_features(${LAYOUT_LIBNAME} PRIVATE cxx_std_20)
target_compile_definitions(${LAYOUT_LIBNAME} PRIVATE QML_MATERIAL_EXPORT)
if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  target_compile_definitions(${LAYOUT_LIBNAME} PUBLIC QML_MATERIAL_STATIC_DEFINE)
endif()
set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/${LAYOUT_LIBNAME}_qmltyperegistrations.cpp"
  TARGET_DIRECTORY "${LAYOUT_LIBNAME}" PROPERTIES INCLUDE_DIRECTORIES
                                                   "${LAYOUT_REG_INCLUDE_DIRS}")
set_target_properties(${LAYOUT_LIBNAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_include_directories(
  ${LAYOUT_LIBNAME}
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE src/layout)
target_link_libraries(
  ${LAYOUT_LIBNAME}
  PUBLIC Qt6::Core Qt6::Qml Qt6::Quick
  PRIVATE Qt6::QuickPrivate)

qt_add_qml_module(
  ${LIBNAME}
  URI
  Qcm.Material
  ${QML_MATERIAL_BUILD_TYPE}
  IMPORTS
  QtQuick.Layouts
  QtQuick.Shapes
  Qcm.Material.Layouts
  DEPENDENCIES
  QtQuick
  OUTPUT_TARGETS
  qml_material_targets
  RESOURCE_PREFIX
  /
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES})
add_library("${LIBNAME}::${LIBNAME}" ALIAS ${LIBNAME})

target_compile_features(${LIBNAME} PUBLIC cxx_std_20)
target_compile_definitions(${LIBNAME} PRIVATE QML_MATERIAL_EXPORT)
if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  target_compile_definitions(${LIBNAME} PUBLIC QML_MATERIAL_STATIC_DEFINE)
endif()
set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}_qmltyperegistrations.cpp"
  TARGET_DIRECTORY "${LIBNAME}" PROPERTIES INCLUDE_DIRECTORIES
                                           "${QML_REG_INCLUDE_DIRS}")

# QT_QMLCACHEGEN_ARGUMENTS "--only-bytecode"
set_target_properties(${LIBNAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

set(TOKEN_COMPILE_DEFINITIONS
    QM_ICON_FONT_URL="${QM_ICON_FONT_URL}"
    QM_ICON_FILL_0_FONT_URL="qrc:/Qcm/Material/${QM_ICON_FILL_0_FONT_FILE}"
    QM_ICON_FILL_1_FONT_URL="qrc:/Qcm/Material/${QM_ICON_FILL_1_FONT_FILE}"
    QM_VERSION="${QM_VERSION}")

set_source_files_properties(
  src/token/token.cpp PROPERTIES COMPILE_DEFINITIONS
                                 "${TOKEN_COMPILE_DEFINITIONS}")

set(SHADER_GLSLS "assets/shader/common.glsl" "assets/shader/sdf.glsl")
set(SHADER_SOURCES
    "assets/shader/default.vert"
    "assets/shader/icon_tint.frag"
    "assets/shader/round_clip.frag"
    "assets/shader/sdf_rectangle.frag"
    "assets/shader/rectangle.vert"
    "assets/shader/rectangle.frag"
    "assets/shader/shadow.vert"
    "assets/shader/shadow.frag"
    "assets/shader/blur_mask.vert"
    "assets/shader/blur_mask.frag"
    "assets/shader/ripple.frag"
    "assets/shader/rect_glow.frag")
set(SHADER_OPT BATCHABLE OPTIMIZED)
if(CMAKE_BUILD_TYPE MATCHES Debug)
  list(APPEND SHADER_OPT DEBUGINFO)
endif()
if(NOT WIN32)
  list(APPEND SHADER_OPT NOHLSL)
endif()

qt_add_shaders(
  ${LIBNAME}
  "shader"
  PREFIX
  "/Qcm/Material"
  GLSL
  "150,140,130,330,300es"
  ${SHADER_OPT}
  DEFINES
  ${SHADER_DEFINES}
  FILES
  ${SHADER_SOURCES})

foreach(file ${SHADER_SOURCES})
  set(output_file ".qsb/${file}.qsb")
  add_custom_command(
    OUTPUT ${output_file}
    DEPENDS ${SHADER_GLSLS}
    APPEND)
endforeach()

target_include_directories(
  ${LIBNAME}
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE "include/${LIBNAME}" src)

add_subdirectory(third_party)
target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick
         Qt6::QuickPrivate # QQuickStateGroup
  PRIVATE material_color Qt6::QmlModelsPrivate)

if(UNIX AND NOT ANDROID AND NOT EMSCRIPTEN)
  target_link_libraries(${LIBNAME} PRIVATE Qt6::DBus)
endif()

if(QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  add_subdirectory(qml)
  target_link_libraries("${LIBNAME}plugin"
                        PUBLIC qml_material_qml_imports
                               "${LAYOUT_LIBNAME}plugin")
endif()

if(QM_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()
if(QM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(NOT QML_MATERIAL_BUILD_TYPE MATCHES STATIC)
  include(CMakePackageConfigHelpers)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config-version.cmake"
    VERSION ${QM_VERSION}
    COMPATIBILITY SameMajorVersion)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${LIBNAME}-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}-config-version.cmake"
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

  install(
    DIRECTORY include/${LIBNAME}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp")

  install(TARGETS ${qml_material_targets} ${qml_material_layout_targets}
          EXPORT ${LIBNAME}-targets)
  install(TARGETS ${LIBNAME} EXPORT ${LIBNAME}-targets)
  install(TARGETS ${LAYOUT_LIBNAME} EXPORT ${LIBNAME}-targets)
  install(
    EXPORT ${LIBNAME}-targets
    FILE ${LIBNAME}-targets.cmake
    NAMESPACE ${LIBNAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${LIBNAME})

  function(qt_install_qml_module target_)
    get_target_property(QML_TARGET_PATH ${target_} QT_QML_MODULE_TARGET_PATH)
    get_target_property(QML_TYEPINFO ${target_} QT_QML_MODULE_TYPEINFO)
    get_target_property(QML_PLUGIN_TARGET ${target_}
                        QT_QML_MODULE_PLUGIN_TARGET)
    set(QML_TARGET_INSTALL_DIR "${QML_INSTALL_DIR}/${QML_TARGET_PATH}")

    install(TARGETS ${QML_PLUGIN_TARGET}
            LIBRARY DESTINATION ${QML_TARGET_INSTALL_DIR})

    if(ARGC GREATER 1)
      install(
        DIRECTORY ${ARGV1}
        DESTINATION ${QML_TARGET_INSTALL_DIR}
        FILES_MATCHING
        PATTERN "*.qml"
        PATTERN "*.mjs")
    endif()

    install(
      FILES "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/qmldir"
            "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/${QML_TYEPINFO}"
      DESTINATION ${QML_TARGET_INSTALL_DIR})
  endfunction()

  qt_install_qml_module(${LIBNAME} qml)
  qt_install_qml_module(${LAYOUT_LIBNAME})

endif()
