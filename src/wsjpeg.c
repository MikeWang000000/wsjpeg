/*
 * Copyright (C) 2022 Wang Sheng
 *
 * This file is part of Wang Sheng's graduation project at
 * Nanjing Institute of Technology.
 *
 * This file is under the GNU Lesser General Public License
 * version 2.1 or later (LGPL v2.1+). You can redistribute it
 * and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation;
 * either version 2.1 of the License, or (at your option) any
 * later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define OUT_OF_MEMORY_ERROR     "Out of Memory!"
#define BMP_OPEN_ERROR          "Can not open BMP file!"
#define BMP_INVALID_ERROR       "Not a valid BMP file!"
#define BMP_CORRUPT_ERROR       "Corrupt BMP File!"
#define BMP_NOT_24BIT_ERROR     "Only supports 24-bit Bitmap!"

#ifdef USE_DOUBLE
typedef double          FLOAT;
#else
typedef float           FLOAT;
#endif
#if UCHAR_MAX == 0xFF
typedef unsigned char   UINT8;
#endif
#if UINT_MAX == 0xFFFF
typedef unsigned int    UINT16;
#elif USHRT_MAX == 0xFFFF
typedef unsigned short  UINT16;
#endif
#if ULONG_MAX == 0xFFFFFFFF
typedef unsigned long   UINT32;
#elif UINT_MAX == 0xFFFFFFFF
typedef unsigned int    UINT32;
#elif USHRT_MAX == 0xFFFFFFFF
typedef unsigned short  UINT32;
#endif
#if LONG_MAX == 0x7FFFFFFF
typedef long            INT32;
#elif INT_MAX == 0x7FFFFFFF
typedef int             INT32;
#elif SHRT_MAX == 0x7FFFFFFF
typedef short           INT32;
#endif
typedef UINT8           BYTE;
typedef UINT32          RGB;
typedef size_t          SIZE_T;

typedef struct
{
    UINT16  value;
    UINT8   nbits;
} BITCODE;

typedef struct
{
    INT32   width;          /* positive:  left to right;  negative:  right to left */
    INT32   height;         /* positive:  bottom to top;  negative:  top to bottom */
    BYTE    *data;          /* bitmap data (without header) */
} BITMAP, *pBITMAP;

typedef struct
{
    UINT8   quant_luma[8][8];
    UINT8   quant_chroma[8][8];
    BITCODE huff_table[4][256];
    BITCODE vli_table[4096];
    UINT32  width;                  /* always positive: left to right */
    UINT32  height;                 /* always positive: top to bottom */
    BYTE    *data;                  /* jpeg data */
    SIZE_T  capacity;               /* max bytes that data can hold */
    SIZE_T  size;                   /* the number of bytes stored in data */
    UINT8   _buff, _nvacant;        /* bits buffer */
} JPEG, *pJPEG;

typedef const struct
{
    UINT8   id;
    UINT8   bits[16];
    UINT8   huffval[256];
} HUFFMAN;

const HUFFMAN HUFF[4] =
{
    /* Luma DC */
    {
        0x00,
        {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
        {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b}
    },
    /* Luma AC */
    {
        0x10,
        {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125},
        {
            0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
            0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
            0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
            0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
            0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
            0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
            0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
            0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
            0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
            0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
            0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
            0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
            0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
            0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
            0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
            0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
            0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
            0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
            0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
            0xf9, 0xfa
        }
    },
    /* Chroma DC */
    {
        0x01,
        {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0},
        {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b}
    },
    /* Chroma AC */
    {
        0x11,
        {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119},
        {
            0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
            0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
            0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
            0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
            0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
            0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
            0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
            0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
            0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
            0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
            0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
            0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
            0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
            0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
            0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
            0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
            0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
            0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
            0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
            0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
            0xf9, 0xfa
        }
    }
};

const int QUANT_LUMA[8][8] =
{
    {16,  11,  10,  16,  24,  40,  51,  61},
    {12,  12,  14,  19,  26,  58,  60,  55},
    {14,  13,  16,  24,  40,  57,  69,  56},
    {14,  17,  22,  29,  51,  87,  80,  62},
    {18,  22,  37,  56,  68, 109, 103,  77},
    {24,  35,  55,  64,  81, 104, 113,  92},
    {49,  64,  78,  87, 103, 121, 120, 101},
    {72,  92,  95,  98, 112, 100, 103,  99}
};

const int QUANT_CHROMA[8][8] =
{
    {17,  18,  24,  47,  99,  99,  99,  99},
    {18,  21,  26,  66,  99,  99,  99,  99},
    {24,  26,  56,  99,  99,  99,  99,  99},
    {47,  66,  99,  99,  99,  99,  99,  99},
    {99,  99,  99,  99,  99,  99,  99,  99},
    {99,  99,  99,  99,  99,  99,  99,  99},
    {99,  99,  99,  99,  99,  99,  99,  99},
    {99,  99,  99,  99,  99,  99,  99,  99}
};

const int JPEG_NATURAL_ORDER[] =
{
     0,   1,   8,  16,   9,   2,   3,  10,
    17,  24,  32,  25,  18,  11,   4,   5,
    12,  19,  26,  33,  40,  48,  41,  34,
    27,  20,  13,   6,   7,  14,  21,  28,
    35,  42,  49,  56,  57,  50,  43,  36,
    29,  22,  15,  23,  30,  37,  44,  51,
    58,  59,  52,  45,  38,  31,  39,  46,
    53,  60,  61,  54,  47,  55,  62,  63,
};

void bitcode_tostring(BITCODE code, char string[17])
{
    int i, j = 0;

    for (i = code.nbits - 1; i >= 0; i--)
    {
        string[j++] = ((code.value >> i) & 1) == 0 ? '0' : '1';
    }
    string[j] = '\0';
}

void error_exit(char *message)
{
    fprintf(stderr, "Error: %s\n", message);
    exit(EXIT_FAILURE);
}

pBITMAP bitmap_read(FILE *fp)
{
    pBITMAP bitmap;
    BYTE header[54];
    SIZE_T bitmap_size;

    if (fp == NULL)
    {
        error_exit(BMP_OPEN_ERROR);
    }

    if ((bitmap = malloc(sizeof(BITMAP))) == NULL)
    {
        error_exit(OUT_OF_MEMORY_ERROR);
    }

    if (fread(header, 1, 54, fp) < 54 || header[0] != 'B' || header[1] != 'M')
    {
        error_exit(BMP_INVALID_ERROR);
    }

    bitmap->width  = (UINT32) header[18]       | (UINT32) header[19] << 8 |
                     (UINT32) header[20] << 16 | (UINT32) header[21] << 24;
    bitmap->height = (UINT32) header[22]       | (UINT32) header[23] << 8 |
                     (UINT32) header[24] << 16 | (UINT32) header[25] << 24;

    if ((header[28] | header[29] << 8) != 24)
    {
        error_exit(BMP_NOT_24BIT_ERROR);
    }

    bitmap_size = labs(bitmap->height) * ((3 + labs(bitmap->width) * 3) & ~3);
    if ((bitmap->data = malloc(bitmap_size)) == NULL)
    {
        error_exit(OUT_OF_MEMORY_ERROR);
    }

    if (fread(bitmap->data, 1, bitmap_size, fp) < bitmap_size)
    {
        error_exit(BMP_CORRUPT_ERROR);
    }

    return bitmap;
}

RGB bitmap_get_rgb(pBITMAP bitmap, UINT32 x, UINT32 y)
{
    UINT32 width_abs, height_abs, a, b;
    RGB rgb_pixel = 0;
    SIZE_T pos;

    width_abs  = labs(bitmap->width);
    height_abs = labs(bitmap->height);
    if (x < 0 || y < 0 || x >= width_abs || y >= height_abs)
    {
        return 0;
    }
    a = (bitmap->height < 0) ? y :  height_abs - y - 1;
    b = (bitmap->width  > 0) ? x :  width_abs  - x - 1;

    pos = a * ((3 + width_abs * 3) & ~3) + b * 3;
    rgb_pixel |= ((RGB) bitmap->data[pos++] << 16);
    rgb_pixel |= ((RGB) bitmap->data[pos++] << 8 );
    rgb_pixel |= ((RGB) bitmap->data[pos++]      );

    return rgb_pixel;
}

void bitmap_free(pBITMAP bitmap)
{
    free(bitmap->data);
    free(bitmap);
}

FLOAT rgb_to_ycc(RGB rgb_pixel, int comp)
{
    UINT8 r, g, b;

    r =  rgb_pixel        & 0xff;
    g = (rgb_pixel >> 8)  & 0xff;
    b = (rgb_pixel >> 16) & 0xff;
    switch (comp)
    {
        /*  Y */ case 0:    return (       0.299 * r +       0.587 * g +       0.114 * b);
        /* Cb */ case 1:    return (-0.168735892 * r - 0.331264108 * g +         0.5 * b) + 128;
        /* Cr */ case 2:    return (         0.5 * r - 0.418687589 * g - 0.081312411 * b) + 128;
                default:    return 0;
    }
}

void dct_init(int quality, pJPEG jpeg)
{
    int i, j, factor, quant;
    if (quality <= 0)
    {
        factor = 5000;
    }
    else if (quality < 50)
    {
        factor = 5000 / quality;
    }
    else if (quality <= 100)
    {
        factor = 200 - quality * 2;
    }
    else
    {
        factor = 0;
    }
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 8; i++)
        {
            quant = (QUANT_LUMA[j][i] * factor + 50) / 100;
            if (quant <= 0)
            {
                quant = 1;
            }
            else if (quant > 255)
            {
                quant = 255;
            }
            jpeg->quant_luma[j][i] = quant;
        }
    }
    for (j = 0; j < 8; j++)
    {
        for (i = 0; i < 8; i++)
        {
            quant = (QUANT_CHROMA[j][i] * factor + 50) / 100;
            if (quant <= 0)
            {
                quant = 1;
            }
            else if (quant > 255)
            {
                quant = 255;
            }
            jpeg->quant_chroma[j][i] = quant;
        }
    }
}

void dct_forward(FLOAT matrix[8][8])
{
    /*
     * Reference:   Arai, Y., Agui, T. and Nakajima, M. (1988) A
     *              Fast DCT-SQ Scheme for Images. Transactions-
     *              IEICE, E-71, 1095-1097.
     */
    const FLOAT AAN_SCALE_FACTOR[] =
    {
        1.00000000000000000000,             /*  1.0                 */
        1.38703984532214746182,             /*  cos(2pi/16)sqrt(2)  */
        1.30656296487637652786,             /*  cos(3pi/16)sqrt(2)  */
        1.17587560241935871697,             /*  cos(4pi/16)sqrt(2)  */
        1.00000000000000000000,             /*  cos(5pi/16)sqrt(2)  */
        0.78569495838710218128,             /*  cos(6pi/16)sqrt(2)  */
        0.54119610014619698440,             /*  cos(7pi/16)sqrt(2)  */
        0.27589937928294301234              /*  cos(8pi/16)sqrt(2)  */
    };
    const FLOAT a1 = 0.70710678118654752440;   /*  cos(4pi/16)                 */
    const FLOAT a2 = 0.54119610014619698440;   /*  cos(2pi/16) - cos(6pi/16)   */
    const FLOAT a3 = 0.70710678118654752440;   /*  cos(4pi/16)                 */
    const FLOAT a4 = 1.30656296487637652786;   /*  cos(2pi/16) + cos(6pi/16)   */
    const FLOAT a5 = 0.38268343236508977173;   /*  cos(6pi/16)                 */

    int i, j;
    FLOAT tmp10, tmp11, tmp12, tmp13, tmp14, tmp15, tmp16, tmp17;
    FLOAT tmp20, tmp21, tmp22, tmp23, tmp24, tmp25, tmp26;
    FLOAT tmp32, tmp4_, tmp42, tmp44, tmp45, tmp46, tmp55, tmp57;

    /* rows */
    for (i = 0; i < 8; i++)
    {
        /* stage 1 */
        tmp10 = matrix[i][7] + matrix[i][0];
        tmp11 = matrix[i][6] + matrix[i][1];
        tmp12 = matrix[i][5] + matrix[i][2];
        tmp13 = matrix[i][4] + matrix[i][3];
        tmp14 = matrix[i][3] - matrix[i][4];
        tmp15 = matrix[i][2] - matrix[i][5];
        tmp16 = matrix[i][1] - matrix[i][6];
        tmp17 = matrix[i][0] - matrix[i][7];

        /* stage 2 */
        tmp20 =  tmp13 + tmp10;
        tmp21 =  tmp12 + tmp11;
        tmp22 =  tmp11 - tmp12;
        tmp23 =  tmp10 - tmp13;
        tmp24 = -tmp15 - tmp14;
        tmp25 =  tmp16 + tmp15;
        tmp26 =  tmp17 + tmp16;

        /* stage 3 */
        matrix[i][0] = tmp21 + tmp20;
        matrix[i][4] = tmp20 - tmp21;
        tmp32        = tmp23 + tmp22;

        /* stage 4 */
        tmp4_ =  a5 * (tmp24 + tmp26);
        tmp42 =  a1 * tmp32;
        tmp44 = -a2 * tmp24 - tmp4_;
        tmp45 =  a3 * tmp25;
        tmp46 =  a4 * tmp26 - tmp4_;

        /* stage 5 */
        matrix[i][2] =  tmp23 + tmp42;
        matrix[i][6] = -tmp42 + tmp23;
        tmp55        =  tmp17 + tmp45;
        tmp57        = -tmp45 + tmp17;

        /* stage 6 */
        matrix[i][5] =  tmp57 + tmp44;
        matrix[i][1] =  tmp46 + tmp55;
        matrix[i][7] =  tmp55 - tmp46;
        matrix[i][3] = -tmp44 + tmp57;
    }

    /* columns */
    for (i = 0; i < 8; i++)
    {
        /* stage 1 */
        tmp10 = matrix[7][i] + matrix[0][i];
        tmp11 = matrix[6][i] + matrix[1][i];
        tmp12 = matrix[5][i] + matrix[2][i];
        tmp13 = matrix[4][i] + matrix[3][i];
        tmp14 = matrix[3][i] - matrix[4][i];
        tmp15 = matrix[2][i] - matrix[5][i];
        tmp16 = matrix[1][i] - matrix[6][i];
        tmp17 = matrix[0][i] - matrix[7][i];

        /* stage 2 */
        tmp20 =  tmp13 + tmp10;
        tmp21 =  tmp12 + tmp11;
        tmp22 =  tmp11 - tmp12;
        tmp23 =  tmp10 - tmp13;
        tmp24 = -tmp15 - tmp14;
        tmp25 =  tmp16 + tmp15;
        tmp26 =  tmp17 + tmp16;

        /* stage 3 */
        matrix[0][i] = tmp21 + tmp20;
        matrix[4][i] = tmp20 - tmp21;
        tmp32        = tmp23 + tmp22;

        /* stage 4 */
        tmp4_ =  a5 * (tmp24 + tmp26);
        tmp42 =  a1 * tmp32;
        tmp44 = -a2 * tmp24 - tmp4_;
        tmp45 =  a3 * tmp25;
        tmp46 =  a4 * tmp26 - tmp4_;

        /* stage 5 */
        matrix[2][i] =  tmp23 + tmp42;
        matrix[6][i] = -tmp42 + tmp23;
        tmp55        =  tmp17 + tmp45;
        tmp57        = -tmp45 + tmp17;

        /* stage 6 */
        matrix[5][i] =  tmp57 + tmp44;
        matrix[1][i] =  tmp46 + tmp55;
        matrix[7][i] =  tmp55 - tmp46;
        matrix[3][i] = -tmp44 + tmp57;
    }

    /* scaling */
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            matrix[i][j] /= AAN_SCALE_FACTOR[i] * AAN_SCALE_FACTOR[j] * 8.0;
        }
    }
}

void dct_quantize(FLOAT matrix[8][8], int comp, pJPEG jpeg)
{
    int x, y, divisor;

    for (y = 0; y < 8; y++)
    {
        for (x = 0; x < 8; x++)
        {
            divisor = comp == 0 ? jpeg->quant_luma[y][x]: jpeg->quant_chroma[y][x];
            matrix[y][x] = (int)(matrix[y][x] / divisor + 0x4000 + 0.5) - 0x4000;
        }
    }
}

void huffman_init(pJPEG jpeg)
{
    int h, i, index, temp;
    int val, nbits, count;

    /* Huffman */
    HUFFMAN *huff;
    UINT8  code_nbits[256];
    BITCODE *huff_table, *pcode;

    for (h = 0; h < 4; h++)
    {
        huff = &HUFF[h];
        huff_table = jpeg->huff_table[h];

        index = 0;
        for (nbits = 1; nbits <= 16; nbits++)
        {
            for (i = 0; i < huff->bits[nbits-1]; i++)
            {
                code_nbits[index++] = nbits;
            }
        }
        count = index;

        val = 0;
        index = 0;
        nbits = code_nbits[0];
        while (index < count)
        {
            while (code_nbits[index] == nbits)
            {
                pcode = &(huff_table[huff->huffval[index]]);
                pcode->value = val;
                pcode->nbits = nbits;
                val++;
                index++;
            }
            val <<= 1;
            nbits++;
        }
    }

    /* VLI */
    for (i = -2048; i < 2048; i++)
    {
        index = i & 0xFFF;
        temp = val = i;
        if (temp < 0)
        {
            temp = -temp;       /* temp should be positive */
            val--;
        }
        nbits = 0;
        while (temp != 0)
        {
            nbits++;
            temp >>= 1;
        }
        val &= ~(-1 << nbits);
        jpeg->vli_table[index].value = val;
        jpeg->vli_table[index].nbits = nbits;
    }
}

void huffman_putcode(BITCODE bitcode, pJPEG jpeg)
{
    UINT16 value = bitcode.value;
    UINT8 length = bitcode.nbits;
    UINT8 shift, fragment;

    /* extend the output buffer */
    if (jpeg->capacity - jpeg->size < 4)
    {
        if ((jpeg->data = realloc(jpeg->data, jpeg->capacity * 2)) == NULL)
        {
            error_exit(OUT_OF_MEMORY_ERROR);
        }
        jpeg->capacity *= 2;
    }

    while (jpeg->_nvacant < length)
    {
        shift = length - jpeg->_nvacant;            /* bits remaining */
        fragment = value >> shift;
        jpeg->_buff <<= (jpeg->_nvacant);
        jpeg->_buff |= fragment;                    /* (jpeg->_nvacant = 0) */

        jpeg->data[(jpeg->size)++] = jpeg->_buff;   /* buffer is full, write and reset */
        if (jpeg->_buff == 0xff)
        {
            jpeg->data[(jpeg->size)++] = 0;         /* byte stuffing (T.81 P.91 F.1.2.3) */
        }
        jpeg->_buff = 0;
        jpeg->_nvacant = 8;

        value = value & ~(~0 << shift);             /* truncate written highs */
        length = shift;                             /* bits left to write */
    }

    /* buffer sufficient */
    jpeg->_buff <<= length;
    jpeg->_buff |= value;
    jpeg->_nvacant -= length;
}

void huffman_finish(pJPEG jpeg)
{
    if (jpeg->_nvacant != 0)                        /* buffer is not full, then left shift, pad with zero */
    {
        jpeg->_buff <<= (jpeg->_nvacant);
    }
    if (jpeg->_nvacant != 8)                        /* buffer is not empty, then write to stream */
    {
        jpeg->data[(jpeg->size)++] = jpeg->_buff;
        if (jpeg->_buff == 0xff)
        {
            jpeg->data[(jpeg->size)++] = 0;         /* byte stuffing (T.81 P.91 F.1.2.3) */
        }
    }
    jpeg->_buff = 0;
    jpeg->_nvacant = 8;
}

void huffman_encode(FLOAT matrix[8][8], int comp, int prev_dc, pJPEG jpeg)
{
    BITCODE vli_code, huff_code;
    BITCODE *dc_table, *ac_table;
    int dc_diff, ac_value;
    int x, y, k, r;

    if (comp == 0)  /* Luma */
    {
        dc_table = jpeg->huff_table[0];
        ac_table = jpeg->huff_table[1];
    }
    else            /* Chroma */
    {
        dc_table = jpeg->huff_table[2];
        ac_table = jpeg->huff_table[3];
    }

    /*
     * DC coefficient
     */
    dc_diff = (int) matrix[0][0] - prev_dc;

    vli_code = jpeg->vli_table[dc_diff & 0xfff];
    huff_code = dc_table[vli_code.nbits];

    huffman_putcode(huff_code, jpeg);
    if (vli_code.nbits != 0)
    {
        huffman_putcode(vli_code, jpeg);
    }

    /*
     * AC coefficients
     */
    r = 0;
    for (k = 1; k < 64; k++)
    {
        x = JPEG_NATURAL_ORDER[k] % 8;
        y = JPEG_NATURAL_ORDER[k] / 8;
        ac_value = (int) matrix[y][x];
        if (ac_value == 0)
        {
            r++;
        }
        else
        {
            while (r > 15)
            {
                /* ZRL */
                huff_code = ac_table[0xf0];
                huffman_putcode(huff_code, jpeg);
                r -= 16;
            }
            vli_code = jpeg->vli_table[ac_value & 0xfff];
            huff_code = ac_table[(r << 4) | vli_code.nbits];
            huffman_putcode(huff_code, jpeg);
            huffman_putcode(vli_code, jpeg);
            r = 0;
        }
    }
    if (r > 0)
    {
        /* EOB */
        huff_code = ac_table[0x00];
        huffman_putcode(huff_code, jpeg);
    }
}

void jpeg_put_header(pBITMAP bitmap, pJPEG jpeg)
{
    HUFFMAN *huff;
    SIZE_T temp01, temp02;
    int i, j, k;
    const int comp_id[3]        = {1, 2, 3};
    const int h_samp_factor[3]  = {2, 1, 1};
    const int v_samp_factor[3]  = {2, 1, 1};
    const int quant_table_id[3] = {0, 1, 1};
    const int dc_table_id[3]    = {0, 1, 1};
    const int ac_table_id[3]    = {0, 1, 1};

    /*
     * Start of Image Marker
     */
    jpeg->data[jpeg->size++] = 0xff;
    jpeg->data[jpeg->size++] = 0xd8;

    /*
     * Start of Frame Header (T.81 P.36)
     */
    jpeg->data[jpeg->size++] = 0xff;                                        /* SOF0 marker - 0xFFC0 */
    jpeg->data[jpeg->size++] = 0xc0;
    jpeg->data[jpeg->size++] = 0x00;                                        /* Length of segment excluding SOF0 marker */
    jpeg->data[jpeg->size++] = 0x11;
    jpeg->data[jpeg->size++] = 0x08;                                        /* Sample precision */
    jpeg->data[jpeg->size++] = (jpeg->height >> 8 & 0xff);                  /* Number of lines */
    jpeg->data[jpeg->size++] = (jpeg->height      & 0xff);
    jpeg->data[jpeg->size++] = (jpeg->width  >> 8 & 0xff);                  /* Number of samples per line */
    jpeg->data[jpeg->size++] = (jpeg->width       & 0xff);
    jpeg->data[jpeg->size++] = 0x03;                                        /* Number of image components in frame */

    for (i = 0; i < 3; i++)
    {
        jpeg->data[jpeg->size++] = comp_id[i];                              /* Component identifier */
        jpeg->data[jpeg->size++] = (( h_samp_factor[i] << 4 )|              /* Horizontal sampling factor */
                                      v_samp_factor[i]       );             /* Vertical sampling factor */
        jpeg->data[jpeg->size++] = quant_table_id[i];                       /* Quantization table destination selector */
    }

    /*
     * Define Quantization Table header (T.81 P.39)
     */
    jpeg->data[jpeg->size++] = 0xff;                                        /* DQT marker - 0xFFDB */
    jpeg->data[jpeg->size++] = 0xdb;
    jpeg->data[jpeg->size++] = 0x00;                                        /* Length of segment excluding DQT marker */
    jpeg->data[jpeg->size++] = 0x84;
    for (i = 0; i < 2; i++)
    {
        jpeg->data[jpeg->size++] = (( 0 << 4 )|                             /* Quantization table element precision */
                                      i       );                            /* Quantization table destination identifier */
        for (j = 0; j < 64; j++)
        {
            k = JPEG_NATURAL_ORDER[j];
            jpeg->data[jpeg->size++] = (i == 0) ?
                                       jpeg->quant_luma  [k / 8][k % 8] :
                                       jpeg->quant_chroma[k / 8][k % 8];
        }
    }

    /*
     * Define Huffman Table Header (T.81 P.40)
     */
    jpeg->data[jpeg->size++] = 0xff;                                        /* DHT marker - 0xFFC4 */
    jpeg->data[jpeg->size++] = 0xc4;
    temp01 = jpeg->size++;                                                  /* (position of length) */
    temp02 = jpeg->size++;
    for (i = 0; i < 4; i++)
    {
        huff = &HUFF[i];
        jpeg->data[jpeg->size++] = huff->id;                                /* Table class & Huffman table destination id */
        k = 0;
        for (j = 0; j < 16; j++)
        {
            k += (jpeg->data[jpeg->size++] = huff->bits[j]);                /* Number of Huffman codes of length i */
        }
        for (j = 0; j < k; j++)
        {
            jpeg->data[jpeg->size++] = huff->huffval[j];                    /* Value associated with each Huffman code */
        }
    }
    k = (int) (jpeg->size - temp01);
    jpeg->data[temp01] = (k >> 8) & 0xff;                                   /* Length of segment excluding DHT marker */
    jpeg->data[temp02] = (k     ) & 0xff;

    /*
     * Start of Scan Header (T.81 P.37)
     */
    jpeg->data[jpeg->size++] = 0xff;                                        /* SOS marker - 0xFFDA */
    jpeg->data[jpeg->size++] = 0xda;
    jpeg->data[jpeg->size++] = 0x00;                                        /* Length of segment excluding SOS marker */
    jpeg->data[jpeg->size++] = 0x0c;
    jpeg->data[jpeg->size++] = 0x03;                                        /* Number of image components in scan */
    for (i = 0; i < 3; i++)
    {
        jpeg->data[jpeg->size++] = comp_id[i];
        jpeg->data[jpeg->size++] = (( dc_table_id[i] << 4 )|                /* DC entropy coding table destination selector */
                                      ac_table_id[i]       );               /* AC entropy coding table destination selector */
    }
    jpeg->data[jpeg->size++] = 0x00;                                        /* Start of spectral or predictor selection */
    jpeg->data[jpeg->size++] = 0x3f;                                        /* End of spectral selection */
    jpeg->data[jpeg->size++] = 0x00;                                        /* Successive approximation bit position high & low */
}

void jpeg_put_eoi(pJPEG jpeg)
{
    /*
     * End of Image Marker
     */
    jpeg->data[jpeg->size++] = 0xff;                                        /* EOI marker - 0xFFD9 */
    jpeg->data[jpeg->size++] = 0xd9;
}

pJPEG jpeg_create_from_bmp(pBITMAP bitmap, int quality)
{
    pJPEG jpeg;
    RGB pixel_rgb;
    FLOAT block_matrix[8][8];
    UINT32 x_unit_count, y_unit_count, x_unit, y_unit;
    UINT32 x_base, y_base, x_pos, y_pos;
    int comp, a, b, x_factor, y_factor, x_block, y_block;
    int prev_dc[3] = { 0 };

    /* 4:2:0 chroma subsampling */
    int h_samp_factor[3] = {2, 1, 1};
    int v_samp_factor[3] = {2, 1, 1};
    int x_factor_max = 2, y_factor_max = 2;

    if ((jpeg = malloc(sizeof(JPEG))) == NULL)
    {
        error_exit(OUT_OF_MEMORY_ERROR);
    }
    jpeg->width = labs(bitmap->width);
    jpeg->height = labs(bitmap->height);
    jpeg->size = 0;
    jpeg->_buff = 0;
    jpeg->_nvacant = 8;

    /*
     * 1024 is enough to hold the header, and we estimate
     * the resulting jpeg size to be width * height / 4.
     * If it's not enough, the buffer will be expanded in
     * the future.
     */
    jpeg->capacity = 1024 + jpeg->width * jpeg->height / 4;

    if ((jpeg->data = malloc(jpeg->capacity)) == NULL)
    {
        error_exit(OUT_OF_MEMORY_ERROR);
    }

    huffman_init(jpeg);
    dct_init(quality, jpeg);
    jpeg_put_header(bitmap, jpeg);

    x_unit_count = (7 + jpeg->width  / x_factor_max) >> 3;
    y_unit_count = (7 + jpeg->height / y_factor_max) >> 3;
    /* Minimum Coded Units */
    for (y_unit = 0; y_unit < y_unit_count; y_unit++)
    {
        for (x_unit = 0; x_unit < x_unit_count; x_unit++)
        {
            x_base = x_unit * 8 * x_factor_max;
            y_base = y_unit * 8 * y_factor_max;
            /* Components */
            for (comp = 0; comp < 3; comp++)
            {
                x_factor = h_samp_factor[comp];
                y_factor = v_samp_factor[comp];
                /* DCT Blocks */
                for (y_block = 0; y_block < y_factor; y_block++)
                {
                    for (x_block = 0; x_block < x_factor; x_block++)
                    {
                        /* Pixels */
                        for (b = 0; b < 8; b++)
                        {
                            for (a = 0; a < 8; a++)
                            {
                                x_pos = x_base + a * x_factor_max / x_factor + x_block * 8;
                                y_pos = y_base + b * y_factor_max / y_factor + y_block * 8;
                                pixel_rgb = bitmap_get_rgb(bitmap, x_pos, y_pos);
                                block_matrix[b][a] = rgb_to_ycc(pixel_rgb, comp) - 128.0;
                            }
                        }
                        dct_forward(block_matrix);
                        dct_quantize(block_matrix, comp, jpeg);
                        huffman_encode(block_matrix, comp, prev_dc[comp], jpeg);

                        prev_dc[comp] = (int) block_matrix[0][0];
                    }
                }
            }
        }
    }
    huffman_finish(jpeg);
    jpeg_put_eoi(jpeg);
    return jpeg;
}

void jpeg_save(pJPEG jpeg, FILE* fp)
{
    fwrite(jpeg->data, 1, jpeg->size, fp);
}

void jpeg_free(pJPEG jpeg)
{
    free(jpeg->data);
    free(jpeg);
}

int main(int argc, char *argv[])
{
    int quality = 75;
    char *ptr;
    FILE *in_file, *out_file;

    pBITMAP bitmap;
    pJPEG jpeg;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s INPUT.bmp OUTPUT.jpg [quality]\n", argv[0]);
        return EXIT_FAILURE;
    }
    else if (argc > 3)
    {
        quality = (int) strtol(argv[3], &ptr, 10);
        if (*ptr != '\0' || quality < 0 || quality > 100)
        {
            fprintf(stderr, "The value of quality should be between 0 and 100.\n\n"
                            "Usage: %s INPUT.bmp OUTPUT.jpg [quality]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    in_file = fopen(argv[1], "rb");
    out_file = fopen(argv[2], "wb");

    bitmap = bitmap_read(in_file);
    jpeg = jpeg_create_from_bmp(bitmap, quality);
    jpeg_save(jpeg, out_file);

    fclose(in_file);
    fclose(out_file);

    bitmap_free(bitmap);
    jpeg_free(jpeg);

    return EXIT_SUCCESS;
}
