cmake_minimum_required(VERSION 3.14)

set(QM_VERSION 0.1.6)
project(
  QmlMaterial
  VERSION ${QM_VERSION}
  LANGUAGES CXX)

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

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Gui ShaderTools)

if(UNIX
   AND NOT ANDROID
   AND NOT EMSCRIPTEN)
  find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()

if(Qt6Quick_VERSION VERSION_GREATER_EQUAL 6.10)
  find_package(Qt6 REQUIRED COMPONENTS QuickPrivate)
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
    include/qml_material/core/helper.hpp
    include/qml_material/token/badge.hpp
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
    src/control/popup.cpp
    src/layout/width_provider.cpp
    src/layout/toolbar_layout.cpp
    src/layout/toolbar_layout_delegate.cpp
    src/layout/toolbar_layout_delegate.hpp
    include/qml_material/layout/toolbar_layout.hpp
    src/path/path_wave.cpp
    src/path/path_arc_wave.cpp
    include/qml_material/path/path_wave.hpp
    include/qml_material/path/path_arc_wave.hpp
    src/flickable/flickable.cpp
    include/qml_material/flickable/flickable.hpp
    src/input/wheel_handler.cpp
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
    qml/control/SegmentedButton.qml
    qml/control/SegmentedButtonGroup.qml
    qml/control/Control.qml
    qml/control/ComboBox.qml
    qml/control/ButtonGroup.qml
    qml/control/CheckBox.qml
    qml/control/RadioButton.qml
    qml/control/Dialog.qml
    qml/control/DialogButtonBox.qml
    qml/control/Drawer.qml
    qml/control/StackView.qml
    qml/control/SplitView.qml
    qml/control/Slider.qml
    qml/control/SliderM2.qml
    qml/control/Menu.qml
    qml/control/MenuItem.qml
    qml/control/Pane.qml
    qml/control/Page.qml
    qml/control/PageIndicator.qml
    qml/control/Carousel.qml
    qml/control/Action.qml
    qml/control/ActionGroup.qml
    qml/control/MenuSeparator.qml
    qml/control/ToolSeparator.qml
    qml/control/ApplicationWindow.qml
    qml/control/Popup.qml
    qml/control/BottomSheet.qml
    qml/control/Switch.qml
    qml/control/ToolTip.qml
    qml/control/TabBar.qml
    qml/control/TabButton.qml
    qml/control/TableViewDelegate.qml
    qml/control/ScrollBar.qml
    qml/control/ScrollIndicator.qml
    qml/control/ItemDelegate.qml
    qml/control/HorizontalHeaderView.qml
    qml/control/HorizontalHeaderViewDelegate.qml
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
    qml/flickable/Flickable.qml
    qml/flickable/VerticalFlickable.qml
    qml/flickable/VerticalGridView.qml
    qml/flickable/VerticalListView.qml
    qml/flickable/HorizontalListView.qml
    qml/flickable/FlickablePane.qml
    qml/flickable/TableView.qml
    qml/flickable/GridView.qml
    qml/flickable/ListView.qml
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
    qml/component/helper/ItemHolder.qml
    qml/container/PageContainer.qml)

set(RESOURCES assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2)

set(QML_REG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/anim"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/style"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/control"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/layout"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/path"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/flickable"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/input"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/item"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/token"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/model"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/util"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/dialog"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/carousel")

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
      list(
        APPEND
        SOURCES
        src/platform/linux/xdp.cpp
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

qt_add_qml_module(
  ${LIBNAME}
  URI
  Qcm.Material
  ${QML_MATERIAL_BUILD_TYPE}
  IMPORTS
  QtQuick.Layouts
  QtQuick.Shapes
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
  ${RESOURCES}
  NO_GENERATE_PLUGIN_SOURCE
  NO_PLUGIN_OPTIONAL)
add_library("${LIBNAME}::${LIBNAME}" ALIAS ${LIBNAME})

target_sources("${LIBNAME}plugin" PRIVATE src/plugin.cpp)

target_compile_features(${LIBNAME} PRIVATE cxx_std_20)
target_compile_definitions(${LIBNAME} PRIVATE QML_MATERIAL_EXPORT)
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
  "300es,330"
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
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}include>"
  PRIVATE "include/${LIBNAME}" src)

add_subdirectory(third_party)
if(QM_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()
if(QM_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick
         Qt6::QuickControls2 # QQuickAttachedPropertyPropagator
         Qt6::QuickPrivate # QQuickStateGroup
  PRIVATE material_color)

if(UNIX
   AND NOT ANDROID
   AND NOT EMSCRIPTEN)
  target_link_libraries(${LIBNAME} PRIVATE Qt6::DBus)
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

  install(TARGETS ${qml_material_targets} EXPORT ${LIBNAME}-targets)
  install(TARGETS ${LIBNAME} EXPORT ${LIBNAME}-targets)
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

    install(
      DIRECTORY qml
      DESTINATION ${QML_TARGET_INSTALL_DIR}
      FILES_MATCHING
      PATTERN "*.qml"
      PATTERN "*.mjs")

    install(
      FILES "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/qmldir"
            "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/${QML_TYEPINFO}"
      DESTINATION ${QML_TARGET_INSTALL_DIR})
  endfunction()

  qt_install_qml_module(${LIBNAME})

endif()
