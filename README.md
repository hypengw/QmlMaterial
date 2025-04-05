# Material Design 3 for Qml
This is a library of Qml implementing Google's Material Design.

## 🌐 [Live Demo](https://hypengw.github.io/QmlMaterialWeb/qmaterial_example.html)

### Dependencies

- Qt 6.8+

## Installation
```cmake
FetchContent_Declare(
  qml_material
  GIT_REPOSITORY https://github.com/hypengw/QmlMaterial.git
  GIT_TAG main
  # GIT_TAG <commit>
  EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(qml_material)
```

## Variation Icon
CMake option: `QM_ICON_FONT_URL`  
The default is embedded static round icon with `wght=400 opsz=24 GRAD=0 FILL=0/1`.  
There is also an embedded partial variation one: `qrc:/Qcm/Material/assets/MaterialSymbolsRounded.wght_400.opsz_24.woff2`.  
Set the option to it if you need animation when switching `FILL`.    

> [!NOTE]
> The full variation font will cause huge memory usage. It's better to make partial with a [font tool](https://fonttools.readthedocs.io/en/latest/index.html).    

### Acknowledgements
- [material-color-utilities](https://github.com/material-foundation/material-color-utilities)
- [kirigami](https://invent.kde.org/frameworks/kirigami)