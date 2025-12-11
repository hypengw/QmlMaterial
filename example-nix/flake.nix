{
  description = "Example for QmlMaterial with Nix Flakes";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  inputs.qmlmaterial.url = "path:/Users/lolimaster/Projects/QmlMaterial"; # 或 github:user/QmlMaterial

  outputs =
    {
      self,
      nixpkgs,
      qmlmaterial,
    }:
    let
      system = "aarch64-darwin"; # 根据需要改
      pkgs = import nixpkgs { inherit system; };
      qmLib = qmlmaterial.packages.${system}.default;
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "qml-material-demo";
        version = "0.0.1";
        src = ./.;

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          qt6.wrapQtAppsHook
        ];

        buildInputs =
          (with pkgs.qt6; [
            qtbase
            qtdeclarative
            qtshadertools
          ])
          ++ [ qmLib ];

        cmakeFlags = [
          "-DCMAKE_BUILD_TYPE=Release"
          "-DCMAKE_INSTALL_RPATH=${qmLib}/lib"
          "-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON"
        ];

        qtWrapperArgs = [
          "--prefix"
          "QML_IMPORT_PATH"
          ":"
          "$out/lib/qt-6/qml:${qmLib}/lib/qt-6/qml"
          "--prefix"
          "QML2_IMPORT_PATH"
          ":"
          "$out/lib/qt-6/qml:${qmLib}/lib/qt-6/qml"
          "--prefix"
          "DYLD_LIBRARY_PATH"
          ":"
          "${qmLib}/lib"
          "--prefix"
          "LD_LIBRARY_PATH"
          ":"
          "${qmLib}/lib"
        ];
      };

      apps.${system}.default = {
        type = "app";
        program = "${self.packages.${system}.default}/bin/qml-material-demo";
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ self.packages.${system}.default ];
      };
    };
}
