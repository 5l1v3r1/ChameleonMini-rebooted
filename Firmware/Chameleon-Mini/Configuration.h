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

#ifndef STANDARDS_H_
#define STANDARDS_H_

#include <stdint.h>
#include <stdbool.h>

#define CONFIGURATION_NAME_LENGTH_MAX   16
#define CONFIGURATION_UID_SIZE_MAX      16

typedef uint8_t ConfigurationUidType[CONFIGURATION_UID_SIZE_MAX];

typedef enum  {
    /* This HAS to be the first element */
    CONFIG_NONE = 0,

#ifdef CONFIG_MF_ULTRALIGHT_SUPPORT
    CONFIG_MF_ULTRALIGHT,
#endif
#ifdef CONFIG_MF_CLASSIC_1K_SUPPORT
    CONFIG_MF_CLASSIC_1K,
#endif
#ifdef CONFIG_MF_CLASSIC_4K_SUPPORT
    CONFIG_MF_CLASSIC_4K,
#endif

#ifdef CONFIG_MF_DETECTION_SUPPORT
CONFIG_MF_DETECTION,
#endif

#ifdef CONFIG_ISO15693_GEN_SUPPORT
    CONFIG_ISO15693_GEN,
#endif
#ifdef CONFIG_ISO14443A_SNIFF_SUPPORT
    CONFIG_ISO14443A_SNIFF,
#endif
#ifdef CONFIG_ISO15693_SNIFF_SUPPORT
    CONFIG_ISO15693_SNIFF,
#endif


    //CONFIG_MF_DESFIRE,
    //CONFIG_ISO14443A_RELAY



    /* This HAS to be the last element */
    CONFIG_COUNT
} ConfigurationEnum;

typedef struct {
    ConfigurationEnum ConfigurationID;
    char ConfigurationName[CONFIGURATION_NAME_LENGTH_MAX];

    /* Codec used for this configuration */
    void (*CodecInitFunc) (void);
    void (*CodecTaskFunc) (void);

    /* Application used for this configuration */
    void (*ApplicationInitFunc) (void);
    void (*ApplicationResetFunc) (void);
    void (*ApplicationTaskFunc) (void);
    uint16_t (*ApplicationProcessFunc) (uint8_t* ByteBuffer, uint16_t ByteCount);
    void (*ApplicationGetUidFunc) (ConfigurationUidType Uid);
    void (*ApplicationSetUidFunc) (ConfigurationUidType Uid);

    uint16_t MemorySize;
    uint8_t UidSize;
    bool ReadOnly;

} ConfigurationType;

extern ConfigurationType ActiveConfiguration;

void ConfigurationInit(void);
void ConfigurationSetById(ConfigurationEnum Configuration);
bool ConfigurationSetByName(const char* ConfigurationName);
void ConfigurationGetList(char* ConfigListOut, uint16_t ByteCount);

#endif /* STANDARDS_H_ */
