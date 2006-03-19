/*
 * mass_stor.c - USB Mass storage driver for PS2
 *
 * (C) 2002, David Ryan ( oobles@hotmail.com )
 * (C) 2003, TyRaNiD <tiraniddo@hotmail.com>
 * (C) 2004, Marek Olejnik (ole00@post.cz)
 * (C) 2004  Hermes (support for sector sizes from 512 to 4096 bytes)
 *
 * Other contributors and testers: Bigboss, Sincro, Spooo, BraveDog.
 * 
 * This module handles the setup and manipulation of USB mass storage devices
 * on the PS2
 *
 * See the file LICENSE included with this distribution for licensing terms.
 */

#include <tamtypes.h>
#include <sifrpc.h>
#include <thsemap.h>
#include <usbd.h>
#include <usbd_macro.h>
#include "mass_stor.h"

#define USB_SUBCLASS_MASS_RBC           0x01
#define USB_SUBCLASS_MASS_ATAPI         0x02
#define USB_SUBCLASS_MASS_QIC           0x03
#define USB_SUBCLASS_MASS_UFI           0x04
#define USB_SUBCLASS_MASS_SFF_8070I     0x05
#define USB_SUBCLASS_MASS_SCSI          0x06
#define USB_PROTOCOL_MASS_CBI           0x00
#define USB_PROTOCOL_MASS_CBI_NO_CCI    0x01
#define USB_PROTOCOL_MASS_BULK_ONLY     0x50

#define TAG_TEST_UNIT_READY  0
#define TAG_REQUEST_SENSE    3
#define TAG_INQUIRY         18
#define TAG_READ_CAPACITY   37
#define TAG_READ            40
#define TAG_START_STOP_UNIT	33
#define TAG_WRITE           42

#define DEVICE_DETECTED     1
#define DEVICE_CONFIGURED   2
#define DEVICE_DISCONNECTED 4

#define SMS_CALLBACK 18

static int inline getBI32 ( unsigned char* buf ) {

 return buf[ 3 ]  + ( buf[ 2 ] << 8 ) + ( buf[ 1 ] << 16 ) + ( buf[ 0 ] << 24 );

}  /* end getBI32 */

unsigned Size_Sector = 512;
unsigned g_MaxLBA;

typedef struct _mass_dev
{
	int controlEp;		//config endpoint id
	int bulkEpI;		//in endpoint id
	unsigned char bulkEpIAddr; // in endpoint address
	int bulkEpO;		//out endpoint id
	unsigned char bulkEpOAddr; // out endpoint address
	int packetSzI;		//packet size in
	int packetSzO;		//packet size out
	int devId;		//device id
	int configId;	//configuration id
	int status;
	int interfaceNumber;	//interface number
	int interfaceAlt;	//interface alternate setting
} mass_dev;

typedef struct _read_info {
	mass_dev* dev;
	void* buffer;
	int transferSize;
	int offset;
	int returnCode;
	int num;
	int semh;
} read_info;

typedef struct _cbw_packet {
	unsigned int signature; 
	unsigned int tag;
	unsigned int dataTransferLength;
	unsigned char flags;
	unsigned char lun;
	unsigned char comLength;		//command data length
	unsigned char comData[16];		//command data
} cbw_packet;

typedef struct _csw_packet {
	unsigned int signature;
	unsigned int tag;
	unsigned int dataResidue;
	unsigned char status;	
} csw_packet;

//void usb_bulk_read1(int resultCode, int bytes, void* arg);
void usb_bulk_read2(int resultCode, int bytes, void* arg);

UsbDriver driver;

int retCode = 0;
int retSize = 0;

static int returnCode;
static int returnSize;
static int residue;

mass_dev mass_device;

static void initCBWPacket ( cbw_packet* apPkt ) {

 apPkt -> signature = 0x43425355; 
 apPkt -> flags     = 0x80;
 apPkt -> lun       = 0;

}  /* end initCBWPacket */

void initCSWPacket(csw_packet* packet) {
	packet->signature = 0x53425355; 
}
void cbw_scsi_test_unit_ready(cbw_packet* packet) {
	packet->tag = TAG_TEST_UNIT_READY;
	packet->dataTransferLength = 0;		//TUR_REPLY_LENGTH
	packet->flags = 0x80;			//data will flow In
	packet->comLength = 6;			//short inquiry command

	//scsi command packet
	packet->comData[0] = 0x00;		//TUR operation code
	packet->comData[1] = 0;			//lun/reserved
	packet->comData[2] = 0;			//reserved
	packet->comData[3] = 0;			//reserved
	packet->comData[4] = 0;			//reserved
	packet->comData[5] = 0;			//control
}

void cbw_scsi_request_sense(cbw_packet* packet) {
	packet->tag = - TAG_REQUEST_SENSE;
	packet->dataTransferLength = 18	;	//REQUEST_SENSE_REPLY_LENGTH
	packet->flags = 0x80;			//sense data will flow In
	packet->comLength = 6;			//scsi command of size 6

	//scsi command packet
	packet->comData[0] = 0x03;		//request sense operation code
	packet->comData[1] = 0;			//lun/reserved
	packet->comData[2] = 0;			//reserved
	packet->comData[3] = 0;			//reserved
	packet->comData[4] = 18;		//allocation length
	packet->comData[5] = 0;			//Control

}

void cbw_scsi_inquiry(cbw_packet* packet) {
	packet->tag = - TAG_INQUIRY;
	packet->dataTransferLength = 36;	//INQUIRY_REPLY_LENGTH
	packet->flags = 0x80;			//inquiry data will flow In
	packet->comLength = 6;			//short inquiry command

	//scsi command packet
	packet->comData[0] = 0x12;		//inquiry operation code
	packet->comData[1] = 0;			//lun/reserved
	packet->comData[2] = 0;			//page code
	packet->comData[3] = 0;			//reserved
	packet->comData[4] = 36;		//inquiry reply length
	packet->comData[5] = 0;			//reserved/flag/link/
}

void cbw_scsi_start_stop_unit(cbw_packet* packet) {
	packet->tag = - TAG_START_STOP_UNIT;
	packet->dataTransferLength = 0;	//START_STOP_REPLY_LENGTH
	packet->flags = 0x80;			//inquiry data will flow In
	packet->comLength = 6;			//short SCSI command

	//scsi command packet
	packet->comData[0] = 0x1B;		//start stop unit operation code
	packet->comData[1] = 1;			//lun/reserved/immed
	packet->comData[2] = 0;			//reserved
	packet->comData[3] = 0;			//reserved
	packet->comData[4] = 3;			//reserved/LoEj/Start (load and stard)
	packet->comData[5] = 0;			//control
}

void cbw_scsi_read_capacity(cbw_packet* packet) {
	packet->tag = - TAG_READ_CAPACITY;
	packet->dataTransferLength = 8	;	//READ_CAPACITY_REPLY_LENGTH
	packet->flags = 0x80;			//inquiry data will flow In
	packet->comLength = 10;			//scsi command of size 10

	//scsi command packet
	packet->comData[0] = 0x25;		//read capacity operation code
	packet->comData[1] = 0;			//lun/reserved/RelAdr
	packet->comData[2] = 0;			//LBA 1
	packet->comData[3] = 0;			//LBA 2
	packet->comData[4] = 0;			//LBA 3
	packet->comData[5] = 0;			//LBA 4
	packet->comData[6] = 0;			//Reserved
	packet->comData[7] = 0;			//Reserved
	packet->comData[8] = 0;			//Reserved
	packet->comData[9] = 0;			//Control

}

void cbw_scsi_read_sector(cbw_packet* packet, int lba, int sectorSize, int sectorCount) {
	packet->tag = - TAG_READ;
	packet->dataTransferLength = sectorSize	 * sectorCount;	
	packet->flags = 0x80;			//read data will flow In
	packet->comLength = 10;			//scsi command of size 10

	//scsi command packet
	packet->comData[0] = 0x28;		//read operation code
	packet->comData[1] = 0;			//LUN/DPO/FUA/Reserved/Reldr
	packet->comData[2] = (lba & 0xFF000000) >> 24;	//lba 1 (MSB)
	packet->comData[3] = (lba & 0xFF0000) >> 16;	//lba 2
	packet->comData[4] = (lba & 0xFF00) >> 8;	//lba 3
	packet->comData[5] = (lba & 0xFF);		//lba 4 (LSB)
	packet->comData[6] = 0;			//Reserved
	packet->comData[7] = (sectorCount & 0xFF00) >> 8;	//Transfer length MSB
	packet->comData[8] = (sectorCount & 0xFF);			//Transfer length LSB 
	packet->comData[9] = 0;			//control
}

void cbw_scsi_write_sector(cbw_packet* packet, int lba, int sectorSize, int sectorCount) {
	packet->tag = -TAG_WRITE;
	packet->dataTransferLength = sectorSize	 * sectorCount;	
	packet->flags = 0x00;			//write data will flow Out
	packet->comLength = 10;			//scsi command of size 10

	//scsi command packet
	packet->comData[0] = 0x2A;		//WRITE(10) operation code
	packet->comData[1] = 0;			//LUN/DPO/FUA/Reserved/Reldr
	packet->comData[2] = (lba & 0xFF000000) >> 24;	//lba 1 (MSB)
	packet->comData[3] = (lba & 0xFF0000) >> 16;	//lba 2
	packet->comData[4] = (lba & 0xFF00) >> 8;	//lba 3
	packet->comData[5] = (lba & 0xFF);		//lba 4 (LSB)
	packet->comData[6] = 0;			//Reserved
	packet->comData[7] = (sectorCount & 0xFF00) >> 8;	//Transfer length MSB
	packet->comData[8] = (sectorCount & 0xFF);			//Transfer length LSB 
	packet->comData[9] = 0;			//control
}

void usb_callback(int resultCode, int bytes, void *arg) {
	int semh = (int) arg;
	returnCode = resultCode;
	returnSize = bytes;
	SignalSema(semh);
}

void set_configuration(mass_dev* dev, int configNumber) {
	int ret;
	int semh;
	iop_sema_t s;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);


	ret = UsbSetDeviceConfiguration(dev->controlEp, configNumber, usb_callback, (void*)semh);

	if(ret != USB_RC_OK) {
		;
	} else {
		
		WaitSema(semh);
	}
	DeleteSema(semh);

}

void set_interface(mass_dev* dev, int interface, int altSetting) {
	int ret;
	int semh;
	iop_sema_t s;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);



	ret = UsbSetInterface(dev->controlEp, interface, altSetting, usb_callback, (void*)semh);

        if(ret != USB_RC_OK) {
;
	} else {

		WaitSema(semh);
	}
	DeleteSema(semh);

}

void set_device_feature(mass_dev* dev, int feature) {
	int ret;
	int semh;
	iop_sema_t s;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);


	ret = UsbSetDeviceFeature(dev->controlEp, feature, usb_callback, (void*)semh);

	if(ret != USB_RC_OK) {
;
	} else {

		WaitSema(semh);
	}
	DeleteSema(semh);

}

void usb_bulk_clear_halt(mass_dev* dev, int direction) {
	int ret;
	int semh;
	iop_sema_t s;
	int endpoint;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;

	if (direction == 0) {
		endpoint = dev->bulkEpIAddr;
		//endpoint = dev->bulkEpI;
	} else {
		endpoint = dev->bulkEpOAddr;
	}

	semh = CreateSema(&s);
	ret = UsbClearEndpointFeature(
		dev->controlEp, 		//Config pipe
		0,			//HALT feature
		endpoint,
		usb_callback, 
		(void*)semh
		);

	if(ret != USB_RC_OK) {
		return;
	}else {
		WaitSema(semh);
	}

	DeleteSema(semh);

}

void usb_bulk_reset(mass_dev* dev, int mode) {
	int ret;
	int semh;
	iop_sema_t s;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);

	//Call Bulk only mass storage reset
	ret = UsbControlTransfer(
		dev->controlEp, 		//default pipe
		0x21,			//bulk reset
		0xFF,
		0,
		0,			//interface number  FIXME - correct number
		0,			//length
		NULL,			//data
		usb_callback, 
		(void*) semh
		);

	if(ret != USB_RC_OK) {
		return;
	}else {
		WaitSema(semh);
	}
	DeleteSema(semh);

	//clear bulk-in endpoint
	if (mode & 0x01) {
		usb_bulk_clear_halt(dev, 0);
	}

	//clear bulk-out endpoint
	if (mode & 0x02) {
		usb_bulk_clear_halt(dev, 1);
	}
}

int usb_bulk_status(mass_dev* dev, csw_packet* csw, int tag) {
	int ret;
	iop_sema_t s;
	int semh;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);

	initCSWPacket(csw);
	csw->tag = tag;
	csw->dataResidue = residue;
	csw->status = 0;

	ret =  UsbBulkTransfer(
		dev->bulkEpI,		//bulk input pipe
		csw,			//data ptr
		13,			//data length
		usb_callback,
		(void*)semh
	);
	if(ret != USB_RC_OK) {
		DeleteSema(semh);
		return -1;
	}else {
		WaitSema(semh);
	}
	DeleteSema(semh);
	return csw->status;
}

/* see flow chart in the usbmassbulk_10.pdf doc (page 15) */
int usb_bulk_manage_status(mass_dev* dev, int tag) {
	int ret;
	csw_packet csw;

	ret = usb_bulk_status(dev, &csw, tag); /* Attempt to read CSW from bulk in endpoint */
	if (ret == 4 || ret < 0) { /* STALL bulk in  -OR- Bulk error */
		usb_bulk_clear_halt(dev, 0); /* clear the stall condition for bulk in */
		
		ret = usb_bulk_status(dev, &csw, tag); /* Attempt to read CSW from bulk in endpoint */
		
	}

	/* CSW not valid  or stalled or phase error */
	if (csw.signature !=  0x53425355 || csw.tag != tag || ret != 0) {
		usb_bulk_reset(dev, 3);	/* Perform reset recovery */
	}

	return 0; //device is ready to process next CBW
}


void usb_bulk_command(mass_dev* dev, cbw_packet* packet ) {
	int ret;
	iop_sema_t s;
	int semh;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);

	ret =  UsbBulkTransfer(
		dev->bulkEpO,		//bulk output pipe
		packet,			//data ptr
		31,			//data length
		usb_callback,
		(void*)semh
	);
	if(ret != USB_RC_OK) {
;
	}else {
		WaitSema(semh);
	}
	DeleteSema(semh);

}


int usb_bulk_transfer(int pipe, void* buffer, int transferSize) {
	int ret;
	char* buf = (char*) buffer;
	int blockSize = transferSize;
	int offset = 0;
	int ep; //endpoint

	iop_sema_t s;
	int semh;

	s.initial = 0;
	s.max = 1;
	s.option = 0;
	s.attr = 0;
	semh = CreateSema(&s);

	while (transferSize > 0) {
		if (transferSize < blockSize) {
			blockSize = transferSize;
		}	

		ret =  UsbBulkTransfer(
			pipe,		//bulk pipe epI(Read)  epO(Write)
			(buf + offset),		//data ptr
			blockSize,		//data length
			usb_callback,
			(void*)semh
		);
		if(ret != USB_RC_OK) {
			returnCode = -1;
			break;
		}else {
			WaitSema(semh);
			if (returnCode > 0) {
				residue = blockSize;
				break;
			}
			offset += returnSize;
			transferSize-= returnSize;
		}
	}
	DeleteSema(semh);
	return returnCode;
}


int mass_stor_readSector(unsigned int sector, unsigned char* buffer) {
	cbw_packet cbw;
	int sectorSize;
	int stat;

	/* assume device is detected and configured - should be checked in upper levels */

	initCBWPacket(&cbw);
	sectorSize = Size_Sector;

	cbw_scsi_read_sector(&cbw, sector, sectorSize, 4096/sectorSize);  // Added by Hermes

	stat = 1;
	while (stat != 0) {
		usb_bulk_command(&mass_device, &cbw);

		stat = usb_bulk_transfer(mass_device.bulkEpI, buffer, 4096); //Modified by Hermes 

		stat = usb_bulk_manage_status(&mass_device, -TAG_READ);
	}
	return 4096;
}

void usb_bulk_probeEndpoint(mass_dev* dev, UsbEndpointDescriptor* endpoint) {
	if (endpoint->bmAttributes == USB_ENDPOINT_XFER_BULK) {
		/* out transfer */
		if ((endpoint->bEndpointAddress & 0x80) == 0 && dev->bulkEpO < 0) {
			dev->bulkEpOAddr = endpoint->bEndpointAddress;
			dev->bulkEpO = UsbOpenBulkEndpoint(dev->devId, endpoint);
			dev->packetSzO = endpoint->wMaxPacketSizeHB * 256 + endpoint->wMaxPacketSizeLB;
		}else
		/* in transfer */
		if ((endpoint->bEndpointAddress & 0x80) != 0 && dev->bulkEpI < 0) {
			dev->bulkEpIAddr = endpoint->bEndpointAddress;
			dev->bulkEpI = UsbOpenBulkEndpoint(dev->devId, endpoint);
			dev->packetSzI = endpoint->wMaxPacketSizeHB * 256 + endpoint->wMaxPacketSizeLB;
		}
	}
}

int mass_stor_probe(int devId) {
	UsbDeviceDescriptor *device = NULL;        
	UsbConfigDescriptor *config = NULL;
	UsbInterfaceDescriptor *intf = NULL;

	if (mass_device.status & DEVICE_DETECTED) {
		return 0;
	}

	device = (UsbDeviceDescriptor*)UsbGetDeviceStaticDescriptor(devId, NULL, USB_DT_DEVICE);
	if (device == NULL)  {
		printf("mass_stor: Error: Couldn't get device descriptor\n");
		return 0;
	}

	if(device->bNumConfigurations < 1) {
	      return 0;
	}
	/* read configuration */
	config = (UsbConfigDescriptor*)UsbGetDeviceStaticDescriptor(devId, device, USB_DT_CONFIG);
	if (config == NULL) {
	      printf("ERROR: Couldn't get configuration descriptor\n");
	      return 0;
	}
	/* check that at least one interface exists */
	if(	(config->bNumInterfaces < 1) || 
		(config->wTotalLength < (sizeof(UsbConfigDescriptor) + sizeof(UsbInterfaceDescriptor)))) {
		      printf("mass_stor: Error: No interfaces available\n");
		      return 0;
	}
        /* get interface */
	intf = (UsbInterfaceDescriptor *) ((char *) config + config->bLength); /* Get first interface */

	if(	(intf->bInterfaceClass		!= USB_CLASS_MASS_STORAGE) ||
		(intf->bInterfaceSubClass	!= USB_SUBCLASS_MASS_SCSI  &&
		 intf->bInterfaceSubClass	!= USB_SUBCLASS_MASS_SFF_8070I) ||
		(intf->bInterfaceProtocol	!= USB_PROTOCOL_MASS_BULK_ONLY) ||
		(intf->bNumEndpoints < 2))    { //one bulk endpoint is not enough because
			 return 0;		//we send the CBW to te bulk out endpoint
	}
	return 1;
}

static int sif_send_cmd ( int anID, int anArg ) {

 static int lCmdData[ 4 ] __attribute__(   (  aligned( 64 )  )   );

 lCmdData[ 3 ] = anArg;

 return sceSifSendCmd ( anID, lCmdData, 16, NULL, NULL, 0 );

}  /* end sif_send_cmd */

int mass_stor_connect(int devId) {
	int i;
	int epCount;
	UsbDeviceDescriptor *device;        
	UsbConfigDescriptor *config;
	UsbInterfaceDescriptor *interface;
	UsbEndpointDescriptor *endpoint;
	u16 number;
	mass_dev* dev;
	
	printf ("usb_mass: connect: devId=%i\n", devId);
	dev = &mass_device;
	
	/* only one mass device allowed */
	if (dev->status & DEVICE_DETECTED) {
		printf("usb_mass: Error - only one mass storage device allowed !\n");
		return 1;
	}

	dev->devId = devId;
	dev->bulkEpI = -1;
	dev->bulkEpO = -1;
	dev->controlEp = -1;

	/* open the config endpoint */
	dev->controlEp = UsbOpenEndpoint(devId, NULL);
	
	device = (UsbDeviceDescriptor*)UsbGetDeviceStaticDescriptor(devId, NULL, USB_DT_DEVICE);

	config = (UsbConfigDescriptor*)UsbGetDeviceStaticDescriptor(devId, device, USB_DT_CONFIG);

	interface = (UsbInterfaceDescriptor *) ((char *) config + config->bLength); /* Get first interface */
	// store interface numbers
	dev->interfaceNumber = interface->bInterfaceNumber;
	dev->interfaceAlt    = interface->bAlternateSetting;

	epCount = interface->bNumEndpoints;
	endpoint = (UsbEndpointDescriptor*) UsbGetDeviceStaticDescriptor(devId, NULL, USB_DT_ENDPOINT);
	usb_bulk_probeEndpoint(dev, endpoint);
	
	for (i = 1; i < epCount; i++) {
		endpoint = (UsbEndpointDescriptor*) ((char *) endpoint + endpoint->bLength);
		usb_bulk_probeEndpoint(dev, endpoint);
	}

	/* we do NOT have enough bulk endpoints */
	if (dev->bulkEpI < 0  /* || dev->bulkEpO < 0 */ ) { /* the bulkOut is not needed now */
		if (dev->bulkEpI >= 0) {
			UsbCloseEndpoint(dev->bulkEpI);
		}
		if (dev->bulkEpO >= 0) {
			UsbCloseEndpoint(dev->bulkEpO);
		}
		return -1;
	}

	dev->configId = config->bConfigurationValue;
	dev->status += DEVICE_DETECTED;
	printf("usb_mass: connect ok: epI=%i, epO=%i \n", dev->bulkEpI, dev->bulkEpO);

      sif_send_cmd ( SMS_CALLBACK, 1 );

	return 0;
}

void mass_stor_release ( void ) {

 mass_dev* lpDev = &mass_device;

 if ( lpDev -> bulkEpI >= 0 ) UsbCloseEndpoint ( lpDev -> bulkEpI );
 if ( lpDev -> bulkEpO >= 0 ) UsbCloseEndpoint ( lpDev -> bulkEpO );

 lpDev -> devId     = -1;
 lpDev -> bulkEpI   = -1;
 lpDev -> bulkEpO   = -1;
 lpDev -> controlEp = -1;
 lpDev -> status    = DEVICE_DISCONNECTED;

}  /* end mass_stor_release */

int mass_stor_disconnect ( int aDevID ) {

 mass_dev* lpDev = &mass_device;

 if (  ( lpDev -> status & DEVICE_DETECTED ) && aDevID == lpDev -> devId ) {

  mass_stor_release ();
  sif_send_cmd ( SMS_CALLBACK, 2 );

 }  /* end if */

 return 0;

}  /* end mass_stor_disconnect */

int mass_stor_warmup ( void ) {

 cbw_packet    lCBW;
 unsigned char lBuff[ 64 ];
 mass_dev*     lpDev;
 int           lSecSize;
 int           lStat;
 int           lRetryCount;
 int           i;

 lpDev = &mass_device;

 if (  !( lpDev -> status & DEVICE_DETECTED )  ) return -1;

 initCBWPacket ( &lCBW );

 memset ( lBuff, 0, 64 );
 cbw_scsi_inquiry ( &lCBW );
 usb_bulk_command ( lpDev, &lCBW );
 usb_bulk_transfer ( lpDev -> bulkEpI, lBuff, 36 );

 residue = 0;
 usb_bulk_manage_status ( lpDev, -TAG_INQUIRY );

 if ( returnSize <= 0 ) return -1;

 cbw_scsi_request_sense ( &lCBW );
 usb_bulk_command ( lpDev, &lCBW );
 usb_bulk_transfer ( lpDev -> bulkEpI, lBuff, 18 );

 lStat = usb_bulk_manage_status ( lpDev, -TAG_REQUEST_SENSE );

 cbw_scsi_read_capacity ( &lCBW );

 lStat       = 1;
 lRetryCount = 6;

 while ( lStat && lRetryCount > 0 ) {

  usb_bulk_command ( lpDev, &lCBW );
  usb_bulk_transfer ( lpDev -> bulkEpI, lBuff, 8 );

  if ( returnCode == 4 )

   usb_bulk_reset ( lpDev, 1 );

  else lStat = usb_bulk_manage_status ( lpDev, -TAG_READ_CAPACITY );

  --lRetryCount;

 }  /* end while */

 if ( lStat ) return 1;

 Size_Sector = getBI32 ( &lBuff[ 4 ] );
 g_MaxLBA    = getBI32 ( &lBuff[ 0 ] );

 return 0;

}  /* end mass_stor_warmup */

void mass_stor_reset ( void ) {

 mass_device.status &= ~DEVICE_DISCONNECTED;

}  /* end mass_stor_reset */

int mass_stor_getStatus ( void ) {

 int i;

 if (  !( mass_device.status & DEVICE_DETECTED )  ) return -1;

 if (  !( mass_device.status & DEVICE_CONFIGURED )  ) {

  set_configuration ( &mass_device, mass_device.configId );

  for ( i = 0; i < 0xFFFFF; ++i ) __asm__ __volatile__( "nop\nnop\nnop\nnop" );	

  set_interface ( &mass_device, mass_device.interfaceNumber, mass_device.interfaceAlt );
  mass_device.status += DEVICE_CONFIGURED;
  i = mass_stor_warmup ();

  if ( i < 0 ) {

   mass_stor_release ();
   return i;

  }  /* end if */

 }  /* end if */

 return mass_device.status;

}  /* end mass_stor_getStatus */

int mass_stor_init ( void ) {

 int ret;

 mass_device.status = 0;

 driver.next       = NULL;
 driver.prev       = NULL;	
 driver.name       = "mass-stor";
 driver.probe      = mass_stor_probe; 
 driver.connect    = mass_stor_connect;
 driver.disconnect = mass_stor_disconnect;

 return UsbRegisterDriver ( &driver );

}  /* end mass_stor_init */
