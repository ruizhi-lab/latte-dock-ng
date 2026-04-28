#pragma once

#if defined(__has_include_next)
#if __has_include_next(<Plasma/version.h>)
#include_next <Plasma/version.h>
#endif
#endif

#if __has_include(<Plasma/plasma.h>)
#include <Plasma/plasma.h>
#elif __has_include(<plasma/plasma.h>)
#include <plasma/plasma.h>
#endif

#if __has_include(<Plasma/plasma_version.h>)
#include <Plasma/plasma_version.h>
#elif __has_include(<plasma/plasma_version.h>)
#include <plasma/plasma_version.h>
#endif
