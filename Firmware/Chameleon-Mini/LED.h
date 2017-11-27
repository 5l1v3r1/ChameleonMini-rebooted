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

#ifndef LED_H
#define LED_H

#include <avr/io.h>

#define LED_PORT PORTA
#define LED_GREEN	PIN5_bm
#define LED_RED		PIN4_bm
#define LED_MASK	(LED_GREEN | LED_RED)
//#define LED_PORT PORTE
//#define LED_GREEN	PIN2_bm
//#define LED_RED		PIN3_bm
//#define LED_MASK	(LED_GREEN | LED_RED)

#define LED_HEIGH_PORT  PORTA
#define LED_ONE	        PIN5_bm
#define LED_TWO		    PIN4_bm
#define LED_THREE		PIN3_bm
#define LED_FOUR		PIN2_bm
#define LED_HEIGH_MASK	(LED_ONE | LED_TWO | LED_THREE| LED_FOUR)

#define LED_LOW_PORT    PORTE
#define LED_FIVE	    PIN3_bm
#define LED_SIX		    PIN2_bm
#define LED_SEVEN		PIN1_bm
#define LED_EIGHT		PIN0_bm
#define LED_LOW_MASK	(LED_FIVE | LED_SIX | LED_SEVEN| LED_EIGHT)

static inline
void CARDInit(void) {
	LED_HEIGH_PORT.DIRSET = LED_HEIGH_MASK;
	LED_LOW_PORT.DIRSET = LED_LOW_MASK;
}

static inline
void CARDHEIGHSetOn(uint8_t Mask) {
	LED_HEIGH_PORT.OUTSET = Mask;
}

static inline
void CARDLOWSetOn(uint8_t Mask) {
	LED_LOW_PORT.OUTSET = Mask;
}

static inline
void CARDHEIGHSetOff(uint8_t Mask) {
	LED_HEIGH_PORT.OUTCLR = Mask;
}

static inline
void CARDLOWSetOff(uint8_t Mask) {
	LED_LOW_PORT.OUTCLR = Mask;
}



extern uint8_t LEDPulseMask;

static inline
void LEDInit(void) {
    LED_PORT.DIRSET = LED_MASK;
}

static inline
void LEDSetOn(uint8_t Mask) {
    LED_PORT.OUTSET = Mask;
}

static inline
void LEDSetOff(uint8_t Mask) {
    LED_PORT.OUTCLR = Mask;
}

static inline
void LEDToggle(uint8_t Mask) {
    LED_PORT.OUTTGL = Mask;
}

static inline
void LEDPulse(uint8_t Mask) {
	LEDPulseMask = Mask;
	LED_PORT.OUTSET = Mask;
}

static inline
void LEDTick(void) {
	LED_PORT.OUTCLR = LEDPulseMask;
	LEDPulseMask = 0;
}

#endif /* LED_H */
