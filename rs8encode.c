/* Reed-Solomon encoder
 * Copyright 2004, Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */

#include <string.h>
#include <avr/pgmspace.h>
#include "rs8.h"

#define MM     (8)
#define NN     (255)
#define NROOTS (32)
#define FCR    (112)
#define PRIM   (11)
#define IPRIM  (116)

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define A0       (NN) /* Special reserved value encoding zero in index form */

inline int mod255(int x)
{
	while(x >= 255)
	{
		x -= 255;
		x = (x >> 8) + (x & 255);
	}
	return(x);
}

/* These tables are read-only, and could be stored in flash only with PROGMEM
 * if RAM usage becomes an issue. They eat up about 646 bytes.
*/

const uint8_t alpha_to[] = {
0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x87,0x89,0x95,0xAD,0xDD,0x3D,0x7A,0xF4,
0x6F,0xDE,0x3B,0x76,0xEC,0x5F,0xBE,0xFB,0x71,0xE2,0x43,0x86,0x8B,0x91,0xA5,0xCD,
0x1D,0x3A,0x74,0xE8,0x57,0xAE,0xDB,0x31,0x62,0xC4,0x0F,0x1E,0x3C,0x78,0xF0,0x67,
0xCE,0x1B,0x36,0x6C,0xD8,0x37,0x6E,0xDC,0x3F,0x7E,0xFC,0x7F,0xFE,0x7B,0xF6,0x6B,
0xD6,0x2B,0x56,0xAC,0xDF,0x39,0x72,0xE4,0x4F,0x9E,0xBB,0xF1,0x65,0xCA,0x13,0x26,
0x4C,0x98,0xB7,0xE9,0x55,0xAA,0xD3,0x21,0x42,0x84,0x8F,0x99,0xB5,0xED,0x5D,0xBA,
0xF3,0x61,0xC2,0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x07,0x0E,0x1C,0x38,0x70,0xE0,
0x47,0x8E,0x9B,0xB1,0xE5,0x4D,0x9A,0xB3,0xE1,0x45,0x8A,0x93,0xA1,0xC5,0x0D,0x1A,
0x34,0x68,0xD0,0x27,0x4E,0x9C,0xBF,0xF9,0x75,0xEA,0x53,0xA6,0xCB,0x11,0x22,0x44,
0x88,0x97,0xA9,0xD5,0x2D,0x5A,0xB4,0xEF,0x59,0xB2,0xE3,0x41,0x82,0x83,0x81,0x85,
0x8D,0x9D,0xBD,0xFD,0x7D,0xFA,0x73,0xE6,0x4B,0x96,0xAB,0xD1,0x25,0x4A,0x94,0xAF,
0xD9,0x35,0x6A,0xD4,0x2F,0x5E,0xBC,0xFF,0x79,0xF2,0x63,0xC6,0x0B,0x16,0x2C,0x58,
0xB0,0xE7,0x49,0x92,0xA3,0xC1,0x05,0x0A,0x14,0x28,0x50,0xA0,0xC7,0x09,0x12,0x24,
0x48,0x90,0xA7,0xC9,0x15,0x2A,0x54,0xA8,0xD7,0x29,0x52,0xA4,0xCF,0x19,0x32,0x64,
0xC8,0x17,0x2E,0x5C,0xB8,0xF7,0x69,0xD2,0x23,0x46,0x8C,0x9F,0xB9,0xF5,0x6D,0xDA,
0x33,0x66,0xCC,0x1F,0x3E,0x7C,0xF8,0x77,0xEE,0x5B,0xB6,0xEB,0x51,0xA2,0xC3,0x00,
};

const uint8_t index_of[] = {
0xFF,0x00,0x01,0x63,0x02,0xC6,0x64,0x6A,0x03,0xCD,0xC7,0xBC,0x65,0x7E,0x6B,0x2A,
0x04,0x8D,0xCE,0x4E,0xC8,0xD4,0xBD,0xE1,0x66,0xDD,0x7F,0x31,0x6C,0x20,0x2B,0xF3,
0x05,0x57,0x8E,0xE8,0xCF,0xAC,0x4F,0x83,0xC9,0xD9,0xD5,0x41,0xBE,0x94,0xE2,0xB4,
0x67,0x27,0xDE,0xF0,0x80,0xB1,0x32,0x35,0x6D,0x45,0x21,0x12,0x2C,0x0D,0xF4,0x38,
0x06,0x9B,0x58,0x1A,0x8F,0x79,0xE9,0x70,0xD0,0xC2,0xAD,0xA8,0x50,0x75,0x84,0x48,
0xCA,0xFC,0xDA,0x8A,0xD6,0x54,0x42,0x24,0xBF,0x98,0x95,0xF9,0xE3,0x5E,0xB5,0x15,
0x68,0x61,0x28,0xBA,0xDF,0x4C,0xF1,0x2F,0x81,0xE6,0xB2,0x3F,0x33,0xEE,0x36,0x10,
0x6E,0x18,0x46,0xA6,0x22,0x88,0x13,0xF7,0x2D,0xB8,0x0E,0x3D,0xF5,0xA4,0x39,0x3B,
0x07,0x9E,0x9C,0x9D,0x59,0x9F,0x1B,0x08,0x90,0x09,0x7A,0x1C,0xEA,0xA0,0x71,0x5A,
0xD1,0x1D,0xC3,0x7B,0xAE,0x0A,0xA9,0x91,0x51,0x5B,0x76,0x72,0x85,0xA1,0x49,0xEB,
0xCB,0x7C,0xFD,0xC4,0xDB,0x1E,0x8B,0xD2,0xD7,0x92,0x55,0xAA,0x43,0x0B,0x25,0xAF,
0xC0,0x73,0x99,0x77,0x96,0x5C,0xFA,0x52,0xE4,0xEC,0x5F,0x4A,0xB6,0xA2,0x16,0x86,
0x69,0xC5,0x62,0xFE,0x29,0x7D,0xBB,0xCC,0xE0,0xD3,0x4D,0x8C,0xF2,0x1F,0x30,0xDC,
0x82,0xAB,0xE7,0x56,0xB3,0x93,0x40,0xD8,0x34,0xB0,0xEF,0x26,0x37,0x0C,0x11,0x44,
0x6F,0x78,0x19,0x9A,0x47,0x74,0xA7,0xC1,0x23,0x53,0x89,0xFB,0x14,0x5D,0xF8,0x97,
0x2E,0x4B,0xB9,0x60,0x0F,0xED,0x3E,0xE5,0xF6,0x87,0xA5,0x17,0x3A,0xA3,0x3C,0xB7,
};

const uint8_t poly[] = {
0x00,0xF9,0x3B,0x42,0x04,0x2B,0x7E,0xFB,0x61,0x1E,0x03,0xD5,0x32,0x42,0xAA,0x05,
0x18,0x05,0xAA,0x42,0x32,0xD5,0x03,0x1E,0x61,0xFB,0x7E,0x2B,0x04,0x42,0x3B,0xF9,
0x00,
};

/* Portable C version */
void encode_rs_8(uint8_t *data, uint8_t *parity, int pad)
{
	int i, j;
	uint8_t feedback;
	
	memset(parity, 0, NROOTS * sizeof(uint8_t));
	
	for(i = 0; i < NN - NROOTS - pad; i++)
	{
		feedback = index_of[data[i] ^ parity[0]];
		if(feedback != A0) /* feedback term is non-zero */
		{
			for(j = 1; j < NROOTS; j++)
				parity[j] ^= alpha_to[mod255(feedback + poly[NROOTS - j])];
		}
		
		/* Shift */
		memmove(&parity[0], &parity[1], sizeof(uint8_t) * (NROOTS - 1));
		if(feedback != A0)
			parity[NROOTS - 1] = alpha_to[mod255(feedback + poly[0])];
		else
			parity[NROOTS - 1] = 0;
	}
}

