{ pkgs ? import <nixpkgs> {} }:

let
  inherit (pkgs) lib stdenv cmake wayland;
  inherit (pkgs) kdePackages;
  buildInputs = [
    kdePackages.qtbase
    kdePackages.qtdeclarative
    kdePackages.qtwayland

    kdePackages.libplasma
    kdePackages.plasma-activities
    kdePackages.plasma-activities-stats
    kdePackages.plasma-workspace
    kdePackages.kwayland
    kdePackages.plasma-wayland-protocols
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
in
stdenv.mkDerivation {
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

  passthru.runtimeInputs = buildInputs;

  meta = with lib; {
    description = "Dock-style app launcher based on Plasma frameworks (KDE Plasma 6 fork)";
    homepage = "https://github.com/ruizhi-lab/latte-dock-ng";
    license = licenses.gpl3Plus;
    platforms = [ "x86_64-linux" ];
    maintainers = [ ];
    mainProgram = "latte-dock-ng";
  };
}
