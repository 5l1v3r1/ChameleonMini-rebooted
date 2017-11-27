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

#include "MifareClassic.h"

#include "ISO14443-3A.h"
#include "../Codec/ISO14443-2A.h"
#include "../Memory.h"
#include "Crypto1.h"
#include "../Random.h"
#include "../LED.h"
#define MFCLASSIC_1K_ATQA_VALUE     0x0004
#define MFCLASSIC_4K_ATQA_VALUE     0x0002
#define MFCLASSIC_1K_SAK_CL1_VALUE  0x08
#define MFCLASSIC_4K_SAK_CL1_VALUE  0x18

#define MEM_UID_CL1_ADDRESS         0x00
#define MEM_UID_CL1_SIZE            4
#define MEM_UID_BCC1_ADDRESS        0x04
#define MEM_KEY_A_OFFSET            48        /* Bytes */
#define MEM_KEY_B_OFFSET            58        /* Bytes */
#define MEM_KEY_SIZE                6        /* Bytes */
#define MEM_SECTOR_ADDR_MASK        0x3C
#define MEM_BYTES_PER_BLOCK         16        /* Bytes */
#define MEM_VALUE_SIZE              4       /* Bytes */

#define ACK_NAK_FRAME_SIZE          4         /* Bits */
#define ACK_VALUE                   0x0A
#define NAK_INVALID_ARG             0x00
#define NAK_CRC_ERROR               0x01
#define NAK_NOT_AUTHED              0x04
#define NAK_EEPROM_ERROR            0x05
#define NAK_OTHER_ERROR             0x06

#define CMD_AUTH_A                  0x60
#define CMD_AUTH_B                  0x61
#define CMD_AUTH_FRAME_SIZE         2         /* Bytes without CRCA */
#define CMD_AUTH_RB_FRAME_SIZE      4        /* Bytes */
#define CMD_AUTH_AB_FRAME_SIZE      8        /* Bytes */
#define CMD_AUTH_BA_FRAME_SIZE      4        /* Bytes */
#define CMD_HALT                    0x50
#define CMD_HALT_FRAME_SIZE         2        /* Bytes without CRCA */
#define CMD_READ                    0x30
#define CMD_READ_FRAME_SIZE         2         /* Bytes without CRCA */
#define CMD_READ_RESPONSE_FRAME_SIZE 16 /* Bytes without CRCA */
#define CMD_WRITE                   0xA0
#define CMD_WRITE_FRAME_SIZE        2         /* Bytes without CRCA */
#define CMD_DECREMENT               0xC0
#define CMD_DECREMENT_FRAME_SIZE    2         /* Bytes without CRCA */
#define CMD_INCREMENT               0xC1
#define CMD_INCREMENT_FRAME_SIZE    2         /* Bytes without CRCA */
#define CMD_RESTORE                 0xC2
#define CMD_RESTORE_FRAME_SIZE      2         /* Bytes without CRCA */
#define CMD_TRANSFER                0xB0
#define CMD_TRANSFER_FRAME_SIZE     2         /* Bytes without CRCA */

static enum {
    STATE_HALT,
    STATE_IDLE,
    STATE_READY,
    STATE_ACTIVE,
    STATE_AUTHING,
    STATE_AUTHED_IDLE,
    STATE_WRITE,
    STATE_INCREMENT,
    STATE_DECREMENT,
    STATE_RESTORE
} State;

static uint8_t CardResponse[4];
static uint8_t ReaderResponse[4];
static uint8_t CurrentAddress;
static uint8_t BlockBuffer[MEM_BYTES_PER_BLOCK];
static uint16_t CardATQAValue;
static uint8_t CardSAKValue;

INLINE bool CheckValueIntegrity(uint8_t* Block)
{
    /* Value Blocks contain a value stored three times, with
     * the middle portion inverted. */
    if (    (Block[0] == (uint8_t) ~Block[4]) && (Block[0] == Block[8])
         && (Block[1] == (uint8_t) ~Block[5]) && (Block[1] == Block[9])
         && (Block[2] == (uint8_t) ~Block[6]) && (Block[2] == Block[10])
         && (Block[3] == (uint8_t) ~Block[7]) && (Block[3] == Block[11])
         && (Block[12] == (uint8_t) ~Block[13])
         && (Block[12] == Block[14])
         && (Block[14] == (uint8_t) ~Block[15])) {
        return true;
    } else {
        return false;
    }
}

INLINE void ValueFromBlock(uint32_t* Value, uint8_t* Block)
{
    *Value = 0;
    *Value |= ((uint32_t) Block[0] << 0);
    *Value |= ((uint32_t) Block[1] << 8);
    *Value |= ((uint32_t) Block[2] << 16);
    *Value |= ((uint32_t) Block[3] << 24);
}

INLINE void ValueToBlock(uint8_t* Block, uint32_t Value)
{
    Block[0] = (uint8_t) (Value >> 0);
    Block[1] = (uint8_t) (Value >> 8);
    Block[2] = (uint8_t) (Value >> 16);
    Block[3] = (uint8_t) (Value >> 24);
    Block[4] = ~Block[0];
    Block[5] = ~Block[1];
    Block[6] = ~Block[2];
    Block[7] = ~Block[3];
    Block[8] = Block[0];
    Block[9] = Block[1];
    Block[10] = Block[2];
    Block[11] = Block[3];
}

void MifareClassicAppInit1K(void)
{
    State = STATE_IDLE;
    CardATQAValue = MFCLASSIC_1K_ATQA_VALUE;
    CardSAKValue = MFCLASSIC_1K_SAK_CL1_VALUE;
}

void MifareClassicAppInit4K(void)
{
    State = STATE_IDLE;
    CardATQAValue = MFCLASSIC_4K_ATQA_VALUE;
    CardSAKValue = MFCLASSIC_4K_SAK_CL1_VALUE;
}

void MifareClassicAppReset(void)
{
    //State = STATE_IDLE;
}

void MifareClassicAppTask(void)
{

}


uint16_t MifareClassicAppProcess(uint8_t* Buffer, uint16_t BitCount)
{


		//0x26  WUPA?
		if(BitCount==7)
		{
				if (ISO14443AWakeUp(Buffer, &BitCount, CardATQAValue))
				{
            State = STATE_READY;
            return BitCount;
        }
		}
		// 0x93 0x20 ,  0x93 0x70  Anticollision 1
		 if(BitCount==16 || BitCount==72)
		 {
		 	if (Buffer[0] == ISO14443A_CMD_SELECT_CL1)
		 	{
            uint8_t UidCL1[4];
            MemoryReadBlock(UidCL1, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
			 	if (ISO14443ASelect(Buffer, &BitCount, UidCL1, CardSAKValue))
            return BitCount;
		 	}
		 }

		if(State != STATE_AUTHED_IDLE)
		{
		 if(BitCount==32)
		 {
		  if((Buffer[0] == CMD_AUTH_A) || (Buffer[0] == CMD_AUTH_B))
		  {
            if (ISO14443ACheckCRCA(Buffer, CMD_AUTH_FRAME_SIZE)) {
                uint8_t SectorAddress = Buffer[1] & MEM_SECTOR_ADDR_MASK;
                uint8_t KeyOffset = (Buffer[0] == CMD_AUTH_A ? MEM_KEY_A_OFFSET : MEM_KEY_B_OFFSET);
                uint16_t KeyAddress = (uint16_t) SectorAddress * MEM_BYTES_PER_BLOCK + KeyOffset;
                uint8_t Key[6];
                uint8_t Uid[4];
			uint8_t CardNonce[4]={0x01,0x20,0x01,0x45};

                /* Generate a random nonce and read UID and key from memory */
			//RandomGetBuffer(CardNonce, sizeof(CardNonce));
                MemoryReadBlock(Uid, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
                MemoryReadBlock(Key, KeyAddress, MEM_KEY_SIZE);

                /* Precalculate the reader response from card-nonce */
                for (uint8_t i=0; i<sizeof(ReaderResponse); i++)
                    ReaderResponse[i] = CardNonce[i];

                Crypto1PRNG(ReaderResponse, 64);

                /* Precalculate our response from the reader response */
                for (uint8_t i=0; i<sizeof(CardResponse); i++)
                    CardResponse[i] = ReaderResponse[i];

                Crypto1PRNG(CardResponse, 32);

                /* Respond with the random card nonce and expect further authentication
                * form the reader in the next frame. */
                State = STATE_AUTHING;

                for (uint8_t i=0; i<sizeof(CardNonce); i++)
                    Buffer[i] = CardNonce[i];

                /* Setup crypto1 cipher. Discard in-place encrypted CardNonce. */
                Crypto1Setup(Key, Uid, CardNonce);

                return CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
			}
			}
		 }
			////����8λ����
			if(BitCount==64 && State==STATE_AUTHING)
			{
        Crypto1Auth(&Buffer[0]);

        for (uint8_t i=0; i<4; i++)
            Buffer[i+4] ^= Crypto1Byte();

        if ((Buffer[4] == ReaderResponse[0]) &&
            (Buffer[5] == ReaderResponse[1]) &&
            (Buffer[6] == ReaderResponse[2]) &&
			(Buffer[7] == ReaderResponse[3])) 
			{
            /* Reader is authenticated. Encrypt the precalculated card response
            * and generate the parity bits. */
            for (uint8_t i=0; i<sizeof(CardResponse); i++) {
                Buffer[i] = CardResponse[i] ^ Crypto1Byte();
                Buffer[ISO14443A_BUFFER_PARITY_OFFSET + i] = ODD_PARITY(CardResponse[i]) ^ Crypto1FilterOutput();
            }

            State = STATE_AUTHED_IDLE;
			//authed_falg=1;
            return (CMD_AUTH_BA_FRAME_SIZE * BITS_PER_BYTE) | ISO14443A_APP_CUSTOM_PARITY;
			}
			}
		 }
		 else
		 {
			//�ٴ���
		if(BitCount==32)
		 {
		//�ȷ���
		for (uint8_t i=0; i<BitCount/8; i++)
            Buffer[i] ^= Crypto1Byte();


        if (Buffer[0] == CMD_READ) {
            if (ISO14443ACheckCRCA(Buffer, CMD_READ_FRAME_SIZE)) {
                /* Read command. Read data from memory and append CRCA. */
                MemoryReadBlock(Buffer, (uint16_t) Buffer[1] * MEM_BYTES_PER_BLOCK, MEM_BYTES_PER_BLOCK);
                ISO14443AAppendCRCA(Buffer, MEM_BYTES_PER_BLOCK);
                /* Encrypt and calculate parity bits. */
                for (uint8_t i=0; i<(ISO14443A_CRCA_SIZE + MEM_BYTES_PER_BLOCK); i++) {
                    uint8_t Plain = Buffer[i];
                    Buffer[i] = Plain ^ Crypto1Byte();
                    Buffer[ISO14443A_BUFFER_PARITY_OFFSET + i] = ODD_PARITY(Plain) ^ Crypto1FilterOutput();
                }
                return ( (CMD_READ_RESPONSE_FRAME_SIZE + ISO14443A_CRCA_SIZE )
                        * BITS_PER_BYTE) | ISO14443A_APP_CUSTOM_PARITY;
		}
		}

         if (Buffer[0] == CMD_WRITE) {
            if (ISO14443ACheckCRCA(Buffer, CMD_WRITE_FRAME_SIZE)) {
                /* Write command. Store the address and prepare for the upcoming data.
                * Respond with ACK. */
                CurrentAddress = Buffer[1];
                State = STATE_WRITE;
                Buffer[0] = ACK_VALUE ^ Crypto1Nibble();
            } else {
                Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
            }
            return ACK_NAK_FRAME_SIZE;
			}

			else if (Buffer[0] == CMD_DECREMENT) {
            if (ISO14443ACheckCRCA(Buffer, CMD_DECREMENT_FRAME_SIZE)) {
                CurrentAddress = Buffer[1];
                State = STATE_DECREMENT;
                Buffer[0] = ACK_VALUE ^ Crypto1Nibble();
            } else {
                Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
            }
            return ACK_NAK_FRAME_SIZE;
			}
			else if (Buffer[0] == CMD_INCREMENT) {
            if (ISO14443ACheckCRCA(Buffer, CMD_DECREMENT_FRAME_SIZE)) {
                CurrentAddress = Buffer[1];
                State = STATE_INCREMENT;
                Buffer[0] = ACK_VALUE ^ Crypto1Nibble();
            } else {
                Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
            }
            return ACK_NAK_FRAME_SIZE;
			} 
			else if (Buffer[0] == CMD_RESTORE) {
            if (ISO14443ACheckCRCA(Buffer, CMD_DECREMENT_FRAME_SIZE)) {
                CurrentAddress = Buffer[1];
                State = STATE_RESTORE;
                Buffer[0] = ACK_VALUE ^ Crypto1Nibble();
            } else {
                Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
            }
            return ACK_NAK_FRAME_SIZE;
			}
			else if (Buffer[0] == CMD_TRANSFER) {
            /* Write back the global block buffer to the desired block address */
            if (ISO14443ACheckCRCA(Buffer, CMD_TRANSFER_FRAME_SIZE)) {
                if (!ActiveConfiguration.ReadOnly) {
                    MemoryWriteBlock(BlockBuffer, (uint16_t) Buffer[1] * MEM_BYTES_PER_BLOCK, MEM_BYTES_PER_BLOCK );
                } else {
                    /* In read only mode, silently ignore the write */
                }

                Buffer[0] = ACK_VALUE ^ Crypto1Nibble();
            } else {
                Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
            }

            return ACK_NAK_FRAME_SIZE;
			}

		  if((Buffer[0] == CMD_AUTH_A) || (Buffer[0] == CMD_AUTH_B))
		  {
            if (ISO14443ACheckCRCA(Buffer, CMD_AUTH_FRAME_SIZE)) {
                uint8_t SectorAddress = Buffer[1] & MEM_SECTOR_ADDR_MASK;
                uint8_t KeyOffset = (Buffer[0] == CMD_AUTH_A ? MEM_KEY_A_OFFSET : MEM_KEY_B_OFFSET);
                uint16_t KeyAddress = (uint16_t) SectorAddress * MEM_BYTES_PER_BLOCK + KeyOffset;
                uint8_t Key[6];
                uint8_t Uid[4];
			uint8_t CardNonce[4]={0x01};

                /* Generate a random nonce and read UID and key from memory */
			//RandomGetBuffer(CardNonce, sizeof(CardNonce));
                MemoryReadBlock(Uid, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
                MemoryReadBlock(Key, KeyAddress, MEM_KEY_SIZE);

                /* Precalculate the reader response from card-nonce */
                for (uint8_t i=0; i<sizeof(ReaderResponse); i++)
                    ReaderResponse[i] = CardNonce[i];

                Crypto1PRNG(ReaderResponse, 64);

                /* Precalculate our response from the reader response */
                for (uint8_t i=0; i<sizeof(CardResponse); i++)
                    CardResponse[i] = ReaderResponse[i];

                Crypto1PRNG(CardResponse, 32);

			/* Respond with the random card nonce and expect further authentication
			* form the reader in the next frame. */
			State = STATE_AUTHING;

			/* Setup crypto1 cipher. Discard in-place encrypted CardNonce. */
                Crypto1Setup(Key, Uid, CardNonce);

                for (uint8_t i=0; i<sizeof(CardNonce); i++)
                    Buffer[i] = CardNonce[i];

 

                return CMD_AUTH_RB_FRAME_SIZE * BITS_PER_BYTE;
			}
			}
		  }
		 }

		 	if( State == STATE_WRITE)
		 	{
        /* We receive 16 bytes of data to be written and 2 bytes CRCA. Decrypt */
        for (uint8_t i=0; i<(MEM_BYTES_PER_BLOCK + ISO14443A_CRCA_SIZE); i++)
            Buffer[i] ^= Crypto1Byte();

        if (ISO14443ACheckCRCA(Buffer, MEM_BYTES_PER_BLOCK)) {
            if (!ActiveConfiguration.ReadOnly) {
                MemoryWriteBlock(Buffer, CurrentAddress * MEM_BYTES_PER_BLOCK, MEM_BYTES_PER_BLOCK);
            } else {
                /* Silently ignore in ReadOnly mode */
            }

            Buffer[0] = ACK_VALUE ^ Crypto1Nibble();
        } else {
            Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
        }

        State = STATE_AUTHED_IDLE;
        return ACK_NAK_FRAME_SIZE;
		 	}

			if(State==STATE_DECREMENT || State==STATE_INCREMENT|| State==STATE_RESTORE)  
			{
 
        for (uint8_t i=0; i<(MEM_VALUE_SIZE  + ISO14443A_CRCA_SIZE); i++)
            Buffer[i] ^= Crypto1Byte();

        if (ISO14443ACheckCRCA(Buffer, MEM_VALUE_SIZE )) {
            MemoryReadBlock(BlockBuffer, (uint16_t) CurrentAddress * MEM_BYTES_PER_BLOCK, MEM_BYTES_PER_BLOCK);

            if (CheckValueIntegrity(BlockBuffer)) {
                uint32_t ParamValue;
                uint32_t BlockValue;

                ValueFromBlock(&ParamValue, Buffer);
                ValueFromBlock(&BlockValue, BlockBuffer);

                if (State == STATE_DECREMENT) {
                    BlockValue -= ParamValue;
                } else if (State == STATE_INCREMENT) {
                    BlockValue += ParamValue;
                } else if (State == STATE_RESTORE) {
                    /* Do nothing */
                }

                ValueToBlock(BlockBuffer, BlockValue);

                State = STATE_AUTHED_IDLE;
                /* No ACK response on value commands part 2 */
                return ISO14443A_APP_NO_RESPONSE;
            } else {
                /* Not sure if this is the correct error code.. */
                Buffer[0] = NAK_OTHER_ERROR ^ Crypto1Nibble();
            }
        } else {
            /* CRC Error. */
            Buffer[0] = NAK_CRC_ERROR ^ Crypto1Nibble();
        }

        State = STATE_AUTHED_IDLE;
        return ACK_NAK_FRAME_SIZE;
    }

    /* No response has been sent, when we reach here */
    return ISO14443A_APP_NO_RESPONSE;


}

void MifareClassicGetUid(ConfigurationUidType Uid)
{
    MemoryReadBlock(Uid, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
}

void MifareClassicSetUid(ConfigurationUidType Uid)
{
    uint8_t BCC =  Uid[0] ^ Uid[1] ^ Uid[2] ^ Uid[3];

    MemoryWriteBlock(Uid, MEM_UID_CL1_ADDRESS, MEM_UID_CL1_SIZE);
    MemoryWriteBlock(&BCC, MEM_UID_BCC1_ADDRESS, ISO14443A_CL_BCC_SIZE);
}



