/*
	This file is part of the smb library
    Copyright (C) 1999  Nicolas Brodu
    brodu@iie.cnam.fr

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/

#include "DES.h"
#include <stdio.h>

// official first permutation minus 1 to index from 0 to 63
const unsigned char PC1[56] = {
                        56, 48, 40, 32, 24, 16,  8,
                         0, 57, 49, 41, 33, 25, 17,
	                     9,  1, 58, 50, 42, 34, 26,
                        18, 10,  2, 59, 51, 43, 35,
                        62, 54, 46, 38, 30, 22, 14,
                         6, 61, 53, 45, 37, 29, 21,
                        13,  5, 60, 52, 44, 36, 28,
                        20, 12,  4, 27, 19, 11,  3
};

// official second permutation minus 1 to index from 0 to 55
const unsigned char PC2[48] = {
                           13, 16, 10, 23,  0,  4,
                            2, 27, 14,  5, 20,  9,
                           22, 18, 11,  3, 25,  7,
                           15,  6, 26, 19, 12,  1,
                           40, 51, 30, 36, 46, 54,
                           29, 39, 50, 44, 32, 47,
                           43, 48, 38, 55, 33, 52,
                           45, 41, 49, 35, 28, 31
};

// official left shifts on C[i], D[i], for each K[i]
const unsigned char LSHIFT[16] = {
                  1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1
};


// as before, data permutation is given minus 1
const unsigned char IP[64] = {
                        57, 49, 41, 33, 25, 17,  9,  1,
                        59, 51, 43, 35, 27, 19, 11,  3,
                        61, 53, 45, 37, 29, 21, 13,  5,
                        63, 55, 47, 39, 31, 23, 15,  7,
                        56, 48, 40, 32, 24, 16,  8,  0,
                        58, 50, 42, 34, 26, 18, 10,  2,
                        60, 52, 44, 36, 28, 20, 12,  4,
                        62, 54, 46, 38, 30, 22, 14,  6
};

// and now the same for the expansion function
const unsigned char E[48] = {
                           31,  0,  1,  2,  3,  4,
                            3,  4,  5,  6,  7,  8,
                            7,  8, 9, 10, 11, 12,
                           11, 12, 13, 14, 15, 16,
                           15, 16, 17, 18, 19, 20,
                           19, 20, 21, 22, 23, 24,
                           23, 24, 25, 26, 27, 28,
                           27, 28, 29, 30, 31,  0
};


// Substitution boxes, do not reduce by 1 since those are not indexes !
const unsigned char S[8][4][16] = {
         { {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
           { 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
           { 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
           {15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13}
         },
         { {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
           { 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
           { 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15},
           {13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9}
         },
         { {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
           {13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
           {13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
           { 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12}
         },
         { { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
           {13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9},
           {10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
           { 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14}
         },
         { { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9},
           {14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
           { 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
           {11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3}
         },
         { {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11},
           {10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
           { 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
           { 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13}
         },
         { { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
           {13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
           { 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
           { 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12}
         },
         { {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
           { 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2},
           { 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
           { 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11}
         }
};

// permutation (after substitution) minus 1 to index from 0 to 31
const unsigned char P[32] = {
                              15,  6, 19, 20,
                              28, 11, 27, 16,
                               0, 14, 22, 25,
                               4, 17, 30,  9,
                               1,  7, 23, 13,
                              31, 26,  2,  8,
                              18, 12, 29,  5,
                              21, 10,  3, 24
};

// final permutation (minus 1)
const unsigned char FP[64] = {
                        39,  7, 47, 15, 55, 23, 63, 31,
                        38,  6, 46, 14, 54, 22, 62, 30,
                        37,  5, 45, 13, 53, 21, 61, 29,
                        36,  4, 44, 12, 52, 20, 60, 28,
                        35,  3, 43, 11, 51, 19, 59, 27,
                        34,  2, 42, 10, 50, 18, 58, 26,
                        33,  1, 41,  9, 49, 17, 57, 25,
                        32,  0, 40,  8, 48, 16, 56, 24
};

// key must be 56 bits
// Anyway, the parity bits introduced in the des-how-to are discarded by the
// first permutation function
DES::DES(const unsigned char* k)
{
	setKey(k);
}

DES::~DES()
{
}

// key must be 56 bits
// Anyway, the parity bits introduced in the des-how-to are discarded by the
// first permutation function
void DES::setKey(const unsigned char* k)
{
	if (!k) return;
	unsigned char *ptr=key;
	int hole=0;
	for (int i=0; i<7; i++) {
		unsigned char c=k[i];
		*(ptr++)=(c&0x80)>>7;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=(c&0x40)>>6;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=(c&0x20)>>5;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=(c&0x10)>>4;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=(c&0x08)>>3;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=(c&0x04)>>2;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=(c&0x02)>>1;
		if ((++hole) % 7 == 0) *(ptr++)=0;
		*(ptr++)=c&0x01;
		if ((++hole) % 7 == 0) *(ptr++)=0;
	}

	unsigned char C[28]; // one bit by byte, simpler algorithm...
	unsigned char D[28]; // one bit by byte, simpler algorithm...
	// Initial permutation of the key
	for (int i=0; i<28; i++) {
		C[i]=key[PC1[i]];
		D[i]=key[PC1[i+28]];
	}

	unsigned char concat[56];
	// calculate the 16 keys
	for (int i=0; i<16; i++) {
		// do LSHIFT[i] shifts
		for (int j=0; j<LSHIFT[i]; j++) {
			// do the shift
			unsigned char savC = C[0];
			unsigned char savD = D[0];
			for (int l=0; l<27; l++) {
				C[l]=C[l+1];
				D[l]=D[l+1];
				concat[l]=C[l];
				concat[l+28]=D[l];
			}
			C[27]=savC;
			D[27]=savD;
			concat[27]=savC;
			concat[55]=savD;
		}
		// new permutation => give key
		for (int j=0; j<48; j++) {
			K[i][j]=concat[PC2[j]];
		}

	}
}


// Encrypt (encdec=0) or decrypt (encdec=1)
void DES::doIt(unsigned char* data, const unsigned long length, int encdec)
{
	unsigned char plain[64];
	unsigned char L[32];
	unsigned char R[32];
	unsigned char ER[48]; // expanded R xored with key
	unsigned char* dataIndex=data;
	int i,j;

	int len=length;
	while (len>=8) {
		// generate 64 bit "plain text" bloc
		unsigned char *ptr=plain;
		for (i=0; i<8; i++) {
			unsigned char c=*(dataIndex+i);
			*(ptr++)=(c&0x80)>>7;
			*(ptr++)=(c&0x40)>>6;
			*(ptr++)=(c&0x20)>>5;
			*(ptr++)=(c&0x10)>>4;
			*(ptr++)=(c&0x08)>>3;
			*(ptr++)=(c&0x04)>>2;
			*(ptr++)=(c&0x02)>>1;
			*(ptr++)=c&0x01;
		}

		// Initial permutation of the data block
		for (i=0; i<32; i++) {
			L[i]=plain[IP[i]];
			R[i]=plain[IP[i+32]];
		}
		
		// Apply the 16 keys
		for (i=0; i<16; i++) {
			// expand R and xor with key number i
			for (j=0; j<48; j++) {
				ER[j] = R[E[j]] ^ K[encdec?15-i:i][j];
			}
			unsigned char* ERIndex=ER;  // where to get the values
			unsigned char* ERIndexS=ER; // where to substitute (4 bits wide only)
			// apply the substitutions
			for (j=0; j<8; j++) {
				unsigned char c = S[j]
					[((*ERIndex)<<1)|(*(ERIndex+5))]
					[((*(ERIndex+1))<<3)|((*(ERIndex+2))<<2)|((*(ERIndex+3))<<1)|(*(ERIndex+4))];
				*(ERIndexS++)=(c&0x08)>>3;
				*(ERIndexS++)=(c&0x04)>>2;
				*(ERIndexS++)=(c&0x02)>>1;
				*(ERIndexS++)=c&0x01;
				ERIndex+=6;
			}
			// permute again and xor with L
			// assign new values for R and L
			for (j=0; j<32; j++) {
				unsigned char tmp = ER[P[j]] ^ L[j];
				L[j] = R[j];
				R[j] = tmp;
			}
		} // end of apply all keys

		// append R, L => plain (no need to declare another 64 bytes array)
		for (i=0; i<32; i++) {
			plain[i]=R[i];
			plain[i+32]=L[i];
		}
		// Final permutation, put result back into "data"
		for (i=0; i<8; i++) {
			j=i<<3;
			*(dataIndex++) = plain[FP[j+7]]
				| (plain[FP[j+6]] << 1)
				| (plain[FP[j+5]] << 2)
				| (plain[FP[j+4]] << 3)
				| (plain[FP[j+3]] << 4)
				| (plain[FP[j+2]] << 5)
				| (plain[FP[j+1]] << 6)
				| (plain[FP[j]] << 7);
		}

		len-=8;
	} // of while len>=8
}

// Data should always be aligned to a 64 bit multiple
// If not, discard the junk (hence force the caller to paddle...)
void DES::encrypt(unsigned char* data, const unsigned long length)
{
	doIt(data,length,0);
}

void DES::decrypt(unsigned char* data, const unsigned long length)
{
	doIt(data,length,1);
}
