//**************************************************************//
// Teensy 4.1 8080 Parallel 8/16 bit with 8 bit ASYNC support.
//**************************************************************//
/*
 * Ra8876LiteTeensy.cpp
 * Modified Version of: File Name : RA8876_t41_p.cpp
 *			Author    : RAiO Application Team
 *			Edit Date : 09/13/2017
 *			Version   : v2.0  1.modify bte_DestinationMemoryStartAddr bug
 *                 			  2.modify ra8876SdramInitial Auto_Refresh
 *                 			  3.modify ra8876PllInitial
 ****************************************************************
 * 	  	              : New 8080 Parallel version
 *                    : For MicroMod
 *                    : By Warren Watson
 *                    : 06/07/2018 - 05/03/2024
 *                    : Copyright (c) 2017-2024 Warren Watson.
 *****************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
//**************************************************************//

#include "RA8876_t41_p.h"
#include "Arduino.h"

//**************************************************************//
// RA8876_t41_p()
//**************************************************************//
// Create RA8876 driver instance 8080 IF
//**************************************************************//
RA8876_t41_p::RA8876_t41_p(const uint8_t DCp, const uint8_t CSp, const uint8_t RSTp) {
    _cs = CSp;
    _rst = RSTp;
    _dc = DCp;
    RA8876_GFX(BUS_WIDTH);
}

FLASHMEM boolean RA8876_t41_p::begin(uint8_t baud_div) {
    switch (baud_div) {
    case 2:
        _baud_div = 120;
        break;
    case 4:
        _baud_div = 60;
        break;
    case 8:
        _baud_div = 30;
        break;
    case 12:
        _baud_div = 20;
        break;
    case 20:
        _baud_div = 12;
        break;
    case 24:
        _baud_div = 10;
        break;
    case 30:
        _baud_div = 8;
        break;
    case 40:
        _baud_div = 6;
        break;
    case 60:
        _baud_div = 4;
        break;
    case 120:
        _baud_div = 2;
        break;
    default:
        _baud_div = 20; // 12Mhz
        break;
    }
    pinMode(_cs, OUTPUT);  // CS
    pinMode(_dc, OUTPUT);  // DC
    pinMode(_rst, OUTPUT); // RST
    *(portControlRegister(_cs)) = 0xFF;
    *(portControlRegister(_dc)) = 0xFF;
    *(portControlRegister(_rst)) = 0xFF;

    digitalWriteFast(_cs, HIGH);
    digitalWriteFast(_dc, HIGH);
    digitalWriteFast(_rst, HIGH);

    delay(15);
    digitalWrite(_rst, LOW);
    delay(15);
    digitalWriteFast(_rst, HIGH);
    delay(100);

    FlexIO_Init();

    if (!checkIcReady())
        return false;

    // read ID code must disable pll, 01h bit7 set 0
    if (BUS_WIDTH == 16) {
        lcdRegDataWrite(0x01, 0x09);
    } else {
        lcdRegDataWrite(0x01, 0x08);
    }
    delay(1);
    if ((lcdRegDataRead(0xff) != 0x76) && (lcdRegDataRead(0xff) != 0x77)) {
        return false;
    }

    // Initialize RA8876 to default settings
    if (!ra8876Initialize()) {
        return false;
    }

    return true;
}

FASTRUN void RA8876_t41_p::CSLow() {
    digitalWriteFast(_cs, LOW); // Select TFT
}

FASTRUN void RA8876_t41_p::CSHigh() {
    digitalWriteFast(_cs, HIGH); // Deselect TFT
}

FASTRUN void RA8876_t41_p::DCLow() {
    digitalWriteFast(_dc, LOW); // Writing command to TFT
}

FASTRUN void RA8876_t41_p::DCHigh() {
    digitalWriteFast(_dc, HIGH); // Writing data to TFT
}

FASTRUN void RA8876_t41_p::microSecondDelay() {
    for (uint32_t i = 0; i < 99; i++)
        __asm__("nop\n\t");
}

FASTRUN void RA8876_t41_p::FlexIO_Init() {
    /* Get a FlexIO channel */
    pFlex = FlexIOHandler::flexIOHandler_list[2]; // use FlexIO3
    /* Pointer to the port structure in the FlexIO channel */
    p = &pFlex->port();
    /* Pointer to the hardware structure in the FlexIO channel */
    hw = &pFlex->hardware();
    /* Basic pin setup */

    pinMode(19, OUTPUT); // FlexIO3:0 D0
    pinMode(18, OUTPUT); // FlexIO3:1 |
    pinMode(14, OUTPUT); // FlexIO3:2 |
    pinMode(15, OUTPUT); // FlexIO3:3 |
    pinMode(40, OUTPUT); // FlexIO3:4 |
    pinMode(41, OUTPUT); // FlexIO3:5 |
    pinMode(17, OUTPUT); // FlexIO3:6 |
    pinMode(16, OUTPUT); // FlexIO3:7 D7

#if (BUS_WIDTH == 16)
    pinMode(22, OUTPUT); // FlexIO3:8 D8
    pinMode(23, OUTPUT); // FlexIO3:9  |
    pinMode(20, OUTPUT); // FlexIO3:10 |
    pinMode(21, OUTPUT); // FlexIO3:11 |
    pinMode(38, OUTPUT); // FlexIO3:12 |
    pinMode(39, OUTPUT); // FlexIO3:13 |
    pinMode(26, OUTPUT); // FlexIO3:14 |
    pinMode(27, OUTPUT); // FlexIO3:15 D15
#endif

    pinMode(RD_PIN, OUTPUT);
    digitalWriteFast(RD_PIN, HIGH);
    pinMode(WR_PIN, OUTPUT);
    digitalWriteFast(WR_PIN, HIGH);

    /* High speed and drive strength configuration */
    *(portControlRegister(WR_PIN)) = 0xFF;
    //    *(portControlRegister(RD_PIN)) = 0xFF;

    *(portControlRegister(19)) = 0xFF;
    *(portControlRegister(18)) = 0xFF;
    *(portControlRegister(14)) = 0xFF;
    *(portControlRegister(15)) = 0xFF;
    *(portControlRegister(40)) = 0xFF;
    *(portControlRegister(41)) = 0xFF;
    *(portControlRegister(17)) = 0xFF;
    *(portControlRegister(16)) = 0xFF;

#if (BUS_WIDTH == 16)
    *(portControlRegister(22)) = 0xFF;
    *(portControlRegister(23)) = 0xFF;
    *(portControlRegister(20)) = 0xFF;
    *(portControlRegister(21)) = 0xFF;
    *(portControlRegister(38)) = 0xFF;
    *(portControlRegister(39)) = 0xFF;
    *(portControlRegister(26)) = 0xFF;
    *(portControlRegister(27)) = 0xFF;
#endif

    /* Set clock */
    pFlex->setClockSettings(3, 1, 0); // (480 MHz source, 1+1, 1+0) >> 480/2/1 >> 240Mhz

    /* Set up pin mux */
    pFlex->setIOPinToFlexMode(WR_PIN);
    //    pFlex->setIOPinToFlexMode(RD_PIN);

    pFlex->setIOPinToFlexMode(19);
    pFlex->setIOPinToFlexMode(18);
    pFlex->setIOPinToFlexMode(14);
    pFlex->setIOPinToFlexMode(15);
    pFlex->setIOPinToFlexMode(40);
    pFlex->setIOPinToFlexMode(41);
    pFlex->setIOPinToFlexMode(17);
    pFlex->setIOPinToFlexMode(16);

#if (BUS_WIDTH == 16)
    pFlex->setIOPinToFlexMode(22);
    pFlex->setIOPinToFlexMode(23);
    pFlex->setIOPinToFlexMode(20);
    pFlex->setIOPinToFlexMode(21);
    pFlex->setIOPinToFlexMode(38);
    pFlex->setIOPinToFlexMode(39);
    pFlex->setIOPinToFlexMode(26);
    pFlex->setIOPinToFlexMode(27);
#endif

    hw->clock_gate_register |= hw->clock_gate_mask;
    /* Enable the FlexIO with fast access */
    p->CTRL = FLEXIO_CTRL_FLEXEN;
}

FASTRUN void RA8876_t41_p::FlexIO_Config_SnglBeat_Read() {
    //    pFlex->setIOPinToFlexMode(37);

    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;
    /* Configure the shifters */
    p->SHIFTCFG[3] =
        FLEXIO_SHIFTCFG_SSTOP(0)                 /* Shifter stop bit disabled */
        | FLEXIO_SHIFTCFG_SSTART(0)              /* Shifter start bit disabled and loading data on enabled */
        | FLEXIO_SHIFTCFG_PWIDTH(BUS_WIDTH - 1); /* Bus width */

    p->SHIFTCTL[3] =
        FLEXIO_SHIFTCTL_TIMSEL(0)      /* Shifter's assigned timer index */
        | FLEXIO_SHIFTCTL_TIMPOL * (1) /* Shift on negative edge of shift clock */
        | FLEXIO_SHIFTCTL_PINCFG(1)    /* Shifter's pin configured as output */
        | FLEXIO_SHIFTCTL_PINSEL(0)    /* Shifter's pin start index */
        | FLEXIO_SHIFTCTL_PINPOL * (0) /* Shifter's pin active high */
        | FLEXIO_SHIFTCTL_SMOD(1);     /* Shifter mode as receive */

    /* Configure the timer for shift clock */
    p->TIMCMP[0] =
        (((1 * 2) - 1) << 8) /* TIMCMP[15:8] = number of beats x 2 – 1 */
        | ((60 / 2) - 1);    /* TIMCMP[7:0] = baud rate divider / 2 – 1 */

    p->TIMCFG[0] =
        FLEXIO_TIMCFG_TIMOUT(0)       /* Timer output logic one when enabled and not affected by reset */
        | FLEXIO_TIMCFG_TIMDEC(0)     /* Timer decrement on FlexIO clock, shift clock equals timer output */
        | FLEXIO_TIMCFG_TIMRST(0)     /* Timer never reset */
        | FLEXIO_TIMCFG_TIMDIS(2)     /* Timer disabled on timer compare */
        | FLEXIO_TIMCFG_TIMENA(2)     /* Timer enabled on trigger high */
        | FLEXIO_TIMCFG_TSTOP(1)      /* Timer stop bit enabled */
        | FLEXIO_TIMCFG_TSTART * (0); /* Timer start bit disabled */

    p->TIMCTL[0] =
        FLEXIO_TIMCTL_TRGSEL((((3) << 2) | 1)) /* Timer trigger selected as shifter's status flag */
        | FLEXIO_TIMCTL_TRGPOL * (1)           /* Timer trigger polarity as active low */
        | FLEXIO_TIMCTL_TRGSRC * (1)           // 1                                              /* Timer trigger source as internal */
        | FLEXIO_TIMCTL_PINCFG(3)              /* Timer' pin configured as output */
        | FLEXIO_TIMCTL_PINSEL(19)             /* Timer' pin index: RD pin */
        | FLEXIO_TIMCTL_PINPOL * (1)           /* Timer' pin active low */
        | FLEXIO_TIMCTL_TIMOD(1);              /* Timer mode as dual 8-bit counters baud/bit */

    /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;
}

FASTRUN void RA8876_t41_p::FlexIO_Config_SnglBeat() {
    pFlex->setIOPinToFlexMode(36);

    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    /* Configure the shifters */
    p->SHIFTCFG[0] =
        FLEXIO_SHIFTCFG_INSRC * (1)              /* Shifter input */
        | FLEXIO_SHIFTCFG_SSTOP(0)               /* Shifter stop bit disabled */
        | FLEXIO_SHIFTCFG_SSTART(0)              /* Shifter start bit disabled and loading data on enabled */
        | FLEXIO_SHIFTCFG_PWIDTH(BUS_WIDTH - 1); // Was 7 for 8 bit bus         /* Bus width */

    p->SHIFTCTL[0] =
        FLEXIO_SHIFTCTL_TIMSEL(0)      /* Shifter's assigned timer index */
        | FLEXIO_SHIFTCTL_TIMPOL * (0) /* Shift on posedge of shift clock */
        | FLEXIO_SHIFTCTL_PINCFG(3)    /* Shifter's pin configured as output */
        | FLEXIO_SHIFTCTL_PINSEL(0)    /* Shifter's pin start index */
        | FLEXIO_SHIFTCTL_PINPOL * (0) /* Shifter's pin active high */
        | FLEXIO_SHIFTCTL_SMOD(2);     /* Shifter mode as transmit */

    /* Configure the timer for shift clock */
    p->TIMCMP[0] =
        (((1 * 2) - 1) << 8)     /* TIMCMP[15:8] = number of beats x 2 – 1 */
        | ((_baud_div / 2) - 1); /* TIMCMP[7:0] = baud rate divider / 2 – 1 */

    p->TIMCFG[0] =
        FLEXIO_TIMCFG_TIMOUT(0)       /* Timer output logic one when enabled and not affected by reset */
        | FLEXIO_TIMCFG_TIMDEC(0)     /* Timer decrement on FlexIO clock, shift clock equals timer output */
        | FLEXIO_TIMCFG_TIMRST(0)     /* Timer never reset */
        | FLEXIO_TIMCFG_TIMDIS(2)     /* Timer disabled on timer compare */
        | FLEXIO_TIMCFG_TIMENA(2)     /* Timer enabled on trigger high */
        | FLEXIO_TIMCFG_TSTOP(0)      /* Timer stop bit disabled */
        | FLEXIO_TIMCFG_TSTART * (0); /* Timer start bit disabled */

    p->TIMCTL[0] =
        FLEXIO_TIMCTL_TRGSEL((((0) << 2) | 1)) /* Timer trigger selected as shifter's status flag */
        | FLEXIO_TIMCTL_TRGPOL * (1)           /* Timer trigger polarity as active low */
        | FLEXIO_TIMCTL_TRGSRC * (1)           /* Timer trigger source as internal */
        | FLEXIO_TIMCTL_PINCFG(3)              /* Timer' pin configured as output */
        | FLEXIO_TIMCTL_PINSEL(18)             /* Timer' pin index: WR pin */
        | FLEXIO_TIMCTL_PINPOL * (1)           /* Timer' pin active low */
        | FLEXIO_TIMCTL_TIMOD(1);              /* Timer mode as dual 8-bit counters baud/bit */

    /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;
}

FASTRUN void RA8876_t41_p::FlexIO_Clear_Config_SnglBeat() {
    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    p->SHIFTCFG[0] = 0;
    p->SHIFTCTL[0] = 0;
    p->SHIFTSTAT = (1 << 0);
    p->TIMCMP[0] = 0;
    p->TIMCFG[0] = 0;
    p->TIMSTAT = (1U << 0); /* Timer start bit disabled */
    p->TIMCTL[0] = 0;

    /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;
}

FASTRUN void RA8876_t41_p::FlexIO_Config_MultiBeat() {
    uint8_t beats = SHIFTNUM * BEATS_PER_SHIFTER; // Number of beats = number of shifters * beats per shifter
    pFlex->setIOPinToFlexMode(WR_PIN);

    /* Disable and reset FlexIO */
    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    /* Configure the shifters */
    for (int i = 0; i <= SHIFTNUM - 1; i++) {
        p->SHIFTCFG[i] =
            FLEXIO_SHIFTCFG_INSRC * (1U)             /* Shifter input from next shifter's output */
            | FLEXIO_SHIFTCFG_SSTOP(0U)              /* Shifter stop bit disabled */
            | FLEXIO_SHIFTCFG_SSTART(0U)             /* Shifter start bit disabled and loading data on enabled */
            | FLEXIO_SHIFTCFG_PWIDTH(BUS_WIDTH - 1); /* 8 bit shift width */
    }

    p->SHIFTCTL[0] =
        FLEXIO_SHIFTCTL_TIMSEL(0)       /* Shifter's assigned timer index */
        | FLEXIO_SHIFTCTL_TIMPOL * (0U) /* Shift on posedge of shift clock */
        | FLEXIO_SHIFTCTL_PINCFG(3U)    /* Shifter's pin configured as output */
        | FLEXIO_SHIFTCTL_PINSEL(0)     /* Shifter's pin start index */
        | FLEXIO_SHIFTCTL_PINPOL * (0U) /* Shifter's pin active high */
        | FLEXIO_SHIFTCTL_SMOD(2U);     /* shifter mode transmit */

    for (int i = 1; i <= SHIFTNUM - 1; i++) {
        p->SHIFTCTL[i] =
            FLEXIO_SHIFTCTL_TIMSEL(0)       /* Shifter's assigned timer index */
            | FLEXIO_SHIFTCTL_TIMPOL * (0U) /* Shift on posedge of shift clock */
            | FLEXIO_SHIFTCTL_PINCFG(0U)    /* Shifter's pin configured as output disabled */
            | FLEXIO_SHIFTCTL_PINSEL(0)     /* Shifter's pin start index */
            | FLEXIO_SHIFTCTL_PINPOL * (0U) /* Shifter's pin active high */
            | FLEXIO_SHIFTCTL_SMOD(2U);
    }
    /* Configure the timer for shift clock */
    p->TIMCMP[0] =
        ((beats * 2U - 1) << 8)  /* TIMCMP[15:8] = number of beats x 2 – 1 */
        | (_baud_div / 2U - 1U); /* TIMCMP[7:0] = shift clock divide ratio / 2 - 1 */

    p->TIMCFG[0] = FLEXIO_TIMCFG_TIMOUT(0U)       /* Timer output logic one when enabled and not affected by reset */
                   | FLEXIO_TIMCFG_TIMDEC(0U)     /* Timer decrement on FlexIO clock, shift clock equals timer output */
                   | FLEXIO_TIMCFG_TIMRST(0U)     /* Timer never reset */
                   | FLEXIO_TIMCFG_TIMDIS(2U)     /* Timer disabled on timer compare */
                   | FLEXIO_TIMCFG_TIMENA(2U)     /* Timer enabled on trigger high */
                   | FLEXIO_TIMCFG_TSTOP(0U)      /* Timer stop bit disabled */
                   | FLEXIO_TIMCFG_TSTART * (0U); /* Timer start bit disabled */

    p->TIMCTL[0] =
        FLEXIO_TIMCTL_TRGSEL(((SHIFTNUM - 1) << 2) | 1U) /* Timer trigger selected as highest shifter's status flag */
        | FLEXIO_TIMCTL_TRGPOL * (1U)                    /* Timer trigger polarity as active low */
        | FLEXIO_TIMCTL_TRGSRC * (1U)                    /* Timer trigger source as internal */
        | FLEXIO_TIMCTL_PINCFG(3U)                       /* Timer' pin configured as output */
        | FLEXIO_TIMCTL_PINSEL(18)                       /* Timer' pin index: WR pin */
        | FLEXIO_TIMCTL_PINPOL * (1U)                    /* Timer' pin active low */
        | FLEXIO_TIMCTL_TIMOD(1U);                       /* Timer mode 8-bit baud counter */
                                                         /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;

    // configure interrupts
    attachInterruptVector(hw->flex_irq, ISR);
    NVIC_ENABLE_IRQ(hw->flex_irq);
    NVIC_SET_PRIORITY(hw->flex_irq, FLEXIO_ISR_PRIORITY);

    // disable interrupts until later
    p->SHIFTSIEN &= ~(1 << SHIFTER_IRQ);
    p->TIMIEN &= ~(1 << TIMER_IRQ);
}

FASTRUN void RA8876_t41_p::flexIRQ_Callback() {
    if (p->TIMSTAT & (1 << TIMER_IRQ)) { // interrupt from end of burst
        p->TIMSTAT = (1 << TIMER_IRQ);   // clear timer interrupt signal
        bursts_to_complete--;
        if (bursts_to_complete == 0) {
            p->TIMIEN &= ~(1 << TIMER_IRQ); // disable timer interrupt
            asm("dsb");
            WR_IRQTransferDone = true;
            CSHigh();
            _onCompleteCB();
            return;
        }
    }
    if (p->SHIFTSTAT & (1 << SHIFTER_IRQ)) { // interrupt from empty shifter buffer
        // note, the interrupt signal is cleared automatically when writing data to the shifter buffers
        if (bytes_remaining == 0) {                     // just started final burst, no data to load
            p->SHIFTSIEN &= ~(1 << SHIFTER_IRQ);        // disable shifter interrupt signal
        } else if (bytes_remaining < BYTES_PER_BURST) { // just started second-to-last burst, load data for final burst
            uint8_t beats = bytes_remaining / BYTES_PER_BEAT;
            p->TIMCMP[0] = ((beats * 2U - 1) << 8) | (_baud_div / 2U - 1); // takes effect on final burst
            readPtr = finalBurstBuffer;
            bytes_remaining = 0;
            for (int i = 0; i < SHIFTNUM; i++) {
                uint32_t data = *readPtr++;
                p->SHIFTBUFHWS[i] = ((data >> 16) & 0xFFFF) | ((data << 16) & 0xFFFF0000);
                while (0 == (p->SHIFTSTAT & (1U << SHIFTER_IRQ))) {
                }
            }
        } else {
            bytes_remaining -= BYTES_PER_BURST;
            for (int i = 0; i < SHIFTNUM; i++) {
                uint32_t data = *readPtr++;
                p->SHIFTBUFHWS[i] = ((data >> 16) & 0xFFFF) | ((data << 16) & 0xFFFF0000);
                while (0 == (p->SHIFTSTAT & (1U << SHIFTER_IRQ))) {
                }
            }
        }
    }
    asm("dsb");
}

FASTRUN void RA8876_t41_p::ISR() {
    asm("dsb");
    IRQcallback->flexIRQ_Callback();
}

RA8876_t41_p *RA8876_t41_p::IRQcallback = nullptr;

FASTRUN void RA8876_t41_p::MulBeatWR_nPrm_IRQ(const void *value, uint32_t const length) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    FlexIO_Config_MultiBeat();
    WR_IRQTransferDone = false;
    uint32_t bytes = length * 2U;

    CSLow();
    DCHigh();

    bursts_to_complete = bytes / BYTES_PER_BURST;

    int remainder = bytes % BYTES_PER_BURST;
    if (remainder != 0) {
        memset(finalBurstBuffer, 0, sizeof(finalBurstBuffer));
        memcpy(finalBurstBuffer, (uint8_t *)value + bytes - remainder, remainder);
        bursts_to_complete++;
    }

    bytes_remaining = bytes;
    readPtr = (uint32_t *)value;
    //    Serial.printf ("arg addr: %x, readPtr addr: %x, contents: %x\n", value, readPtr, *readPtr);
    //    Serial.printf("START::bursts_to_complete: %d bytes_remaining: %d \n", bursts_to_complete, bytes_remaining);

    uint8_t beats = SHIFTNUM * BEATS_PER_SHIFTER;
    p->TIMCMP[0] = ((beats * 2U - 1) << 8) | (_baud_div / 2U - 1U);
    p->TIMSTAT = (1 << TIMER_IRQ); // clear timer interrupt signal

    asm("dsb");

    IRQcallback = this;
    // enable interrupts to trigger bursts
    p->TIMIEN |= (1 << TIMER_IRQ);
    p->SHIFTSIEN |= (1 << SHIFTER_IRQ);
}

FASTRUN void RA8876_t41_p::_onCompleteCB() {
    if (_callback) {
        _callback();
    }
    return;
}

FASTRUN void RA8876_t41_p::pushPixels16bitAsync(const uint16_t *pcolors, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any DMA transfers to complete
    uint32_t area = (x2) * (y2);
    graphicMode(true);
    activeWindowXY(x1, y1);
    activeWindowWH(x2, y2);
    setPixelCursor(x1, y1);
    ramAccessPrepare();
    MulBeatWR_nPrm_IRQ(pcolors, area);
}

//**********************************************************************
// This section defines the low level parallel 8080 access routines.
// It uses "BUS_WIDTH" (8 or 16) to decide which drivers to use.
//**********************************************************************
void RA8876_t41_p::LCD_CmdWrite(unsigned char cmd) {
    //  startSend();
    //  _pspi->transfer16(0x00);
    //  _pspi->transfer(cmd);
    //  endSend(true);
}

//****************************************************************
// Write to a RA8876 register (8 bit only) in 8/16 bbit buss mode.
//****************************************************************
void RA8876_t41_p::lcdRegWrite(ru8 reg, bool finalize) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCLow();
    /* Write command index */
    p->SHIFTBUF[0] = reg;
    /*Wait for transfer to be completed */
    while (0 == (p->SHIFTSTAT & (1 << 0))) {
    }
    while (0 == (p->TIMSTAT & (1 << 0))) {
    }
    /* De-assert RS pin */
    DCHigh();
    CSHigh();
}

//*****************************************************************
// Write RA8876 Data 8 bit data reg or memory in 8/16 bit bus mode.
//*****************************************************************
void RA8876_t41_p::lcdDataWrite(ru8 data, bool finalize) {
    while (WR_IRQTransferDone == false) {
    }
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8876.

    FlexIO_Config_SnglBeat();

    CSLow();
    DCHigh();

    p->SHIFTBUF[0] = data;
    /*Wait for transfer to be completed */
    while (0 == (p->SHIFTSTAT & (1 << 0))) {
    }
    while (0 == (p->TIMSTAT & (1 << 0))) {
    }
    CSHigh();
}

//**************************************************************//
// Read RA8876 parallel Data (8-bit read) 8/16 bit bus mode.
//**************************************************************//
ru8 RA8876_t41_p::lcdDataRead(bool finalize) {
    uint16_t dummy __attribute__((unused)) = 0;
    uint16_t data = 0;

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    FlexIO_Config_SnglBeat_Read();

    CSLow();  // Must to go low after config above.
    DCHigh(); // Set HIGH for data read

    digitalWriteFast(RD_PIN, LOW); // Set RD pin low manually

    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    dummy = p->SHIFTBUFBYS[3];
    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    data = p->SHIFTBUFBYS[3];

    digitalWriteFast(RD_PIN, HIGH); // Set RD pin high manually

    CSHigh();

    // Serial.printf("lcdDataread(): Dummy 0x%4.4x, data 0x%4.4x\n", dummy, data);

    // Set FlexIO back to Write mode
    FlexIO_Config_SnglBeat(); // Not sure if this is needed.
    if (BUS_WIDTH == 8)
        return data;
    else
        return (data >> 8) & 0xff;
}

//**************************************************************//
// Read RA8876 status register 8bit data R/W only. 8/16 bit bus.
// Special case for status register access:
// /CS = 0, /DC = 0, /RD = 0, /WR = 1.
//**************************************************************//
ru8 RA8876_t41_p::lcdStatusRead(bool finalize) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    FlexIO_Config_SnglBeat_Read();

    CSLow();
    DCLow();

    digitalWriteFast(RD_PIN, LOW); // Set RD pin low manually

    uint16_t data = 0;
    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    data = p->SHIFTBUFBYS[3];

    DCHigh();
    CSHigh();

    digitalWriteFast(RD_PIN, HIGH); // Set RD pin high manually

    // Set FlexIO back to Write mode
    FlexIO_Config_SnglBeat();

    if (BUS_WIDTH == 8)
        return data;
    else
        return (data >> 8) & 0xff;
}

//**************************************************************//
// Write Data to a RA8876 register
//**************************************************************//
void RA8876_t41_p::lcdRegDataWrite(ru8 reg, ru8 data, bool finalize) {
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8876.
    lcdRegWrite(reg);
    lcdDataWrite(data);
}

//**************************************************************//
// Read a RA8876 register Data
//**************************************************************//
ru8 RA8876_t41_p::lcdRegDataRead(ru8 reg, bool finalize) {
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8876.
    lcdRegWrite(reg, finalize);
    return lcdDataRead();
}

//******************************************************************
// support 8080 bus interface to write 16bpp data after Regwrite 04h
//******************************************************************
void RA8876_t41_p::lcdDataWrite16bbp(ru16 data, bool finalize) {
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8876.
    if (BUS_WIDTH == 8) {
        lcdDataWrite(data & 0xff);
        lcdDataWrite(data >> 8);
    } else {
        lcdDataWrite16(data, false);
    }
}

//*********************************************************
// Write RA8876 data to display memory, 8/16 bit buss mode.
//*********************************************************
void RA8876_t41_p::lcdDataWrite16(uint16_t data, bool finalize) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8876.

    FlexIO_Config_SnglBeat();

    CSLow();
    DCHigh();

    if (BUS_WIDTH == 16) {
        p->SHIFTBUF[0] = data;
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        while (0 == (p->TIMSTAT & (1 << 0))) {
        }
    } else {
        p->SHIFTBUF[0] = data >> 8;
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = data & 0xff;
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        while (0 == (p->TIMSTAT & (1 << 0))) {
        }
    }
    /* De-assert /CS pin */
    CSHigh();
    DCLow();
}

//**************************************************************//
/* Write 16bpp(RGB565) picture data for user operation          */
/* Not recommended for future use - use BTE instead             */
//**************************************************************//
void RA8876_t41_p::putPicture_16bpp(ru16 x, ru16 y, ru16 width, ru16 height) {
    graphicMode(true);
    activeWindowXY(x, y);
    activeWindowWH(width, height);
    setPixelCursor(x, y);
    ramAccessPrepare();
    // Now your program has to send the image data pixels
}

//*******************************************************************//
/* write 16bpp(RGB565) picture data in byte format from data pointer */
/* Not recommended for future use - use BTE instead                  */
//*******************************************************************//
void RA8876_t41_p::putPicture_16bppData8(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned char *data) {
    ru16 i, j;

    putPicture_16bpp(x, y, width, height);
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(10); // Initially setup for the dev board v4.0
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
    checkWriteFifoEmpty(); // if high speed mcu and without Xnwait check
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

//****************************************************************//
/* Write 16bpp(RGB565) picture data word format from data pointer */
/* Not recommended for future use - use BTE instead               */
//****************************************************************//
void RA8876_t41_p::putPicture_16bppData16(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned short *data) {
    ru16 i, j;

    putPicture_16bpp(x, y, width, height);
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(25); // Initially setup for the dev board v4.0
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
    checkWriteFifoEmpty(); // if high speed mcu and without Xnwait check
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

//**************************************************************//
// Send data from the microcontroller to the RA8876
// Does a Raster OPeration to combine with an image already in memory
// For a simple overwrite operation, use ROP 12
//**************************************************************//
void RA8876_t41_p::bteMpuWriteWithROPData8(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                           ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned char *data) {
    bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width, des_x, des_y, width, height, rop_code);
    // MulBeatWR_nPrm_DMA(data,(width)*(height));
    ru16 i, j;
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(10); // Initially setup for the T4.1 board
            if (_rotation & 1)
                delayNanoseconds(20);
            p->SHIFTBUF[0] = *data++;
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
            p->SHIFTBUF[0] = *data++;
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    // De-assert /CS pin
    CSHigh();
}

//**************************************************************//
// For 16-bit byte-reversed data.
// Note this is 4-5 milliseconds slower than the 8-bit version above
// as the bulk byte-reversing SPI transfer operation is not available
// on all Teensys.
//**************************************************************//
void RA8876_t41_p::bteMpuWriteWithROPData16(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                            ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned short *data) {
    ru16 i, j;
    bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width, des_x, des_y, width, height, rop_code);

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(10); // Initially setup for the T4.1 board
            if (_rotation & 1)
                delayNanoseconds(70);
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
}

//**************************************************************//
// write data after setting, using lcdDataWrite() or lcdDataWrite16bbp()
//**************************************************************//
void RA8876_t41_p::bteMpuWriteWithROP(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                      ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code) {
    check2dBusy();
    graphicMode(true);
    bte_Source1_MemoryStartAddr(s1_addr);
    bte_Source1_ImageWidth(s1_image_width);
    bte_Source1_WindowStartXY(s1_x, s1_y);
    bte_DestinationMemoryStartAddr(des_addr);
    bte_DestinationImageWidth(des_image_width);
    bte_DestinationWindowStartXY(des_x, des_y);
    bte_WindowSize(width, height);
    lcdRegDataWrite(RA8876_BTE_CTRL1, rop_code << 4 | RA8876_BTE_MPU_WRITE_WITH_ROP);                                                             // 91h
    lcdRegDataWrite(RA8876_BTE_COLR, RA8876_S0_COLOR_DEPTH_16BPP << 5 | RA8876_S1_COLOR_DEPTH_16BPP << 2 | RA8876_DESTINATION_COLOR_DEPTH_16BPP); // 92h
    lcdRegDataWrite(RA8876_BTE_CTRL0, RA8876_BTE_ENABLE << 4);                                                                                    // 90h
    ramAccessPrepare();
}

//**************************************************************//
// Send data from the microcontroller to the RA8876
// Does a chromakey (transparent color) to combine with the image already in memory
//**************************************************************//
void RA8876_t41_p::bteMpuWriteWithChromaKeyData8(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width,
                                                 ru16 height, ru16 chromakey_color, const unsigned char *data) {
    bteMpuWriteWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, chromakey_color);
    // MulBeatWR_nPrm_DMA(data,(width)*(height));
    ru16 i, j;
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8876.

    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            // delayNanoseconds(70);  // Initially setup for the dev board v4.0
            if (_rotation & 1)
                delayNanoseconds(20);
            p->SHIFTBUF[0] = *data++;
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
            p->SHIFTBUF[0] = *data++;
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    // De-assert /CS pin
    CSHigh();
}
//**************************************************************//
// Chromakey for 16-bit byte-reversed data. (Slower than 8-bit.)
//**************************************************************//
void RA8876_t41_p::bteMpuWriteWithChromaKeyData16(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y,
                                                  ru16 width, ru16 height, ru16 chromakey_color, const unsigned short *data) {
    ru16 i, j;
    bteMpuWriteWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, chromakey_color);

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(20); // Initially setup for the dev board v4.0
            if (_rotation & 1)
                delayNanoseconds(70);
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
}

// Helper functions.
void RA8876_t41_p::beginWrite16BitColors() {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    delayNanoseconds(10); // Initially setup for the T4.1 board
    if (_rotation & 1)
        delayNanoseconds(20);
}
void RA8876_t41_p::write16BitColor(uint16_t color) {
    delayNanoseconds(10); // Initially setup for the T4.1 board
    if (_rotation & 1)
        delayNanoseconds(20);

    if (BUS_WIDTH == 8) {
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = color & 0xff;

        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = color >> 8;
    } else {
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = color;
    }
}
void RA8876_t41_p::endWrite16BitColors() {
    // De-assert /CS pin
    while (0 == (p->TIMSTAT & (1 << 0))) {
    }
    delayNanoseconds(10); // Initially setup for the T4.1 board
    CSHigh();
}