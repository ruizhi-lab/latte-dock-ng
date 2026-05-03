#pragma once

#if __has_include(<KQuickAddons/QuickViewSharedEngine>)
#include <KQuickAddons/QuickViewSharedEngine>
#elif __has_include(<kquickaddons/quickviewsharedengine.h>)
#include <kquickaddons/quickviewsharedengine.h>
#elif __has_include(<plasmaquick/quickviewsharedengine.h>)
#include <plasmaquick/quickviewsharedengine.h>
#elif __has_include_next(<KQuickAddons/quickviewsharedengine.h>)
#include_next <KQuickAddons/quickviewsharedengine.h>
#else
#error "Could not find a usable QuickViewSharedEngine header"
#endif
