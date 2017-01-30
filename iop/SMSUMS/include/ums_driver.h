#ifndef __ums_driver_H
# define __ums_driver_H

# include <usbd.h>
# include <ums.h>

void UmsNotifyEE          ( int, int                            );
void UmsBulkProbeEndPoint ( USBMDevice*, UsbEndpointDescriptor* );
void UmsBulkRelease       ( USBMDevice*                         );
int  UmsWarmUp            ( USBMDevice*                         );
int  UmsRead              ( USBMDevice*, int, void*, int        );
void UmsCloseUnit         ( USBMDevice*                         );
int  UmsGetMaxLUN         ( USBMDevice*                         );
int  UmsGetCapacity       ( USBMDevice*                         );
int  UmsRequestSense      ( USBMDevice*                         );
int  UmsStart             ( USBMDevice*                         );
int  UmsTestReady         ( USBMDevice*                         );

#endif  /* __ums_driver_H */
