#include <usbd.h>
#include <ioman.h>
#include <sifcmd.h>
#include <sifman.h>
#include <smsutils.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thsemap.h>

#include <stdio.h>

usbd_IMPORTS_start
 I_UsbOpenEndpoint
 I_UsbOpenEndpointAligned
 I_UsbCloseEndpoint
 I_UsbGetDeviceStaticDescriptor
 I_UsbRegisterDriver
 I_UsbTransfer
usbd_IMPORTS_end

ioman_IMPORTS_start
 I_AddDrv
#ifdef _DEBUG
 I_open
 I_lseek
 I_write
 I_close
#endif  /* _DEBUG */
ioman_IMPORTS_end

sifcmd_IMPORTS_start
 I_sceSifSendCmd
sifcmd_IMPORTS_end

sifman_IMPORTS_start
 I_sceSifDmaStat
sifman_IMPORTS_end

smsutils_IMPORTS_start
 I_mips_memcpy
 I_mips_memset
smsutils_IMPORTS_end

sysclib_IMPORTS_start
 I_strcmp
 I_strcpy
 I_strlen
 I_tolower
#ifdef _DEBUG
 I_strcat
 I_sprintf
#endif  /* _DEBUG */
sysclib_IMPORTS_end

sysmem_IMPORTS_start
 I_AllocSysMemory
 I_FreeSysMemory
sysmem_IMPORTS_end

thbase_IMPORTS_start
 I_CreateThread
 I_DelayThread
 I_StartThread
 I_TerminateThread
 I_DeleteThread
thbase_IMPORTS_end

thsemap_IMPORTS_start
 I_CreateSema
 I_DeleteSema
 I_SignalSema
 I_WaitSema
thsemap_IMPORTS_end
#ifdef _DEBUG
stdio_IMPORTS_start
 I_printf
stdio_IMPORTS_end
#endif  /* _DEBUG */
