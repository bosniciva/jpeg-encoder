#include "JPEG.h"
#include "NxNDCT.h"
#include <math.h>

#include "JPEGBitStreamWriter.h"


#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)



// quantization tables from JPEG Standard, Annex K
uint8_t QuantLuminance[8*8] =
    { 16, 11, 10, 16, 24, 40, 51, 61,
      12, 12, 14, 19, 26, 58, 60, 55,
      14, 13, 16, 24, 40, 57, 69, 56,
      14, 17, 22, 29, 51, 87, 80, 62,
      18, 22, 37, 56, 68,109,103, 77,
      24, 35, 55, 64, 81,104,113, 92,
      49, 64, 78, 87,103,121,120,101,
      72, 92, 95, 98,112,100,103, 99 };
uint8_t QuantChrominance[8*8] =
    { 17, 18, 24, 47, 99, 99, 99, 99,
      18, 21, 26, 66, 99, 99, 99, 99,
      24, 26, 56, 99, 99, 99, 99, 99,
      47, 66, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99 };

static char quantizationMatrix[64] =
{
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

struct imageProperties{
    int width;
    int height;
    int16_t* coeffs;
};


void DCTUandV(const char input[], int16_t output[], int N, double* DCTKernel)
{
    double* temp = new double[N*N];
    double* DCTCoefficients = new double[N*N];

    double sum;
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + DCTKernel[i*N+k] * (input[k*N+j]);
            }
            temp[i*N + j] = sum;
        }
    }

    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + temp[i*N+k] * DCTKernel[j*N+k];
            }
            DCTCoefficients[i*N+j] = sum;
        }
    }

    for(int i = 0; i < N*N; i++)
    {
        output[i] = floor(DCTCoefficients[i]+0.5);
    }

    delete[] temp;
    delete[] DCTCoefficients;

    return;
}

uint8_t quantQuality(uint8_t quant, uint8_t quality) {
    // Convert to an internal JPEG quality factor, formula taken from libjpeg
    int16_t q = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return clamp((quant * q + 50) / 100, 1, 255);
}

static const int zigzagIndex[64] = {
     0,  1,  5,  6, 14, 15, 27, 28,
     2,  4,  7, 13, 16, 26, 29, 42,
     3,  8, 12, 17, 25, 30, 41, 43,
     9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

static void doZigZag(int16_t block[], uint8_t quantizationBlock[], int N, int DCTorQuantization)
{
    /* TO DO */
    if (N != 8) return;

    if (DCTorQuantization == 0) {
        int16_t tmp[64];
        int16_t quantized[64];

        for (int k = 0; k < 64; ++k) tmp[k] = block[k];

        for (int i = 0; i < 64; ++i) {
            uint8_t q = quantizationBlock ? quantizationBlock[i] : 1;
            double v = (double)tmp[i] / (double)q;
            int16_t r = (int16_t) ( (v >= 0.0) ? floor(v + 0.5) : (int)ceil(v - 0.5) );
            quantized[i] = r;
        }

        for (int k = 0; k < 64; ++k) {
            block[k] = quantized[ zigzagIndex[k] ];
        }
    } else {
        int16_t tmp[64];
        for (int k = 0; k < 64; ++k) tmp[ zigzagIndex[k] ] = block[k];

        for (int i = 0; i < 64; ++i) {
            uint8_t q = quantizationBlock ? quantizationBlock[i] : 1;
            int val = (int)tmp[i] * (int)q;
            block[i] = (int16_t)val;
        }
    }
}

/* perform DCT */
imageProperties performDCT(char input[], int xSize, int ySize, int N, uint8_t quality, bool quantType)
{
	// TO DO
    imageProperties img;
    img.width = xSize;
    img.height = ySize;

    char* padded = nullptr;
    int newX = 0, newY = 0;
    if ((xSize % N != 0) || (ySize % N != 0)) {
        extendBorders(input, xSize, ySize, N, &padded, &newX, &newY);
    } else {
        padded = input;
        newX = xSize;
        newY = ySize;
    }

    int blocksPerRow = newX / N;
    int blocksPerCol = newY / N;
    int totalBlocks = blocksPerRow * blocksPerCol;
    img.coeffs = new int16_t[ totalBlocks * N * N ];

    double* DCTKernel = new double[N*N];
    for (int u = 0; u < N; ++u) {
        for (int x = 0; x < N; ++x) {
            DCTKernel[u*N + x] = cos(((2.0 * x + 1.0) * u * M_PI) / (2.0 * N));
        }
    }

    uint8_t quantTable[64];
    for (int k = 0; k < 64; ++k) {
        uint8_t base = quantType ? QuantChrominance[k] : QuantLuminance[k];
        quantTable[k] = quantQuality(base, quality);
        if (quantTable[k] < 1) quantTable[k] = 1;
    }

    int blockIndex = 0;
    int16_t dctOut[64];

    for (int by = 0; by <= newY - N; by += N)
    {
        for (int bx = 0; bx <= newX - N; bx += N)
        {
            char blockIn[64];
            for (int yy = 0; yy < N; ++yy) {
                for (int xx = 0; xx < N; ++xx) {
                    int px = bx + xx;
                    int py = by + yy;
                    int srcIdx = py * newX + px;
                    if (!quantType) {
                        unsigned char v = (unsigned char) padded[srcIdx];
                        blockIn[yy*N + xx] = (char)((int)v - 128);
                    } else {
                        blockIn[yy*N + xx] = padded[srcIdx];
                    }
                }
            }

            DCTUandV(blockIn, dctOut, N, DCTKernel);

            int16_t blockCopy[64];
            for (int k = 0; k < 64; ++k) blockCopy[k] = dctOut[k];

            doZigZag(blockCopy, quantTable, N, 0);
            for (int k = 0; k < 64; ++k) {
                img.coeffs[ blockIndex * 64 + k ] = blockCopy[k];
            }
            ++blockIndex;
        }
    }

    delete[] DCTKernel;
    if (padded != input) delete[] padded;
    return img;
}

//JPEGBitStreamWriter streamer("example.jpg");
void performJPEGEncoding(uchar Y_buff[], char U_buff[], char V_buff[], int xSize, int ySize, int quality)
{
	DEBUG(quality);
    auto s = new JPEGBitStreamWriter("example.jpg");
	// TO DO
    s->writeHeader();

    uint8_t LQ[64], CQ[64];
    for (int k = 0; k < 64; ++k) {
        LQ[k] = quantQuality(QuantLuminance[k], (uint8_t)quality);
        CQ[k] = quantQuality(QuantChrominance[k], (uint8_t)quality);
    }

    s->writeQuantizationTables(LQ, CQ);
    s->writeImageInfo(xSize, ySize);
    s->writeHuffmanTables();

    imageProperties Yimg = performDCT((char*)Y_buff, xSize, ySize, 8, quality, false);
    imageProperties Uimg = performDCT(U_buff, xSize / 2, ySize / 2, 8, quality, true);
    imageProperties Vimg = performDCT(V_buff, xSize / 2, ySize / 2, 8, quality, true);

    int blockIndexY = 0, blockIndexU = 0, blockIndexV = 0;
    for (int by = 0; by < ySize; by += 16) {
        for (int bx = 0; bx < xSize; bx += 16) {
            for (int b = 0; b < 4; ++b) {
                s->writeBlockY(&Yimg.coeffs[blockIndexY * 64]);
                    ++blockIndexY;
            }

            s->writeBlockU(&Uimg.coeffs[blockIndexU * 64]);
            s->writeBlockV(&Vimg.coeffs[blockIndexV * 64]);
            ++blockIndexU;
            ++blockIndexV;
        }
    }

    s->finishStream();

    delete[] Yimg.coeffs;
    delete[] Uimg.coeffs;
    delete[] Vimg.coeffs;
}
