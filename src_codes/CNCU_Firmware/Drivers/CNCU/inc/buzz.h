/**
  ******************************************************************************
  * @file           : buzz.h
  * @brief          : Header of Module working with CNCU Buzzer
  ******************************************************************************
*/

#ifndef _CNCU_BUZZ_H
#define _CNCU_BUZZ_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "cncu_conf.h"

#define BUZZ_PORT		GPIOA
#define BUZZ_PIN		GPIO_PIN_3

#define SOUND_BUF_SIZE	128		// MAX number of Sounds in 1 melody (x6 [bytes])

#define DEFAULT_TEMPO		120		// Music speed [bpm]

// Note freq table [Hz]
// Big Octave
#define	fC_				65.41
#define	fC_dies		69.30
#define	fD_				73.91
#define	fD_dies		77.78
#define	fE_				82.41
#define	fF_				87.31
#define	fF_dies		92.50
#define fG_				98.00			
#define	fG_dies		103.80
#define	fA_				110.00
#define	fA_dies		116.54
#define fB_				123.48
// Small Octave
#define	fC0				130.82
#define	fC0dies		138.59
#define	fD0				147.83
#define	fD0dies		155.56
#define	fE0				164.81
#define	fF0				174.62
#define	fF0dies		185.00
#define fG0				196.00			
#define	fG0dies		207.00
#define	fA0				220.00
#define	fA0dies		233.08
#define fB0				246.96
// 1st Octave			
#define fC1				261.63
#define fC1dies		277.18
#define fD1				293.66
#define	fD1dies		311.13
#define	fE1				329.63
#define	fF1				349.23
#define	fF1dies		369.99
#define fG1				392.00
#define	fG1dies		415.30
#define	fA1				440.00
#define fA1dies		466.16
#define fB1				493.88
// 2nd Octave
#define	fC2				523.25
#define fC2dies		554.36

#define	Hz_us(x)	( (int)(1000000.00 / x) )		// Freq [Hz] -> Period [us]

/* Exported types ------------------------------------------------------------*/
// Periods of notes [us]
typedef enum
{
	P = 0,					// Pause
	C_ = Hz_us(fC_),
	C_dies = Hz_us(fC_dies),
	D_ = Hz_us(fD_),
	D_dies = Hz_us(fD_dies),
	E_ = Hz_us(fE_),
	F_ = Hz_us(fF_),
	F_dies = Hz_us(fF_dies),
	G_ = Hz_us(fG_),
	G_dies = Hz_us(fG_dies),
	A_ = Hz_us(fA_),
	A_dies = Hz_us(fA_dies),
	B_ = Hz_us(fB_),
	C0 = Hz_us(fC0),
	C0dies = Hz_us(fC0dies),
	D0 = Hz_us(fD0),
	D0dies = Hz_us(fD0dies),
	E0 = Hz_us(fE0),
	F0 = Hz_us(fF0),
	F0dies = Hz_us(fF0dies),
	G0 = Hz_us(fG0),							//5102, 
	G0dies = Hz_us(fG0dies),
	A0 = Hz_us(fA0),
	A0dies = Hz_us(fA0dies),
	B0 = Hz_us(fB0),
	C1 = Hz_us(fC1),							//3822,
	C1dies = Hz_us(fC1dies),			//3608,
	D1 = Hz_us(fD1),							//3405,
	D1dies = Hz_us(fD1dies),			//3214,
	E1 = Hz_us(fE1),							//3034,
	F1 = Hz_us(fF1),							//2863,
	F1dies = Hz_us(fF1dies),			//2703,
	G1 = 	Hz_us(fG1),							//2551,
	G1dies = Hz_us(fG1dies),			//2408,
	A1 = Hz_us(fA1),							//2272,
	B1 = Hz_us(fB1),							//2025,
	C2 = Hz_us(fC2),							//1911
	C2dies = Hz_us(fC2dies), 
}notes_t;

// Simple Sound structure
typedef struct
{
	_Bool act;			// active - 1, inactive - 0
	notes_t note;		// note to play
	uint16_t dur;		// note duration (65535 MAX)
}sound_t;				// 6 [bytes]

// Buffer of sounds
typedef struct
{
	sound_t sound[SOUND_BUF_SIZE];
	uint16_t sound_idx;
}sound_buf_t;

/* Exported functions --------------------------------------------------------*/
/**
  * @brief	Calculation of notes duration and starting PWM generation. 
	*	@ntote	Use in 1ms tick Timer IRQ
 */
void BuzzTIM_ISR(void);

/**
  * @brief	Hardware init (PWM Timer and GPIO as PWM generation output) 	   
  * @retval	CNCU_OK - Success, INIT_ERR - Failure  
 */
cncu_err_t Buzz_Init(void);
 
/**
  * @brief	Set music speed [bpm] - number of 1\4 in 60 sec 	   
  * @param	
			tempo - desired music speed [bpm] 
 */
void Buzz_SetTemp(uint16_t tempo);

/**
  * @brief	Add desired Sound to SoundBuffer 	   
  * @param	
			note - Note of sound
			duration_ms - duration of sound
 */
void Buzz_AddNote(notes_t note, uint32_t duration_ms);

/**
  * @brief	Start Playing Sounds from SoundBuffer 	    
 */
void Buzz_Play(void);

/**
  * @brief	Play Mario bros. Music Theme	   
 */
void Buzz_PlayMario(void);
void Buzz_PlayClintEastwood(void);

#endif

