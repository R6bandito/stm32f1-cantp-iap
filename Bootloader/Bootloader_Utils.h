#ifndef __BOOTLOADER_UTILS_H__
#define __BOOTLOADER_UTILS_H__

#include "BootloaderConf.h"

  #if (USE_UTILS_DEBUG)
    void Cus_Bootloader_Utils_DebugUART( void );
  #endif // USE_UTILS_DEBUG

  
  #if (USE_UTILS_SYSCONF)
    void Cus_Bootloader_Utils_SystemClockConfig( void );
  #endif // USE_UTILS_SYSCONF

#endif // __BOOTLOADER_UTILS_H__
