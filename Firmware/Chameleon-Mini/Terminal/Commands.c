/* Copyright 2013 Timo Kasper, Simon Küppers, David Oswald ("ORIGINAL
 * AUTHORS"). All rights reserved.
 *
 * DEFINITIONS:
 *
 * "WORK": The material covered by this license includes the schematic
 * diagrams, designs, circuit or circuit board layouts, mechanical
 * drawings, documentation (in electronic or printed form), source code,
 * binary software, data files, assembled devices, and any additional
 * material provided by the ORIGINAL AUTHORS in the ChameleonMini project
 * (https://github.com/skuep/ChameleonMini).
 *
 * LICENSE TERMS:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK are permitted provided that the
 * following conditions are met:
 *
 * Redistributions and use of this WORK, with or without modification, or
 * of substantial portions of this WORK must include the above copyright
 * notice, this list of conditions, the below disclaimer, and the following
 * attribution:
 *
 * "Based on ChameleonMini an open-source RFID emulator:
 * https://github.com/skuep/ChameleonMini"
 *
 * The attribution must be clearly visible to a user, for example, by being
 * printed on the circuit board and an enclosure, and by being displayed by
 * software (both in binary and source code form).
 *
 * At any time, the majority of the ORIGINAL AUTHORS may decide to give
 * written permission to an entity to use or redistribute the WORK (with or
 * without modification) WITHOUT having to include the above copyright
 * notice, this list of conditions, the below disclaimer, and the above
 * attribution.
 *
 * DISCLAIMER:
 *
 * THIS PRODUCT IS PROVIDED BY THE ORIGINAL AUTHORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE ORIGINAL AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS PRODUCT, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the hardware, software, and
 * documentation should not be interpreted as representing official
 * policies, either expressed or implied, of the ORIGINAL AUTHORS.
 */

#include "Commands.h"
#include <stdio.h>
#include <avr/pgmspace.h>
#include "XModem.h"
#include "../Settings.h"
#include "../Chameleon-Mini.h"
#include "../src/LUFA/LUFA/Version.h"
#include "../Configuration.h"
#include "../Random.h"
#include "../Memory.h"
#include "../System.h"
#include "../Button.h"
#include "../AntennaLevel.h"
#include "../Application/MifareUltralight.h"

extern const PROGMEM CommandEntryType CommandTable[];

CommandStatusIdType CommandGetVersion(char* OutParam) {
	//snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("Chameleon-new-1.0"));
	snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("Chameleon Mini %s v%d.%d (%s)"),FIRMWARE_NAME, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_FORK_AUTHOR );
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetConfig(char* OutParam) {
	ConfigurationGetByName(OutParam, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetConfig(char* OutMessage, const char* InParam) {
	if (ConfigurationSetByName(InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecConfig(char* OutMessage) {
  ConfigurationGetList(OutMessage, TERMINAL_BUFFER_SIZE);
  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetUid(char* OutParam) {
  uint8_t UidBuffer[COMMAND_UID_BUFSIZE];
  uint16_t UidSize = ActiveConfiguration.UidSize;

  ApplicationGetUid(UidBuffer);
  BufferToHexString(OutParam, TERMINAL_BUFFER_SIZE, UidBuffer, UidSize);
  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetUid(char* OutMessage, const char* InParam) {
  uint8_t UidBuffer[COMMAND_UID_BUFSIZE];
  uint16_t UidSize = ActiveConfiguration.UidSize;

  if (strcmp_P(InParam, PSTR(COMMAND_UID_RANDOM)) == 0) {
    /* Load with random bytes */
    for (uint8_t i=0; i<UidSize; i++) {
      UidBuffer[i] = RandomGetByte();
    }
  } else {
    /* Convert to Bytes */
    if (HexStringToBuffer(UidBuffer, sizeof(UidBuffer), InParam) != UidSize) {
      /* Malformed input. Abort */
      return COMMAND_ERR_INVALID_PARAM_ID;
    }
  }

  ApplicationSetUid(UidBuffer);
  return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandGetReadOnly(char* OutParam)
{
  if (ActiveConfiguration.ReadOnly) {
    OutParam[0] = COMMAND_CHAR_TRUE;
  } else {
    OutParam[0] = COMMAND_CHAR_FALSE;
  }

  OutParam[1] = '\0';

  return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetReadOnly(char* OutMessage, const char* InParam)
{
  if (InParam[1] == '\0') {
    if (InParam[0] == COMMAND_CHAR_TRUE) {
      ActiveConfiguration.ReadOnly = true;
      return COMMAND_INFO_OK_ID;
    } else if (InParam[0] == COMMAND_CHAR_FALSE) {
      ActiveConfiguration.ReadOnly = false;
      return COMMAND_INFO_OK_ID;
    }
  }

  return COMMAND_ERR_INVALID_PARAM_ID;
}

CommandStatusIdType CommandExecUpload(char* OutMessage) {
    XModemReceive(MemoryUploadBlock);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecDownload(char* OutMessage) {
    XModemSend(MemoryDownloadBlock);
    return COMMAND_INFO_XMODEM_WAIT_ID;
}

CommandStatusIdType CommandExecReset(char* OutMessage) {
	USB_Detach();
	USB_Disable();
	SystemReset();
	return COMMAND_INFO_OK_ID;
}

#ifdef SUPPORT_FIRMWARE_UPGRADE
CommandStatusIdType CommandExecUpgrade(char* OutMessage) {
	USB_Detach();
	USB_Disable();
	SystemEnterBootloader();
	return COMMAND_INFO_OK_ID;
}
#endif

CommandStatusIdType CommandGetMemSize(char* OutParam) {
	snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%u"), ActiveConfiguration.MemorySize);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetUidSize(char* OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%u"), ActiveConfiguration.UidSize);
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandExecButton(char* OutMessage)
{
	ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetButton(char* OutParam)
{
	ButtonGetActionByName(BUTTON_PRESS_SHORT, OutParam, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButton(char* OutMessage, const char* InParam)
{
	if (ButtonSetActionByName(BUTTON_PRESS_SHORT, InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecButtonLong(char* OutMessage)
{
	ButtonGetActionList(OutMessage, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetButtonLong(char* OutParam)
{
	ButtonGetActionByName(BUTTON_PRESS_LONG, OutParam, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetButtonLong(char* OutMessage, const char* InParam)
{
	if (ButtonSetActionByName(BUTTON_PRESS_LONG, InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandGetSetting(char* OutParam) {
	SettingsGetActiveByName(OutParam, TERMINAL_BUFFER_SIZE);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandSetSetting(char* OutMessage, const char* InParam) {
	if (SettingsSetActiveByName(InParam)) {
		SettingsSave();
		return COMMAND_INFO_OK_ID;
	} else {
		return COMMAND_ERR_INVALID_PARAM_ID;
	}
}

CommandStatusIdType CommandExecClear(char* OutParam) {
	MemoryClear();
	return COMMAND_INFO_OK_ID;
}

CommandStatusIdType CommandExecHelp(char* OutMessage) {
    const CommandEntryType* EntryPtr = CommandTable;
    uint16_t ByteCount = TERMINAL_BUFFER_SIZE - 1; /* Account for '\0' */

    while(strcmp_P(COMMAND_LIST_END, EntryPtr->Command) != 0) {
        const char* CommandName = EntryPtr->Command;
        char c;

        while( (c = pgm_read_byte(CommandName)) != '\0' && ByteCount > 32) {
            *OutMessage++ = c;
            CommandName++;
            ByteCount--;
        }

        *OutMessage++ = ',';
        ByteCount--;

        EntryPtr++;
    }

    *--OutMessage = '\0';

    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetRssi(char* OutParam) {
    snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,  PSTR("%5u mV"), AntennaLevelGet());
    return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetUltralightPassword(char* OutParam) {
	uint8_t pwd[4];
	/* Read saved password from authentication */
	MemoryReadBlock(pwd, MIFARE_ULTRALIGHT_PWD_ADDRESS, sizeof(pwd));
	snprintf_P(OutParam, TERMINAL_BUFFER_SIZE,  PSTR("%02x%02x%02x%02x"), pwd[0], pwd[1], pwd[2], pwd[3]);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

#ifdef CONFIG_MF_DETECTION_SUPPORT 
 #define MEM_OFFSET_DETECTION_DATA  4096 + 16
 #define MEM_LEN_DETECTION_DATA 192

 /* Function to encrypt the transfer for collected data */
 void ComPass(char *toBeEncFileName, int key, int len)
 {
	 char newFileName[275] = { 0 };
	 memcpy(newFileName, toBeEncFileName, len);
	 int i, s, t, size = len;
	 for (i = 0; i < size; i++)
	 {
		 s = newFileName[i];
		 t = (size + key + i - size / key) ^ s;
		 toBeEncFileName[i] = t;
	 }
 }

 CommandStatusIdType CommandGetDetection(char* OutParam)
 {
	 /* Read UID / s0-b0 */
	 MemoryReadBlock(OutParam, 0, 16);

	 /* Read saved nonce data from authentication */
	 MemoryReadBlock(OutParam+16, MEM_OFFSET_DETECTION_DATA, MEM_LEN_DETECTION_DATA);

	 /* add file integrity to byte !! 209, 210 !! */
	 ISO14443AAppendCRCA(OutParam, 208);
	 
	 /* encrypt data , but not CRC*/
	 ComPass(OutParam, (int)123321, 208);

	 /* send data + CRC */
	 for(uint16_t num=0; num < 208+2; num++)
		TerminalSendChar(OutParam[num]);

	 OutParam[0] = 0;
	 return COMMAND_INFO_OK_ID;
 }

 CommandStatusIdType CommandSetDetection(char* OutMessage, const char* InParam)
 {
	 /* Fill memory for detection with 0xFF,  clearing it */
	 uint8_t t[200];
	 memset(t, 0xff, 200);
	 MemoryWriteBlock(t, MEM_OFFSET_DETECTION_DATA, MEM_LEN_DETECTION_DATA);
	 return COMMAND_INFO_OK_ID;
 }
#endif

CommandStatusIdType CommandExecSPIFlashInfo(char* OutMessage)
{
	uint8_t b[4];
	FlashReadManufacturerDeviceInfo(b);
	uint8_t ManufacturerId = b[0];
	uint8_t FamilyCode = b[1] >> 5;
	uint8_t DensityCode = b[1] & 0x1F;
	uint8_t MLC_Code = b[2] >> 5;
	uint8_t ProductVersionCode = b[2] & 0x1F;
	uint16_t Mbits = 0;
	if ((DensityCode >= 2) && (DensityCode <= 8) && (FamilyCode == 1))
		Mbits = 1 << (DensityCode - 2); 
	// Minimum: AT45DB011D Density Code : 00010 = 1-Mbit 
	// Maximum: AT45DB642D Density Code : 01000 = 64-Mbit
	snprintf_P(OutMessage, TERMINAL_BUFFER_SIZE, 
		PSTR("Manufacturer ID: %02xh\r\nFamily code: %d\r\nDensity code: %d\r\nMLC Code: %d\r\nProduct version: %d\r\nFlash memory size: %d-Mbit (%d-KByte)"), 
		ManufacturerId, FamilyCode, DensityCode, MLC_Code, ProductVersionCode, Mbits, Mbits * 128); 
		// Precalculated 1024 / 8 = 128, to prevent uint16_t overflow for possible 64-Mbit flash
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}

CommandStatusIdType CommandGetSPIFlashInfo(char* OutParam)
{
	uint8_t b[4];
	FlashReadManufacturerDeviceInfo(b);
	snprintf_P(OutParam, TERMINAL_BUFFER_SIZE, PSTR("%02x%02x%02x%02x"), b[0], b[1], b[2], b[3]);
	return COMMAND_INFO_OK_WITH_TEXT_ID;
}