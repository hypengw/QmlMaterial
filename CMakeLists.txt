cmake_minimum_required(VERSION 3.14)

set(QM_VERSION 0.1.1)
project(
  QmlMaterial
  VERSION ${QM_VERSION}
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_PATH_PREFIX ON)

option(QM_BUILD_EXAMPLE ON)

set(QM_ICON_FONT_URL
    ""
    CACHE
      STRING
      "default empty, there is an already embed file: qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2"
)

if("${QT_INSTALL_QML}" STREQUAL "")
  set(QT_INSTALL_QML "${CMAKE_INSTALL_LIBDIR}/qt6/qml")
endif()

set(QML_INSTALL_DIR
    "${QT_INSTALL_QML}"
    CACHE STRING "")
message("QML_INSTALL_DIR: ${QML_INSTALL_DIR}")

# check lto
include(CheckIPOSupported)
check_ipo_supported(RESULT is_ipo_supported OUTPUT lto_error)
if(is_ipo_supported AND NOT EMSCRIPTEN)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)
endif()

if(UNIX AND NOT APPLE)
  set(LINUX CACHE INTERNAL TRUE "")
endif()

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core Quick QuickControls2 Gui ShaderTools)
find_package(PkgConfig REQUIRED)
if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0004 NEW)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(LIBNAME qml_material)

set(SOURCES
    include/qml_material/helper.hpp
    include/qml_material/util/loggingcategory.hpp
    include/qml_material/scenegraph/geometry.h
    include/qml_material/scenegraph/rectangle_material.h
    include/qml_material/scenegraph/elevation_material.h
    include/qml_material/upstream/qquickmaterialtextcontainer_p.h
    src/scenegraph/geometry.cpp
    src/scenegraph/rectangle_material.cpp
    src/scenegraph/elevation_material.cpp
    src/kirigami/displayhint.cpp
    src/kirigami/toolbarlayout.cpp
    src/kirigami/toolbarlayoutdelegate.cpp
    src/kirigami/wheelhandler.cpp
    src/upstream/qquickmaterialtextcontainer.cpp
    src/enum.cpp
    src/theme.cpp
    src/helper.cpp
    src/util/corner.cpp
    src/util/input_block.cpp
    src/util/item_holder.cpp
    src/util/item_holder.cpp
    src/util/image.cpp
    src/util/qml_util.cpp
    src/util/page_context.cpp
    src/model/snake_model.cpp
    src/token/color.cpp
    src/token/icon.cpp
    src/token/token.cpp
    src/token/type_scale.cpp
    src/item/round_item.cpp
    src/item/state.cpp
    src/item/rectangle.cpp
    src/item/elevation.cpp)

set(QML_FILES
    qml/Token.qml
    qml/js/jsUtils.mjs
    qml/control/Button.qml
    qml/control/Control.qml
    qml/control/ComboBox.qml
    qml/control/ButtonGroup.qml
    qml/control/Dialog.qml
    qml/control/DialogButtonBox.qml
    qml/control/Drawer.qml
    qml/control/StackView.qml
    qml/control/SplitView.qml
    qml/control/Slider.qml
    qml/control/Menu.qml
    qml/control/MenuItem.qml
    qml/control/Pane.qml
    qml/control/Page.qml
    qml/control/PageIndicator.qml
    qml/control/Action.qml
    qml/control/ActionGroup.qml
    qml/control/MenuSeparator.qml
    qml/control/ToolSeparator.qml
    qml/control/ApplicationWindow.qml
    qml/control/Popup.qml
    qml/control/Switch.qml
    qml/control/TabBar.qml
    qml/control/TabButton.qml
    qml/control/ScrollBar.qml
    qml/control/ScrollIndicator.qml
    qml/controlEx/RailItem.qml
    qml/controlEx/SearchBar.qml
    qml/controlEx/ActionMenu.qml
    qml/controlEx/ActionToolBar.qml
    qml/controlEx/AppBar.qml
    qml/controlEx/BarItem.qml
    qml/controlEx/Card.qml
    qml/controlEx/AssistChip.qml
    qml/controlEx/FilterChip.qml
    qml/controlEx/InputChip.qml
    qml/controlEx/SuggestionChip.qml
    qml/controlEx/CircleProgressBar.qml
    qml/controlEx/CircularIndicator.qml
    qml/controlEx/LinearIndicator.qml
    qml/controlEx/SnakeBar.qml
    qml/controlEx/StandardDrawer.qml
    qml/controlEx/DrawerItem.qml
    qml/controlEx/IconButton.qml
    qml/controlEx/StandardIconButton.qml
    qml/controlEx/FAB.qml
    qml/component/Image.qml
    qml/component/Label.qml
    qml/component/FontMetrics.qml
    qml/component/Text.qml
    qml/component/TextEdit.qml
    qml/component/TextInput.qml
    qml/component/TextMetrics.qml
    qml/component/TextField.qml
    qml/componentEx/ColorRadio.qml
    qml/componentEx/Divider.qml
    qml/componentEx/DragHandle.qml
    qml/componentEx/IconLabel.qml
    qml/componentEx/Icon.qml
    qml/componentEx/IconSvg.qml
    qml/componentEx/Space.qml
    qml/componentEx/SdfRectangle.qml
    qml/componentEx/SnakeView.qml
    qml/delegate/ListItem.qml
    qml/flickable/Flickable.qml
    qml/flickable/VerticalFlickable.qml
    qml/flickable/VerticalGridView.qml
    qml/flickable/VerticalListView.qml
    qml/flickable/FlickablePane.qml
    qml/flickable/GridView.qml
    qml/flickable/ListView.qml
    qml/motion/FadeInMotion.qml
    qml/motion/FadeOutMotion.qml
    qml/motion/FadeInThroughMotion.qml
    qml/motion/FadeOutThroughMotion.qml
    qml/state/StateAppBar.qml
    qml/state/StateSnakeBar.qml
    qml/state/StateButton.qml
    qml/state/StateCard.qml
    qml/state/StateDragHandle.qml
    qml/state/StateDrawerItem.qml
    qml/state/StateTabButton.qml
    qml/state/StateListItem.qml
    qml/state/StateSlider.qml
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
    qml/impl/BoxShadow.qml
    qml/impl/CircleProgressShape.qml
    qml/impl/CircleIndicatorShape.qml
    qml/impl/LinearIndicatorShape.qml
    qml/impl/ListBusyFooter.qml
    qml/impl/Ripple2.qml
    qml/impl/RectangularGlow.qml
    qml/impl/CursorDelegate.qml
    qml/impl/SliderHandle.qml
    qml/impl/FloatingPlaceholderText.qml
    qml/impl/TextFieldEmbed.qml
    qml/impl/RoundClip.qml
    qml/impl/Elevation.qml
    qml/impl/ElevationRectangle.qml
    qml/impl/ElevationRectangleBorder.qml
    qml/impl/Shape.qml)

set(RESOURCES assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2)

set(QML_REG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/item"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/token"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/model"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/qml_material/util")

set(SHADER_DEFINES)

set(QML_MATERIAL_BUILD_TYPE SHARED)

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
  add_link_options("-sEXPORT_NAME=qml_material" -s FULL_ES3=1 -flto)
else()

  set(QM_ICON_FILL_0_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_0.woff2)
  set(QM_ICON_FILL_1_FONT_FILE
      assets/MaterialSymbolsRounded.wght_400.opsz_24.fill_1.woff2)

  if(UNIX)
    if(ANDROID)
      list(APPEND SOURCES src/platform/android/android.cpp)
      list(APPEND SHADER_DEFINES "GLES=1")
    else()
      list(APPEND SOURCES src/platform/linux/xdp.cpp)
    endif()
  elseif(WIN32)
    list(APPEND SOURCES src/platform/win/win.cpp)
  endif()
endif()

list(APPEND RESOURCES "${QM_ICON_FILL_0_FONT_FILE}"
     "${QM_ICON_FILL_1_FONT_FILE}")

set_source_files_properties(qml/Token.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
include_directories(include)

qt_add_qml_module(
  ${LIBNAME}
  URI
  Qcm.Material
  ${QML_MATERIAL_BUILD_TYPE}
  RESOURCE_PREFIX
  /
  QML_FILES
  ${QML_FILES}
  SOURCES
  ${SOURCES}
  RESOURCES
  ${RESOURCES}
  NO_PLUGIN_OPTIONAL)
add_library("${LIBNAME}::${LIBNAME}" ALIAS ${LIBNAME})

target_compile_features(${LIBNAME} PRIVATE cxx_std_20)
target_compile_definitions(${LIBNAME} PRIVATE QML_MATERIAL_EXPORT)
set_source_files_properties(
  "${CMAKE_CURRENT_BINARY_DIR}/${LIBNAME}_qmltyperegistrations.cpp"
  TARGET_DIRECTORY "${LIBNAME}" PROPERTIES INCLUDE_DIRECTORIES
                                           "${QML_REG_INCLUDE_DIRS}")

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
    "assets/shader/shadow_.frag"
    "assets/shader/rectangle.vert"
    "assets/shader/rectangle.frag"
    "assets/shader/shadow.vert"
    "assets/shader/shadow.frag"
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
  PUBLIC include
  PRIVATE "include/${LIBNAME}" "include/${LIBNAME}/kirigami"
          "include/${LIBNAME}/upstream" src)

set_target_properties(${LIBNAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_subdirectory(third_party)
if(QM_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick
         Qt6::QuickControls2 # QQuickAttachedPropertyPropagator
  PRIVATE material_color)

function(qt_install_qml_module target_)
  get_target_property(QML_TARGET_PATH ${target_} QT_QML_MODULE_TARGET_PATH)
  get_target_property(QML_TYEPINFO ${target_} QT_QML_MODULE_TYPEINFO)
  get_target_property(QML_PLUGIN_TARGET ${target_} QT_QML_MODULE_PLUGIN_TARGET)
  set(QML_TARGET_INSTALL_DIR "${QML_INSTALL_DIR}/${QML_TARGET_PATH}")

  install(TARGETS ${target_})

  install(TARGETS ${QML_PLUGIN_TARGET}
          LIBRARY DESTINATION ${QML_TARGET_INSTALL_DIR})

  install(FILES "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/qmldir"
                "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/${QML_TYEPINFO}
    " DESTINATION ${QML_TARGET_INSTALL_DIR})
endfunction()

qt_install_qml_module(${LIBNAME})
