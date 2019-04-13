/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2019     SEGGER Microcontroller GmbH              *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device * USB Device stack for embedded applications    *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product.                          *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emUSB-Device version: V3.18b                                 *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : USB_HID_Keyboard.c
Purpose : Demonstrates usage of the HID component of the USB stack
          as a keyboard.
          Types a predefined string like from a regular keyboard.

Additional information:
  Preparations:
    It is advised to open a notepad application before
    connecting the USB cable.

  Expected behavior:
    The sample types a predefined string like from a regular keyboard.

  Sample output:
    The target side does not produce terminal output.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include <string.h>
#include <ctype.h>
#include "USB.h"
#include "USB_HID.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
//
// Specifies whether the return key should be sent in this sample.
//
#define SEND_RETURN 0

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
//  Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfo = {
  0xFFFF,//8765,         // VendorId
  0xFFFF,//1115,         // ProductId
  "Vendor",       // VendorName
  "HID keyboard sample",  // ProductName
  "12345678"      // SerialNumber
};

/*********************************************************************
*
*       Local data definitions
*
**********************************************************************
*/
typedef struct {
  U16 KeyCode;
  char cCharacter;
} SCANCODE_TO_DESC;

/*********************************************************************
*
*       _aHIDReport
*
*  This report is generated according to HID spec and
*  HID Usage Tables specifications.
*/
const U8 _aHIDReport[] = {
  USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_GENERIC_DESKTOP,
  USB_HID_LOCAL_USAGE + 1, USB_HID_USAGE_KEYBOARD,
  USB_HID_MAIN_COLLECTION + 1, USB_HID_COLLECTION_APPLICATION,
    USB_HID_GLOBAL_USAGE_PAGE + 1, 7,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 224,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 231,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 8,
    USB_HID_MAIN_INPUT + 1, USB_HID_VARIABLE,
    USB_HID_MAIN_INPUT + 1, 1,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 0,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 101,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 101,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 8,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 6,
    USB_HID_MAIN_INPUT + 1, 0,
    USB_HID_GLOBAL_USAGE_PAGE + 1, USB_HID_USAGE_PAGE_LEDS,
    USB_HID_LOCAL_USAGE_MINIMUM + 1, 1,
    USB_HID_LOCAL_USAGE_MAXIMUM + 1, 5,
    USB_HID_GLOBAL_LOGICAL_MINIMUM + 1, 0,
    USB_HID_GLOBAL_LOGICAL_MAXIMUM + 1, 1,
    USB_HID_GLOBAL_REPORT_SIZE + 1, 1,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 5,
    USB_HID_MAIN_OUTPUT + 1, 2,
    USB_HID_GLOBAL_REPORT_COUNT + 1, 3,
    USB_HID_MAIN_OUTPUT + 1, 1,
  USB_HID_MAIN_ENDCOLLECTION
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static U8 _IntervalTime;
static USB_HID_HANDLE _hInst;
static const  SCANCODE_TO_DESC _aScanCode2StringTable[] = {
  { 0x04, 'a'},
  { 0x05, 'b'},
  { 0x06, 'c'},
  { 0x07, 'd'},
  { 0x08, 'e'},
  { 0x09, 'f'},
  { 0x0A, 'g'},
  { 0x0B, 'h'},
  { 0x0C, 'i'},
  { 0x0D, 'j'},
  { 0x0E, 'k'},
  { 0x0F, 'l'},
  { 0x10, 'm'},
  { 0x11, 'n'},
  { 0x12, 'o'},
  { 0x13, 'p'},
  { 0x14, 'q'},
  { 0x15, 'r'},
  { 0x16, 's'},
  { 0x17, 't'},
  { 0x18, 'u'},
  { 0x19, 'v'},
  { 0x1A, 'w'},
  { 0x1B, 'x'},
  { 0x1C, 'y'},
  { 0x1D, 'z'},
  { 0x1E, '1'},
  { 0x1F, '2'},
  { 0x20, '3'},
  { 0x21, '4'},
  { 0x22, '5'},
  { 0x23, '6'},
  { 0x24, '7'},
  { 0x25, '8'},
  { 0x26, '9'},
  { 0x27, '0'},
  { 0x2C, ' '},
  { 0x37, '.'}
};

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _AddHID
*
*  Function description
*    Add HID mouse class to USB stack
*/
static USB_HID_HANDLE _AddHID(void) {
  USB_HID_INIT_DATA InitData;

  memset(&InitData, 0, sizeof(InitData));
  InitData.EPIn    = USBD_AddEP(USB_DIR_IN,  USB_TRANSFER_TYPE_INT, _IntervalTime, NULL, 0);
  InitData.pReport = _aHIDReport;
  InitData.NumBytesReport = sizeof(_aHIDReport);
  USBD_SetDeviceInfo(&_DeviceInfo);
  _hInst = USBD_HID_Add(&InitData);
  return _hInst;
}

/*********************************************************************
*
*       _Output
*
*  Function description
*    Outputs a string
*/
static void _Output(const char *sString) {
  U8   ac[8];
  char cTemp;
  unsigned int i;
  unsigned int j;

  memset(ac, 0, sizeof(ac));
  for (i = 0; sString[i] != 0; i++) {
    //
    // A character is uppercase if it's hex value is less than 0x61 ('a')
    // and greater or equal to 0x41 ('A'), therefore we set the
    // LeftShiftUp bit for those characters
    //
    if (sString[i] < 0x61 && sString[i] >= 0x41) {
      ac[0] = (1 << 1);
      cTemp = tolower((int)sString[i]);
    } else {
      cTemp = sString[i];
    }
    for (j = 0; j < sizeof(_aScanCode2StringTable)/sizeof(_aScanCode2StringTable[0]); j++) {
      if (_aScanCode2StringTable[j].cCharacter == cTemp) {
        ac[2] = _aScanCode2StringTable[j].KeyCode;
      }
    }
    USBD_HID_Write(_hInst, &ac[0], 8, 0);
    memset(ac, 0, sizeof(ac));
    //
    // Send a 0 field packet to tell the host that the key has been released
    //
    USBD_HID_Write(_hInst, &ac[0], 8, 0);
    USB_OS_Delay(50);
  }
}

#if (SEND_RETURN == 1)
/*********************************************************************
*
*       _SendReturnCharacter
*
*  Function description
*    Outputs a return character
*/
static void _SendReturnCharacter(void) 
{
  U8 ac[8];

  memset(ac, 0, sizeof(ac));
  ac[2] = 0x28;
  USBD_HID_Write(_hInst, &ac[0], 8, 0);
  memset(ac, 0, sizeof(ac));
  //
  // Send a 0 field packet to tell the host that the key has been released
  //
  USBD_HID_Write(_hInst, &ac[0], 8, 0);
  USB_OS_Delay(50);
}
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       MainTask
*
* USB handling task.
*   Modify to implement the desired protocol
*/
#ifdef __cplusplus
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif
void MainTask(void);
#ifdef __cplusplus
}
#endif
void MainTask(void) 
{
  const char * sInfo0 = "This sample is based on the SEGGER emUSB-Device software with an HID component. ";
  const char * sInfo1 = "For further information please visit: www.segger.com ";

  _IntervalTime = 64;   // We set a interval time of 8 ms (64 micro frames (64 * 125us = 8ms))
  USBD_Init();
  _AddHID();
  USBD_Start();
  while (1) 
  {

    //
    // Wait for configuration
    //
    while ((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED) {
      BSP_ToggleLED(0);
      USB_OS_Delay(50);
    }
    USB_OS_Delay(500);
    BSP_SetLED(0);

    /*while (1)
    {
        BSP_ToggleLED(0);
        USB_OS_Delay(300);
    }*/

    //
    // The "_SendReturnCharacter()" line can be added if desired. Please set
    // the SEND_RETURN define to 1 in the "Defines, configurable" section of this file.
    // This function will send a Return/Enter key to the host.
    // In some cases this is not wanted as a return key may have undesired behavior.
    //
    
    _Output(sInfo0);
#if (SEND_RETURN == 1)
    _SendReturnCharacter();
#endif
    _Output(sInfo1);
#if (SEND_RETURN == 1)
    _SendReturnCharacter();
#endif
  
  }
}

/**************************** end of file ***************************/

