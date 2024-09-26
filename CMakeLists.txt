cmake_minimum_required(VERSION 3.14)

project(
  QmlMaterial
  VERSION 0.1.1
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD
    20)
set(CMAKE_CXX_STANDARD_REQUIRED
    ON)
set(CMAKE_POSITION_INDEPENDENT_CODE
    ON)
set(CMAKE_AUTOMOC
    ON)

# check lto
include(CheckIPOSupported)
check_ipo_supported(
  RESULT is_ipo_supported
  OUTPUT lto_error)
if(is_ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION
      ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG
      OFF)
endif()

if(UNIX
   AND NOT APPLE)
  set(LINUX
      CACHE INTERNAL TRUE "")
endif()

include(GNUInstallDirs)

find_package(
  Qt6 REQUIRED
  COMPONENTS Core Quick QuickControls2 Gui ShaderTools)
find_package(PkgConfig REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      Release)
endif()

set(LIBNAME
    qml_material)

set(SOURCES
    include/qml_material/util.h
    include/qml_material/theme.h
    include/qml_material/color.h
    include/qml_material/corner.h
    include/qml_material/icon.h
    include/qml_material/input_block.h
    include/qml_material/item_holder.h
    include/qml_material/enum.h
    include/qml_material/round_item.h
    include/qml_material/state.h
    include/qml_material/helper.h
    include/qml_material/type_scale.h
    include/qml_material/token.h
    include/qml_material/snake_model.h
    include/qml_material/loggingcategory.h
    include/qml_material/kirigami/displayhint.h
    include/qml_material/kirigami/toolbarlayout.h
    include/qml_material/kirigami/toolbarlayoutdelegate.h
    include/qml_material/kirigami/wheelhandler.h
    src/kirigami/displayhint.cpp
    src/kirigami/toolbarlayout.cpp
    src/kirigami/toolbarlayoutdelegate.cpp
    src/kirigami/wheelhandler.cpp
    src/color.cpp
    src/corner.cpp
    src/icon.cpp
    src/input_block.cpp
    src/item_holder.cpp
    src/image.cpp
    src/util.cpp
    src/state.cpp
    src/theme.cpp
    src/helper.cpp
    src/snake_model.cpp
    src/token.cpp
    src/type_scale.cpp
    src/round_item.cpp)
set(QML_FILES
    qml/js/utils.mjs
    qml/ActionMenu.qml
    qml/ActionToolBar.qml
    qml/AppBar.qml
    qml/Button.qml
    qml/BarItem.qml
    qml/Control.qml
    qml/ComboBox.qml
    qml/CircleProgress.qml
    qml/Dialog.qml
    qml/Divider.qml
    qml/Drawer.qml
    qml/DrawerItem.qml
    qml/Token.qml
    qml/IconLabel.qml
    qml/IconButton.qml
    qml/FAB.qml
    qml/Card.qml
    qml/CircularIndicator.qml
    qml/Menu.qml
    qml/MenuItem.qml
    qml/Image.qml
    qml/ListItem.qml
    qml/ListView.qml
    qml/Label.qml
    qml/FontMetrics.qml
    qml/Flickable.qml
    qml/GridView.qml
    qml/Icon.qml
    qml/IconSvg.qml
    qml/Text.qml
    qml/TextEdit.qml
    qml/Pane.qml
    qml/Page.qml
    qml/PageIndicator.qml
    qml/Popup.qml
    qml/Space.qml
    qml/SnakeBar.qml
    qml/StackView.qml
    qml/SplitView.qml
    qml/Slider.qml
    qml/SearchBar.qml
    qml/ScrollBar.qml
    qml/Switch.qml
    qml/RailItem.qml
    qml/TextInput.qml
    qml/TextField.qml
    qml/TabBar.qml
    qml/TabButton.qml
    qml/Rectangle.qml
    qml/state/StateAppBar.qml
    qml/state/StateSnakeBar.qml
    qml/state/StateButton.qml
    qml/state/StateTabButton.qml
    qml/state/StateListItem.qml
    qml/state/StateSlider.qml
    qml/state/StateIconButton.qml
    qml/impl/BoxShadow.qml
    qml/impl/Shadow.qml
    qml/impl/ListBusyFooter.qml
    qml/impl/Ripple.qml
    qml/impl/Ripple2.qml
    qml/impl/RectangularGlow.qml
    qml/impl/CursorDelegate.qml
    qml/impl/SliderHandle.qml
    qml/impl/RoundedElevationEffect.qml
    qml/impl/ElevationEffect.qml
    qml/impl/TextFieldEmbed.qml
    qml/impl/RoundClip.qml)
set(RESOURCES
    assets/MaterialIconsRound-Regular.otf
    assets/MaterialIconsOutlined-Regular.otf)

set(QML_MATERIAL_BUILD_TYPE
    SHARED)

if(UNIX
   AND NOT EMSCRIPTEN)
  list(APPEND SOURCES src/linux/xdp.h src/linux/xdp.cpp)
endif()
if(EMSCRIPTEN)
  list(APPEND SOURCES src/wasm/wasm.cpp)

  set(QML_MATERIAL_BUILD_TYPE
      STATIC)
  add_link_options("-sEXPORT_NAME=Something" -s FULL_ES3=1)
  add_compile_options(-oz)
endif()

set_source_files_properties(
  qml/Token.qml
  PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
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

set(SHADER_GLSLS
    "assets/shader/sdf.glsl")
set(SHADER_SOURCES
    "assets/shader/default.vert" "assets/shader/round_clip.frag"
    "assets/shader/round_rect.frag" "assets/shader/shadow.frag"
    "assets/shader/rect_glow.frag")
qt_add_shaders(
  ${LIBNAME}
  "shader"
  PREFIX
  "/Qcm/Material"
  GLSL
  300es,330
  FILES
  ${SHADER_SOURCES})
foreach(
  file
  ${SHADER_SOURCES})
  set(output_file
      ".qsb/${file}.qsb")
  add_custom_command(
    OUTPUT ${output_file}
    DEPENDS ${SHADER_GLSLS}
    APPEND)
endforeach()

target_include_directories(
  ${LIBNAME}
  PUBLIC include
  PRIVATE "include/${LIBNAME}" "include/${LIBNAME}/kirigami" src)

set_target_properties(
  ${LIBNAME}
  PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_subdirectory(third_party)
add_subdirectory(example)

target_link_libraries(
  ${LIBNAME}
  PUBLIC Qt6::Core Qt6::Quick Qt6::QuickControls2 Qt6::GuiPrivate
  PRIVATE material_color)

function(qt_install_qml_module target_)
  get_target_property(QML_TARGET_PATH ${target_} QT_QML_MODULE_TARGET_PATH)
  get_target_property(QML_TYEPINFO ${target_} QT_QML_MODULE_TYPEINFO)
  get_target_property(QML_PLUGIN_TARGET ${target_} QT_QML_MODULE_PLUGIN_TARGET)
  set(QML_INSTALL_DIR
      "${CMAKE_INSTALL_LIBDIR}/qml/${QML_TARGET_PATH}")

  install(TARGETS ${target_})

  install(
    TARGETS ${QML_PLUGIN_TARGET}
    LIBRARY DESTINATION ${QML_INSTALL_DIR})

  install(
    FILES "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/qmldir"
          "${QT_QML_OUTPUT_DIRECTORY}/${QML_TARGET_PATH}/${QML_TYEPINFO}"
    DESTINATION ${QML_INSTALL_DIR})
endfunction()

qt_install_qml_module(${LIBNAME})
