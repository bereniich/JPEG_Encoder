#include "JPEG.h"
#include "NxNDCT.h"
#include <math.h>
#include "JPEGBitStreamWriter.h"
#include <algorithm>

#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)

// Quantization tables from JPEG Standard, Annex K
static const uint8_t QuantLuminance[8*8] =
{
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68,109,103, 77,
    24, 35, 55, 64, 81,104,113, 92,
    49, 64, 78, 87,103,121,120,101,
    72, 92, 95, 98,112,100,103, 99
};

static const uint8_t QuantChrominance[8*8] =
{
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

// Structure to hold extended image dimensions and DCT coefficients
struct imageProperties
{
    int width;
    int height;
    int16_t* coeffs;
};

// DCT specifically for Chrominance (U and V) components
// Does not perform the -128 level shift because U/V are already centered around 0
void DCTUandV(const char input[], int16_t output[], int N, double* DCTKernel)
{
    double* temp = new double[N * N];
    double* DCTCoefficients = new double[N * N];
    double sum;

    // Matrix Multiplication: DCTKernel * input
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + DCTKernel[i*N+k] * (double)((signed char)input[k*N+j]);
            }
            temp[i*N + j] = sum;
        }
    }

    // Matrix Multiplication: (DCTKernel * input) * DCTKernel^T
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
                sum = sum + temp[i*N+k] * DCTKernel[j*N+k];
            DCTCoefficients[i*N+j] = sum;
        }
    }

    // Rounding coefficients to the nearest integer
    for(int i = 0; i < N*N; i++)
        output[i] = (int16_t)floor(DCTCoefficients[i] + 0.5);

    delete[] temp;
    delete[] DCTCoefficients;
}

// Adjusts quantization tables based on the desired quality factor (1-100)
uint16_t quantQuality(uint8_t quant, uint8_t quality)
{
    // Convert to an internal JPEG quality factor, formula taken from libjpeg
    int16_t q = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return clamp<int>((quant * q + 50) / 100, 1, 255);
}

// Reorders an 8x8 block into Zig-Zag order for better compression
static void doZigZag(int16_t block[], uint8_t quantizationBlock[], int N, int DCTorQuantization)
{
    const uint8_t ZigZagOrder[64] = {
         0,  1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };

    if (N != 8)
        return;

    if (DCTorQuantization == 0)
    {
        // Processing DCT coefficients
        int16_t temp[64];
        for (int i = 0; i < N * N; i++) temp[i] = block[ZigZagOrder[i]];
        for (int i = 0; i < N * N; i++) block[i] = temp[i];
    }
    else
    {
        // Processing Quantization table
        uint8_t temp[64];
        for (int i = 0; i < N * N; i++) temp[i] = quantizationBlock[ZigZagOrder[i]];
        for (int i = 0; i < N * N; i++) quantizationBlock[i] = temp[i];
    }
}

// Performs DCT and Quantization for the entire image component (Y, U, or V)
imageProperties performDCT(char input[], int xSize, int ySize, int N, uint8_t quality, bool quantType)
{
    // Buffer for a single NxN input block (unsigned to handle 0-255 correctly)
    unsigned char* inBlock = new unsigned char[N * N];
    int16_t* dctCoeffs = new int16_t[N * N];

    // Initialize DCT kernel matrix
    double* DCTKernel = new double[N * N];
    GenerateDCTmatrix(DCTKernel, N);

    // Padding image to ensure dimensions are multiples of N (8 or 16)
    int xSizeExtend, ySizeExtend;
    char* inputExtend;
    extendBorders(input, xSize, ySize, N, &inputExtend, &xSizeExtend, &ySizeExtend);

    // Total coefficients for the processed image
    int totalElements = xSizeExtend * ySizeExtend;
    int16_t* finalCoeffs = new int16_t[totalElements];

    // Scale quantization matrix by quality factor
    uint16_t quantMatrix[N * N];
    for(int i = 0; i < N * N; i++)
    {
        uint8_t baseQuant = quantType ? QuantChrominance[i] : QuantLuminance[i];
        quantMatrix[i] = quantQuality(baseQuant, quality);
    }

    // Iterate through image blocks
    for(int by = 0; by < ySizeExtend / N; by++)
    {
        for(int bx = 0; bx < xSizeExtend / N; bx++)
        {
            // Loading block
            for(int j = 0; j < N; j++) {
                for(int i = 0; i < N; i++) {
                    inBlock[j * N + i] = (unsigned char)inputExtend[(by * N + j) * xSizeExtend + (bx * N + i)];
                }
            }

            // Apply DCT
            if(!quantType)
                // For Luma (Y) - includes level shift
                DCT((const char*)inBlock, dctCoeffs, N, DCTKernel);
            else
                // For Chroma (U/V) - no level shift
                DCTUandV((const char*)inBlock, dctCoeffs, N, DCTKernel);

            // Divide by quantization matrix and round
            for(int k = 0; k < N * N; k++)
            {
                double val = (double)dctCoeffs[k] / (double)quantMatrix[k];
                int16_t rounded = (val >= 0) ?
                    (int16_t)(val + 0.5) :
                    (int16_t)(val - 0.5);

                // Clamp to 12-bit range used in JPEG
                if (rounded > 2047) rounded = 2047;
                else if (rounded < -2048) rounded = -2048;

                dctCoeffs[k] = rounded;
            }

            // Zig-Zag: Reorder coefficients for entropy coding
            doZigZag(dctCoeffs, NULL, N, 0);

            // // Store block into final flattened coefficient array
            int blockOffset = (by * (xSizeExtend / N) + bx) * (N * N);
            for(int k = 0; k < N * N; k++)
                finalCoeffs[blockOffset + k] = dctCoeffs[k];
        }
    }

    // Prepare output strucure
    imageProperties result = {
        xSizeExtend,
        ySizeExtend,
        finalCoeffs
    };

    // Clean up
    delete[] dctCoeffs;
    delete[] inBlock;
    delete[] DCTKernel;
    delete[] inputExtend;

    return result;
}

// Orchestrates the full JPEG encoding process
void performJPEGEncoding(uchar Y_buff[], char U_buff[], char V_buff[], int xSize, int ySize, int quality) {
    DEBUG(quality);
    auto s = new JPEGBitStreamWriter("example.jpg");
    s->writeHeader();

    const int N = 8;

    uint8_t qLuma[N * N];
    uint8_t qChroma[N * N];

    // Generate quality-adjusted quantization tables
    for(int i = 0; i < N * N; i++)
    {
        qLuma[i] = quantQuality(QuantLuminance[i], (uint8_t)quality);
        qChroma[i] = quantQuality(QuantChrominance[i], (uint8_t)quality);
    }

    // Quantization tables must be stored in Zig-Zag order in the JPEG file
    doZigZag(NULL, qLuma, N, 1);
    doZigZag(NULL, qChroma, N, 1);

    s->writeQuantizationTables(qLuma, qChroma);

    // Process Y, U, and V
    imageProperties Y = performDCT((char*)Y_buff, xSize, ySize, 8, (uint8_t)quality, false);
    imageProperties U = performDCT(U_buff, xSize / 2, ySize / 2, 8, (uint8_t)quality, true);
    imageProperties V = performDCT(V_buff, xSize / 2, ySize / 2, 8, (uint8_t)quality, true);

    s->writeImageInfo(xSize, ySize);
    s->writeHuffmanTables();

    // 4:2:0 Subsampling MCU (Minimum Coded Unit) = 16x16 pixels
    // One MCU consists of four 8x8 Y blocks, one 8x8 U block, and one 8x8 V block
    int mcuPerRow = Y.width / 16;
    int mcuPerCol = Y.height / 16;
    int yBlocksPerRow = Y.width / 8; // Ovo mora biti xSizeExtend / 8

    for (int my = 0; my < mcuPerCol; my++) {
        for (int mx = 0; mx < mcuPerRow; mx++) {

            // Calculate starting index for Y blocks in the MCU
            int yBase = (my * 2 * yBlocksPerRow) + (mx * 2);

            // Write 4 Y blocks (Top-Left, Top-Right, Bottom-Left, Bottom-Right)
            s->writeBlockY(&Y.coeffs[yBase * 64]);
            s->writeBlockY(&Y.coeffs[(yBase + 1) * 64]);
            s->writeBlockY(&Y.coeffs[(yBase + yBlocksPerRow) * 64]);
            s->writeBlockY(&Y.coeffs[(yBase + yBlocksPerRow + 1) * 64]);

            // Write corresponding Chroma blocks (each covers the same 16x16 area)
            int uvBlocksPerRow = U.width / 8;
            int uvIdx = (my * uvBlocksPerRow) + mx;

            s->writeBlockU(&U.coeffs[uvIdx * 64]);
            s->writeBlockV(&V.coeffs[uvIdx * 64]);
        }
    }

    s->finishStream();

    // Compression Statistics Calculation
    long originalSize = (xSize * ySize) + (xSize / 2 * ySize / 2) * 2;
    long compressedSize = 0;

    FILE* f = fopen("example.jpg", "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        compressedSize = ftell(f);
        fclose(f);
    }

    double originalKiB = (double)originalSize / 1024.0;
    double compressedKiB = (double)compressedSize / 1024.0;
    double compressionRatio = (double)originalSize / compressedSize;
    double savingsPercent = (1.0 - ((double)compressedSize / originalSize)) * 100.0;

    // --- Compression Statistics Output ---
    printf("\n************************************************\n");
    printf("           JPEG ENCODER STATISTICS            \n");
    printf("************************************************\n");
    char dimStr[20];
    sprintf(dimStr, "%d x %d", xSize, ySize);
    printf("%-25s %-15s\n", "Dimensions:", dimStr);
    printf("%-25s %-15d\n", "Quality Factor:", quality);
    printf("------------------------------------------------\n");
    printf("%-25s %-10.2f %-4s\n", "Input Image Size:", originalKiB, "KiB");
    printf("%-25s %-10.2f %-4s\n", "Output Image Size:", compressedKiB, "KiB");
    printf("------------------------------------------------\n");
    printf("%-25s %-10.2f %-4s\n", "Compression Ratio:", compressionRatio, "x");
    printf("%-25s %-10.2f %-4s\n", "Storage Savings:", savingsPercent, "%");
    printf("************************************************\n");

    // Clean up
    delete[] Y.coeffs;
    delete[] U.coeffs;
    delete[] V.coeffs;
    delete s;
}
