#pragma once

// In Plasma 6, plasma/applet.h forward-declares 'class Plasma::Package;'
// Defining 'using Plasma::Package = KPackage::Package' here would conflict.
// Just provide the full KPackage::Package definition; callers use kPackage()
// which returns KPackage::Package directly.
#if __has_include(<KPackage/Package>)
#include <KPackage/Package>
#else
#include <kpackage/package.h>
#endif
