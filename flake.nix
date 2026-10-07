{
  description = "Latte Dock NG: a Wayland-first dock for KDE Plasma 6.3+";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" ];
    in {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          package = import ./default.nix { inherit pkgs; };
          testInputs = (package.buildInputs or [ ]) ++ [
            pkgs.kdePackages.breeze-icons
            pkgs.fontconfig
            pkgs.dejavu_fonts
            pkgs.qt6.qtsvg
          ];
          runtimeTestInputs = map
            (input: pkgs.lib.getOutput "out" (input.unwrapped or input))
            (package.passthru.runtimeInputs ++ [
              pkgs.kdePackages.breeze-icons
              pkgs.fontconfig
              pkgs.dejavu_fonts
              pkgs.qt6.qtsvg
            ]);
          qmlImportPath = pkgs.lib.makeSearchPath "lib/qt-6/qml" runtimeTestInputs;
          qtPluginPath = pkgs.lib.makeSearchPath "lib/qt-6/plugins" runtimeTestInputs;
        in {
          # Build and test the actual package in one derivation so flake check
          # and release packaging reuse the same compiler output.
          default = package.overrideAttrs (old: {
            cmakeFlags = (old.cmakeFlags or [ ]) ++ [
              "-DBUILD_TESTING=ON"
              "-DLATTE_STRICT_WARNINGS=ON"
              "-DLATTE_DBUS_SESSION_CONFIG=${pkgs.dbus}/share/dbus-1/session.conf"
            ];
            preConfigure = (old.preConfigure or "") + ''
              export QML2_IMPORT_PATH="${qmlImportPath}''${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
              export QML_IMPORT_PATH="$QML2_IMPORT_PATH"
              export NIXPKGS_QT6_QML_IMPORT_PATH="${qmlImportPath}''${NIXPKGS_QT6_QML_IMPORT_PATH:+:$NIXPKGS_QT6_QML_IMPORT_PATH}"
              export QT_PLUGIN_PATH="${qtPluginPath}''${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
              export FONTCONFIG_FILE="${pkgs.makeFontsConf { fontDirectories = [ pkgs.dejavu_fonts ]; }}"
              export XDG_CACHE_HOME="$TMPDIR/font-cache"
              mkdir -p "$XDG_CACHE_HOME"
              export QT_QPA_PLATFORM=offscreen
              export QT_QUICK_BACKEND=software
            '';
            buildInputs = testInputs;
            doCheck = true;
            buildPhase = ''
              runHook preBuild
              # The install phase needs every runtime target, including helpers
              # not pulled in by the application or the excluded-from-all tests.
              cmake --build . --parallel "$NIX_BUILD_CORES" \
                --target all latte-autotests
              runHook postBuild
            '';
            checkPhase = ''
              runHook preCheck
              # Nix hooks may scope configure-phase exports to that phase; give
              # CTest the same explicit module and plugin paths as CMake's QML probes.
              export QML2_IMPORT_PATH="${qmlImportPath}''${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
              export QML_IMPORT_PATH="$QML2_IMPORT_PATH"
              export NIXPKGS_QT6_QML_IMPORT_PATH="${qmlImportPath}''${NIXPKGS_QT6_QML_IMPORT_PATH:+:$NIXPKGS_QT6_QML_IMPORT_PATH}"
              export QT_PLUGIN_PATH="${qtPluginPath}''${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
              export FONTCONFIG_FILE="${pkgs.makeFontsConf { fontDirectories = [ pkgs.dejavu_fonts ]; }}"
              export XDG_CACHE_HOME="$TMPDIR/font-cache"
              mkdir -p "$XDG_CACHE_HOME"
              export QT_QPA_PLATFORM=offscreen
              export QT_QUICK_BACKEND=software
              export LATTE_TEST_ICON_THEME_PATH="${pkgs.kdePackages.breeze-icons}/share/icons"
              dbus-run-session --config-file=${pkgs.dbus}/share/dbus-1/session.conf \
                -- ctest --output-on-failure
              runHook postCheck
            '';
          });
        });

      checks = forAllSystems (system: {
        autotests = self.packages.${system}.default;
      });

      devShells = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          package = import ./default.nix { inherit pkgs; };
          runtimeInputs = map
            (input: pkgs.lib.getOutput "out" (input.unwrapped or input))
            (package.passthru.runtimeInputs ++ [
              pkgs.kdePackages.breeze-icons
              pkgs.fontconfig
              pkgs.dejavu_fonts
              pkgs.qt6.qtsvg
            ]);
          qmlImportPath = pkgs.lib.makeSearchPath "lib/qt-6/qml" runtimeInputs;
          qtPluginPath = pkgs.lib.makeSearchPath "lib/qt-6/plugins" runtimeInputs;
        in {
          default = pkgs.mkShell {
            inputsFrom = [ package ];
            packages = [
              pkgs.clang
              pkgs.dbus
              pkgs.gnumake
              pkgs.python3
            ];
            shellHook = ''
              export QML2_IMPORT_PATH="${qmlImportPath}''${QML2_IMPORT_PATH:+:$QML2_IMPORT_PATH}"
              export QML_IMPORT_PATH="$QML2_IMPORT_PATH"
              export NIXPKGS_QT6_QML_IMPORT_PATH="${qmlImportPath}''${NIXPKGS_QT6_QML_IMPORT_PATH:+:$NIXPKGS_QT6_QML_IMPORT_PATH}"
              export QT_PLUGIN_PATH="${qtPluginPath}''${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}"
            '';
          };
        });

      overlays.default = final: prev: {
        latte-dock-ng = import ./default.nix { pkgs = final; };
      };

      nixosModules.default = { ... }: {
        nixpkgs.overlays = [ self.overlays.default ];
      };
    };
}
