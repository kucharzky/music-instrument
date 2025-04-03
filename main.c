/*-------------------------------------------------------------------------
					Technika Mikroprocesorowa 2 - CWP
					Projekt nr 18 - Instrument muzyczny (13 dźwięków) sterowany akcelerometrem
					autor: Maciej Kucharski
					wersja: 29.01.2024r.
					rev 1.4
----------------------------------------------------------------------------*/

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
#define	ZYXDR_Mask	1<<3 // Maska dla flagi gotowości danych akcelerometru
#define DIV_CORE	32768	
#define ACCEL_SENSITIVITY 0 // Czułość akcelerometru
//#define ACCEL_UPDATE_PERIOD 50

// Częstotliwości dla 13 dźwięków (C4 do C5)
const float frequencies[] = {
    261.63, 277.18, 293.66, 311.13, 329.63, 349.23,
    369.99, 392.00, 415.30, 440.00, 466.16, 493.88, 523.25
};

// Nazwy nut
const char *noteNames[] = {
    "C", "C#", "D", "D#", "E", "F",
    "F#", "G", "G#", "A", "A#", "B", "C"
};

volatile uint8_t S2_press = 0;       // Flaga dla przycisku S2
volatile uint8_t S3_press = 0;       // Flaga dla przycisku S3
volatile uint8_t S4_press = 0;       // Flaga dla przycisku S4

volatile float currentNote = 0; // Indeks aktualnej nuty
volatile int8_t octave = 0;     // Aktualna oktawa
volatile uint8_t waveForm = 0;   // Aktualny kształt fali (0-sinus, 1-trojkat, 2-pila)
volatile uint8_t volume = 20;   // Głośność (0-100)

volatile uint16_t dac;	// Wartość do wysłania do DAC
volatile int32_t Sinus[20];
const int32_t Trojkat[] = {0, 409, 818, 1227, 1636, 2045, 1636, 1227, 818, 409, 0, -409, -818, -1227, -1636, -2045, -1636, -1227, -818, -409};
const int32_t Pila[] = {0, 205, 409, 614, 819, 1024, 1228, 1433, 1638, 1842, 2047, -2047, -1820, -1592, -1365, -1137, -910, -682, -455, -227};
volatile float n;	// Zmienna do generowania fazy w DDS
volatile uint8_t trig = 0;	// Flaga do generowania sygnału

// Zmienne do obsługi akcelerometru i wyświetlacza
volatile uint8_t update_accel = 0;   // Flaga do aktualizacji danych z akcelerometru
volatile int16_t accelX = 0;         // Wartość odczytana z osi X akcelerometru
volatile uint8_t update_display = 0; // Flaga do aktualizacji wyświetlacza
static uint8_t status;               // Status akcelerometru

void SysTick_Handler(void)
{
    /*/ Flaga do aktualizacji danych z akcelerometru
    /static uint32_t accel_count = 0;
    accel_count++;
    if (accel_count >= ACCEL_UPDATE_PERIOD) {
        update_accel = 1;
        accel_count = 0;
    }*/

    trig ^= 0x1;	// Przełączanie flagi trig (co drugie przerwanie)
    if (trig)
    {
				// Obliczenie częstotliwości
        float currentFrequency = frequencies[(int)round(currentNote)] * pow(2, octave);
        // Obliczenie przyrostu fazy
        float phaseIncrement = currentFrequency / (SystemCoreClock / DIV_CORE / 2.0f) * 20.0f;

        uint16_t n_range = 20;       // Zakres wartości fazy
        uint16_t n_int = (uint16_t)n; // Bieżąca wartość fazy

				// Generowanie sygnału w zależności od wybranego kształtu fali
			  // dodane 0x0800 czyli 2048 aby uzyskac dodatnie wyniki dla dac
        switch (waveForm)
        {
        case 0:
            dac = (Sinus[n_int % 20] * volume / 100) + 0x0800; 
            break;
        case 1:
            dac = (Trojkat[n_int % 20] * volume / 100) + 0x0800;
            break;
        case 2:
            dac = (Pila[n_int % 20] * volume / 100) + 0x0800;
            break;
        }

        DAC_Load_Trig(dac);
        n += phaseIncrement;
        if (n >= n_range)
        {
            n = fmod(n, n_range);	// Normalizacja fazy
        }
    }
}
// Przerwanie PORTA - obsługa akcelerometru na PTA10
void PORTA_IRQHandler(void)
{
    if (PORTA->ISFR & (1 << 10)) {
        update_accel = 1;
				PORTA->ISFR |= (1 << 10);
			}
		
}
// Przerwanie PORTB - obsługa przycisków
void PORTB_IRQHandler(void)
{
		uint32_t buf;
    buf = PORTB->ISFR & (S2_MASK | S3_MASK | S4_MASK); // Sprawdzenie, który przycisk wywołał przerwanie

		// Minimalizacja drgan ze stykow i ustawianie flagi dla przyciskow
    switch (buf)
    {
    case S2_MASK:
        DELAY(100);
        if (!(PTB->PDIR & S2_MASK)) 
        {
            DELAY(100);
            if (!(PTB->PDIR & S2_MASK))
            {
                if (!S2_press)
                {
                    S2_press = 1;
                }
            }
        }
        break;
    case S3_MASK:
        DELAY(100);
        if (!(PTB->PDIR & S3_MASK)) 
        {
            DELAY(100);
            if (!(PTB->PDIR & S3_MASK))
            {
                if (!S3_press)
                {
                    S3_press = 1;
                }
            }
        }
        break;
		case S4_MASK:
        DELAY(100);
        if (!(PTB->PDIR & S4_MASK)) 
        {
            DELAY(100);
            if (!(PTB->PDIR & S4_MASK))
            {
                if (!S4_press)
                {
                    S4_press = 1;
                }
            }
        }
        break;
    default:
        break;
    }
	PORTB->ISFR |= S2_MASK | S3_MASK | S4_MASK;	// Czyszczenie flagi przerwan
	NVIC_ClearPendingIRQ(PORTB_IRQn);
}

void Init_Accel(void) {
    I2C_Init();
	// Przerwania dla freefall/ motion oraz dla osi X
    I2C_WriteReg(0x1D, 0x2A, 0x00);
    I2C_WriteReg(0x1D, 0x0E, ACCEL_SENSITIVITY);
		I2C_WriteReg(0x1D, 0x15, 0xC8); 
		I2C_WriteReg(0x1D, 0x2D, 0x04);
		I2C_WriteReg(0x1D, 0x2E, 0x00);
    I2C_WriteReg(0x1D, 0x2A, 0x23); 

    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    PORTA->PCR[10] |= PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(0xb);
	
		PORTB->PCR[0] |= PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(0xA); // GPIO, pull-up, interrupt na zbocze opadające
    PORTB->PCR[2] |= PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(0xA); 
    PORTB->PCR[6] |= PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK | PORT_PCR_IRQC(0xA); 

    NVIC_SetPriority(PORTA_IRQn, 3);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
    NVIC_EnableIRQ(PORTA_IRQn);
		
		NVIC_SetPriority(PORTB_IRQn, 3);
    NVIC_ClearPendingIRQ(PORTB_IRQn);
    NVIC_EnableIRQ(PORTB_IRQn);
}

int main(void)
{
    char display[17]; // Bufor na wyswietlanie danych
		
    //Klaw_Init();
    //Klaw_S2_4_Int();
    LCD1602_Init();
    LCD1602_Backlight(TRUE);
    DAC_Init();
    Init_Accel();
    TSI_Init();

		// Generowanie tablicy sinusoidy
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

    SysTick_Config(SystemCoreClock / DIV_CORE);	// Konfiguracja przerwania SysTick
		uint8_t temp;
    while (1)
    {
			I2C_ReadReg(0x1d,0x16,&temp);
			I2C_ReadReg(0x1d, 0x0, &status);
		status&=ZYXDR_Mask;
		if(update_accel)	// Czy dane gotowe do odczytu?
		{
		// Obsługa akcelerometru
        if (status) {
            update_accel = 0;
            uint8_t accelData[2]; // 2 bajty - os X
            I2C_ReadRegBlock(0x1D, 0x01, 2, accelData); // odczyt z OUT_X_MSB i OUT_X_LSB (0x01 i 0x02)
            accelX = (int16_t)((accelData[0] << 8) | accelData[1]) >> 2;
            currentNote = 6.5f + (float)accelX / 4096.0f * 6.0f;
            if (currentNote < 0) currentNote = 0;
            if (currentNote > 12) currentNote = 12;
						update_display = 1; // Zasygnalizowanie konieczności aktualizacji wyświetlacza
        }
				// Obsługa przycisków
				if(S2_press){
					S2_press = 0;
					octave++;
					if (octave > 3) octave = 3;
					update_display = 1;
				}

				if(S3_press){
					S3_press = 0;
					octave--;
					if (octave < -3) octave = -3;
					update_display = 1;
				}

				if(S4_press){
					S4_press = 0;
					waveForm = (waveForm + 1) % 3;
					update_display = 1;
				}
        // Obsługa wyświetlacza
        if (update_display) {
            update_display = 0;
            LCD1602_SetCursor(0, 0);
            sprintf(display, "%s%d ", noteNames[(int)round(currentNote)], octave + 4);
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
}
