#ifndef SEPI_PLATFORM_H
#define SEPI_PLATFORM_H

#if defined(SEPI_PLATFORM_IMPLEMENTATION)
  #define SEPI_COMMON_PLATFORM_IMPLEMENTATION

  #if defined(OS_LINUX)
    #define SEPI_LINUX_PLATFORM_IMPLEMENTATION
  #endif

  #if defined(OS_MACOS)
    #define SEPI_MACOS_PLATFORM_IMPLEMENTATION
  #endif

  #if defined(OS_WINDOWS)
    #define SEPI_WINDOWS_PLATFORM_IMPLEMENTATION
  #endif
#endif

#if defined(OS_LINUX)
#include "linux/platform.h"

#elif defined(OS_MACOS)
#include "macos/platform.h"

#elif defined(OS_WINDOWS)
#include "windows/platform.h"

#else
#error "Platform module is not defined for this OS!"

#endif // OS SELECTION ENDS

#endif  // SEPI_PLATFORM_H
