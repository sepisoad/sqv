#ifndef SEPI_IO_H
#define SEPI_IO_H

#if defined(SEPI_IO_IMPLEMENTATION)
  #define SEPI_COMMON_IO_IMPLEMENTATION

  #if defined(OS_LINUX)
    #define SEPI_LINUX_IO_IMPLEMENTATION
  #endif

  #if defined(OS_MACOS)
    #define SEPI_MACOS_IO_IMPLEMENTATION
  #endif

  #if defined(OS_WINDOWS)
    #define SEPI_WINDOWS_IO_IMPLEMENTATION
  #endif
#endif

#if defined(OS_LINUX)
#include "linux/io.h"

#elif defined(OS_MACOS)
#include "macos/io.h"

#elif defined(OS_WINDOWS)
#include "windows/io.h"

#else
#error "IO module is not defined for this OS!"

#endif // OS SELECTION ENDS

#endif  // SEPI_IO_H
