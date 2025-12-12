{
  description = "QmlMaterial Development Environment";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
    in
    {
      # 1. Build rules for "nix build"
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.stdenv.mkDerivation rec {
            pname = "QmlMaterial";
            version = "0.1.4";

            # `nix run github:...` uses a GitHub source tarball, which does not
            # include Git LFS objects (fonts/images). Re-fetch via git with LFS
            # enabled so assets are available at build time.
            rawSrc =
              if (self.sourceInfo.type or "") == "github" then
                builtins.fetchGit {
                  url =
                    "https://github.com/${self.sourceInfo.owner}/${self.sourceInfo.repo}.git";
                  rev = self.sourceInfo.rev;
                  submodules = true;
                  lfs = true;
                }
              else
                ./.;

            # Drop Nix-specific files under example; keep shared QML/CMake sources.
            src = pkgs.lib.cleanSourceWith {
              src = rawSrc;
              filter =
                path: type:
                let
                  srcPrefix = toString rawSrc + "/";
                  rel = pkgs.lib.removePrefix srcPrefix (toString path);
                in
                !(
                  rel == "example/flake.nix" || rel == "example/flake.lock" || pkgs.lib.hasPrefix "example/build" rel
                );
            };

            # Build-time dependencies
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              pkg-config
              qt6.wrapQtAppsHook # Wrap to expose Qt plugin/QML paths
            ];

            # Runtime dependencies (Libraries)
            buildInputs =
              (with pkgs.qt6; [
                qtbase
                qtdeclarative
                qtshadertools
                qtsvg
                qttools
                qt5compat
              ])
              ++ (if pkgs.stdenv.isLinux then [ pkgs.qt6.qtwayland ] else [ ]);

            configurePhase = ''
              runHook preConfigure
              cmake -S . -B build-nix \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_LIBDIR=lib \
                -DQM_BUILD_EXAMPLE=ON \
                -DQM_DEPLOY_QML_APP=OFF \
                -DCMAKE_INSTALL_PREFIX=$out
              runHook postConfigure
            '';
            buildPhase = ''
              runHook preBuild
              cmake --build build-nix -- -j''${NIX_BUILD_CORES:-1}
              runHook postBuild
            '';
            installPhase = ''
              runHook preInstall
              cmake --install build-nix --prefix $out
              runHook postInstall
            '';

            qtWrapperArgs = [
              "--prefix"
              "QML_IMPORT_PATH"
              ":"
              "$out/lib/qt-6/qml"
              "--prefix"
              "QML_IMPORT_PATH"
              ":"
              "$out/lib64/qt-6/qml"
              "--prefix"
              "QML2_IMPORT_PATH"
              ":"
              "$out/lib/qt-6/qml"
              "--prefix"
              "QML2_IMPORT_PATH"
              ":"
              "$out/lib64/qt-6/qml"
              "--prefix"
              "DYLD_LIBRARY_PATH"
              ":"
              "$out/lib"
              "--prefix"
              "DYLD_LIBRARY_PATH"
              ":"
              "$out/lib64"
              "--prefix"
              "LD_LIBRARY_PATH"
              ":"
              "$out/lib"
              "--prefix"
              "LD_LIBRARY_PATH"
              ":"
              "$out/lib64"
            ];
          };
        }
      );

      # 2. Development environment for "nix develop"
      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgsFor.${system};
        in
        {
          default = pkgs.mkShell {
            name = "qmlmaterial-dev-shell";

            # Inherit dependencies from the package definition above
            # This ensures dev env matches build env exactly
            inputsFrom = [ self.packages.${system}.default ];

            packages = with pkgs; [
              gdb
              clang-tools # clangd, clang-format
            ];

            CMAKE_EXPORT_COMPILE_COMMANDS = "1";
          };
        }
      );

      # 3. "nix run"
      apps = forAllSystems (
        system:
        let
          pkg = self.packages.${system}.default;
        in
        {
          default = {
            type = "app";
            program = "${pkg}/bin/qm_example";
          };
        }
      );
    };
}
