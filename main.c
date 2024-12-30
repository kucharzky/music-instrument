/*----------------------------------------------------------------------------
	Technika Mikroprocesorowa 2 - CWP
	Projekt nr 18 - Instrument muzyczny (13 dźwięków) sterowany akcelerometrem
	autor: Maciej Kucharski
	wersja: 30.12.2024r.
	rev 1.2
------------------------------------------------------------------------------*/

#include "MKL05Z4.h"
#include "DAC.h"
#include "tsi.h"
#include "klaw.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include "i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define DIV_CORE	10000	// Przerwanie co 0.5ms - dźwięk 250Hz (co drugie przerwanie wyzwala DAC0)
#define ACCEL_SENSITIVITY 1 // Czułość akcelerometru 
#define ACCEL_UPDATE_PERIOD 50 // Okres odczytu danych z akcelerometru w ms

// Częstotliwości dla 13 dźwięków (C4 do C5)
const float frequencies[] = {
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23,
    369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 523.25
};

// Nazwy nut
const char *noteNames[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B", "C5"
};

volatile uint8_t S2_press = 0;
volatile uint8_t S3_press = 0;
volatile uint8_t S4_press = 0;

volatile float currentNote = 0; // Indeks aktualnej nuty
volatile int8_t octave = 0;     // Aktualna oktawa
volatile uint8_t waveForm = 0;   // Aktualny kształt fali (0-sinus, 1-trojkat, 2-pila)
volatile uint8_t volume = 100;   // Głośność (0-100)

volatile uint16_t dac;
volatile int32_t Sinus[20];
const int32_t Trojkat[] = {0, 409, 818, 1227, 1636, 2045, 1636, 1227, 818, 409, 0, -409, -818, -1227, -1636, -2045, -1636, -1227, -818, -409};
const int32_t Pila[] = {0, 205, 409, 614, 819, 1024, 1228, 1433, 1638, 1842, 2047, -2047, -1820, -1592, -1365, -1137, -910, -682, -455, -227};
volatile float n;
volatile uint8_t trig = 0;

// Zmienne do obsługi akcelerometru i wyświetlacza
volatile uint8_t update_accel = 0;
volatile int16_t accelX = 0;
volatile uint8_t update_display = 0;

void SysTick_Handler(void)
{
    // Flaga do aktualizacji danych z akcelerometru
    static uint32_t accel_count = 0;
    accel_count++;
    if (accel_count >= ACCEL_UPDATE_PERIOD) {
        update_accel = 1;
        accel_count = 0;
    }

    trig ^= 0x1;
    if (trig)
    {
        float currentFrequency = frequencies[(int)round(currentNote)] * pow(2, octave);
        // Poprawne obliczanie phaseIncrement
        float phaseIncrement = currentFrequency / (SystemCoreClock / DIV_CORE / 2.0f) * 20.0f;

        // Poprawne obliczanie n_range
        uint16_t n_range = 20;

        switch (waveForm)
        {
        case 0:
            dac = (Sinus[(int)fmod(n, 20)] * volume / 100) + 0x0800;
            break;
        case 1:
            dac = (Trojkat[(int)fmod(n, 20)] * volume / 100) + 0x0800;
            break;
        case 2:
            dac = (Pila[(int)fmod(n, 20)] * volume / 100) + 0x0800;
            break;
        }

        DAC_Load_Trig(dac);
        n += phaseIncrement;
        if (n >= n_range)
        {
            n = fmod(n, n_range);
        }
    }
}

void PORTA_IRQHandler(void)
{
	uint32_t buf;
    buf = PORTA->ISFR;

    if (buf & (1 << 10)) { // S2
        S2_press = 1;
		octave++;
		if (octave > 3) octave = 3;
        PORTA->ISFR |= (1 << 10);
    }

    if (buf & (1 << 11)) { // S3
        S3_press = 1;
		octave--;
		if (octave < -3) octave = -3;
        PORTA->ISFR |= (1 << 11);
    }

    if (buf & (1 << 12)) { // S4
		S4_press = 1;
		waveForm = (waveForm + 1) % 3;
		update_display = 1;
        PORTA->ISFR |= (1 << 12);
    }
		
	NVIC_ClearPendingIRQ(PORTA_IRQn);
}

void Init_Accel(void) {
    I2C_Init();

    I2C_WriteReg(0x1D, 0x2A, 0x00);
    I2C_WriteReg(0x1D, 0x0E, ACCEL_SENSITIVITY);
    I2C_WriteReg(0x1D, 0x2A, 0x01);

    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    PORTA->PCR[10] |= PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    NVIC_SetPriority(PORTA_IRQn, 3);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
}

int main(void)
{
    char display[17];

    Klaw_Init();
    Klaw_S2_4_Int();
    LCD1602_Init();
    LCD1602_Backlight(TRUE);
    DAC_Init();
    Init_Accel();
    TSI_Init();

    for (int i = 0; i < 20; i++)
        Sinus[i] = (sin((double)i * 0.314159) * 2047.0);
    n = 0;

    LCD1602_SetCursor(0, 0);
    sprintf(display, "%s%d", noteNames[(int)round(currentNote)], octave + 4);
    LCD1602_Print(display);

    LCD1602_SetCursor(0, 1);
    sprintf(display, "Vol: %3d", volume);
    LCD1602_Print(display);

    LCD1602_SetCursor(8, 1);
    switch (waveForm)
    {
    case 0:
        LCD1602_Print("Sinus   ");
        break;
    case 1:
        LCD1602_Print("Trojkat ");
        break;
    case 2:
        LCD1602_Print("Pila    ");
        break;
    }

    SysTick_Config(SystemCoreClock / DIV_CORE);

    while (1)
    {
		// Obsługa akcelerometru
        if (update_accel) {
            update_accel = 0;
            uint8_t accelData[6];
            I2C_ReadRegBlock(0x1D, 0x01, 6, accelData);
            accelX = (int16_t)((accelData[0] << 8) | accelData[1]) >> 2;
            currentNote = 6.5f + (float)accelX / 4096.0f * 6.0f;
            if (currentNote < 0) currentNote = 0;
            if (currentNote > 12) currentNote = 12;
			update_display = 1; // Zasygnalizowanie konieczności aktualizacji wyświetlacza
        }
        // Obsługa wyświetlacza
        if (update_display) {
            update_display = 0;
            LCD1602_SetCursor(0, 0);
            sprintf(display, "%s%d", noteNames[(int)round(currentNote)], octave + 4);
            LCD1602_Print(display);

            LCD1602_SetCursor(8, 1);
            switch (waveForm) {
                case 0:
                    LCD1602_Print("Sinus   ");
                    break;
                case 1:
                    LCD1602_Print("Trojkat ");
                    break;
                case 2:
                    LCD1602_Print("Pila    ");
                    break;
            }
        }

        // Obsługa slidera
        uint8_t sliderValue = TSI_ReadSlider();
        if (sliderValue != 0) {
            volume = sliderValue;
            LCD1602_SetCursor(0, 1);
            sprintf(display, "Vol: %3d", volume);
            LCD1602_Print(display);
        }
    }
}