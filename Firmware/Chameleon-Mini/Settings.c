/* Copyright 2013 Timo Kasper, Simon K�ppers, David Oswald ("ORIGINAL
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

#include "Settings.h"
#include <avr/eeprom.h>
#include "Configuration.h"
#include  "LED.h"
#include <string.h>
SettingsType GlobalSettings=
{
	.ActiveSetting = DEFAULT_SETTING,
	.ActiveSettingPtr = &GlobalSettings.Settings[DEFAULT_SETTING],

	.Settings = { [0 ... (SETTINGS_COUNT-1)] =	{
		.Configuration = DEFAULT_CONFIGURATION,
		.ButtonAction =	DEFAULT_BUTTON_ACTION,
	} }
};
SettingsType EEMEM StoredSettings = {
	.ActiveSetting = DEFAULT_SETTING,
	.ActiveSettingPtr = &GlobalSettings.Settings[DEFAULT_SETTING],

	.Settings = { [0 ... (SETTINGS_COUNT-1)] =	{
		.Configuration = DEFAULT_CONFIGURATION,
		.ButtonAction =	DEFAULT_BUTTON_ACTION,
	} }
};

void SettingsLoad(void) {
	eeprom_read_block(&GlobalSettings, &StoredSettings, sizeof(SettingsType));
}

void SettingsSave(void) {

	uint8_t temp[35];
	memcpy(temp,&GlobalSettings,sizeof(SettingsType));
	ISO14443AAppendCRCA(temp,sizeof(SettingsType));
	Write_Save(temp, 32*1024,sizeof(SettingsType)+2);
	
}

void SettingsCycle(void) {
	char i = SETTINGS_COUNT;
	uint8_t Setting = GlobalSettings.ActiveSetting;

	while (i-- > 0) {
		Setting = (Setting + 1) % SETTINGS_COUNT;

		if (GlobalSettings.Settings[Setting].Configuration != CONFIG_NONE) {
 
			 	if (Setting < SETTINGS_COUNT) {
				 	GlobalSettings.ActiveSetting = Setting;
				 	GlobalSettings.ActiveSettingPtr =
				 	&GlobalSettings.Settings[GlobalSettings.ActiveSetting];

			 	}
			break;
		}
	}
}

void SettingsSetActiveById(uint8_t Setting) {
	if (Setting < SETTINGS_COUNT) {
		GlobalSettings.ActiveSetting = Setting;
		GlobalSettings.ActiveSettingPtr =
				&GlobalSettings.Settings[GlobalSettings.ActiveSetting];

		/* Settings have changed. Progress changes through system */
		ConfigurationInit();
	}
}

uint8_t SettingsGetActiveById(void) {
	return GlobalSettings.ActiveSetting;
}

void SettingsGetActiveByName(char* SettingOut, uint16_t BufferSize) {
	SettingOut[0] = 'N';
	SettingOut[1] = 'O';
	SettingOut[2] = '.';	
	SettingOut[3] = SettingsGetActiveById() + '0';
	SettingOut[4] = '\0';
}

bool SettingsSetActiveByName(const char* Setting) {
	uint8_t SettingNr = Setting[0] - '0';

	if ((Setting[1] == '\0') && (SettingNr < SETTINGS_COUNT)) {
		SettingsSetActiveById(SettingNr);
		return true;
	} else {
		return false;
	}
}

