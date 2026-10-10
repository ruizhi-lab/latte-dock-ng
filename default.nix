{ pkgs ? import <nixpkgs> {}, releaseVersion ? builtins.getEnv "LATTE_RELEASE_VERSION" }:

let
  inherit (pkgs) lib stdenv cmake wayland;
  inherit (pkgs) kdePackages;
  runtimeInputs = [
    kdePackages.qtbase
    kdePackages.qtdeclarative
    kdePackages.qtwayland

    kdePackages.libplasma
    kdePackages.plasma-activities
    kdePackages.plasma-activities-stats
    kdePackages.plasma-workspace
    kdePackages.kwayland
    kdePackages.layer-shell-qt
    wayland

    kdePackages.karchive
    kdePackages.kcmutils
    kdePackages.kconfig
    kdePackages.kcoreaddons
    kdePackages.kcrash
    kdePackages.kdbusaddons
    kdePackages.kdeclarative
    kdePackages.kglobalaccel
    kdePackages.kguiaddons
    kdePackages.ki18n
    kdePackages.kiconthemes
    kdePackages.kio
    kdePackages.kirigami
    kdePackages.knewstuff
    kdePackages.knotifications
    kdePackages.kpackage
    kdePackages.ksvg
    kdePackages.kwindowsystem
    kdePackages.kxmlgui
  ];
  # CMake reads these XML definitions to generate Wayland client bindings.
  buildInputs = runtimeInputs ++ [ kdePackages.plasma-wayland-protocols ];
  package = stdenv.mkDerivation {
    pname = "latte-dock-ng";
    version = "1.2.55";

    src = lib.cleanSource ./.;

    nativeBuildInputs = [
      cmake
      pkgs.dbus
      kdePackages.extra-cmake-modules
      kdePackages.wrapQtAppsHook
    ];

    inherit buildInputs;

    passthru.runtimeInputs = runtimeInputs;

    meta = with lib; {
      description = "Dock-style app launcher based on Plasma frameworks (KDE Plasma 6 fork)";
      homepage = "https://github.com/ruizhi-lab/latte-dock-ng";
      license = licenses.gpl3Plus;
      platforms = [ "x86_64-linux" ];
      maintainers = [ ];
      mainProgram = "latte-dock-ng";
    };
  };
in
# Pure flake evaluation keeps the source default. Release CI explicitly opts
# into impure evaluation to bind both package metadata and CMake to its input.
if releaseVersion == "" then package else
assert builtins.match "(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)" releaseVersion != null;
package.overrideAttrs (old: {
  version = releaseVersion;
  cmakeFlags = (old.cmakeFlags or [ ]) ++ [ "-DVERSION=${releaseVersion}" ];
})
