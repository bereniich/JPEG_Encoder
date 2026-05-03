#include "ColorSpaces.h"
#include <algorithm>

/********************************************************************************************************************************/
/* RGB processing */
/********************************************************************************************************************************/
void processing_RGB(const uchar rgbInputImg[], int x, int y, uchar rgbOutputImg[], double R, double G, double B)
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i++)
        {
            int idx = (j * x + i) * 3;

            double r_val = R * rgbInputImg[idx];
            double g_val = G * rgbInputImg[idx + 1];
            double b_val = B * rgbInputImg[idx + 2];

            rgbOutputImg[idx]     = (uchar)(r_val > 255 ? 255 : (r_val < 0 ? 0 : r_val));
            rgbOutputImg[idx + 1] = (uchar)(g_val > 255 ? 255 : (g_val < 0 ? 0 : g_val));
            rgbOutputImg[idx + 2] = (uchar)(b_val > 255 ? 255 : (b_val < 0 ? 0 : b_val));
        }
    }
}

/********************************************************************************************************************************/
/* YUV444 processing */
/********************************************************************************************************************************/
void RGBtoYUV444(const uchar rgbImg[], int x, int y, uchar Y_buff[], signed char U_buff[], signed char V_buff[])
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i++)
        {
            int rgb_idx = (j * x + i) * 3;
            int yuv_idx = j * x + i;

            uchar R = rgbImg[rgb_idx];
            uchar G = rgbImg[rgb_idx + 1];
            uchar B = rgbImg[rgb_idx + 2];

            Y_buff[yuv_idx] = (uchar)(0.299 * R + 0.587 * G + 0.114 * B + 0.5);
            U_buff[yuv_idx] = (signed char)(-0.14713 * R - 0.28886 * G + 0.436 * B);
            V_buff[yuv_idx] = (signed char)(0.615 * R - 0.51499 * G - 0.10001 * B);
        }
    }
}

void YUV444toRGB(const uchar Y_buff[], const signed char U_buff[], const signed char V_buff[], int x, int y, uchar rgbImg[])
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i++)
        {
            int idx = j * x + i;
            double Y = Y_buff[idx];
            double U = U_buff[idx];
            double V = V_buff[idx];

            double R = Y + 1.13983 * V;
            double G = Y - 0.39465 * U - 0.58060 * V;
            double B = Y + 2.03211 * U;

            int rgb_idx = idx * 3;
            rgbImg[rgb_idx]     = (uchar)(R > 255 ? 255 : (R < 0 ? 0 : R));
            rgbImg[rgb_idx + 1] = (uchar)(G > 255 ? 255 : (G < 0 ? 0 : G));
            rgbImg[rgb_idx + 2] = (uchar)(B > 255 ? 255 : (B < 0 ? 0 : B));
        }
    }
}

void processing_YUV444(uchar Y_buff[], signed char U_buff[], signed char V_buff[], int x, int y, double Y, double U, double V)
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i++)
        {
            int idx = j * x + i;

            double y_val = Y_buff[idx] * Y;
            double u_val = U_buff[idx] * U;
            double v_val = V_buff[idx] * V;

            Y_buff[idx] = (uchar)(y_val > 255 ? 255 : (y_val < 0 ? 0 : y_val));
            U_buff[idx] = (signed char)(u_val > 127 ? 127 : (u_val < -128 ? -128 : u_val));
            V_buff[idx] = (signed char)(v_val > 127 ? 127 : (v_val < -128 ? -128 : v_val));
        }
    }
}

/*******************************************************************************************************************************/
/* YUV422 processing */
/********************************************************************************************************************************/
void RGBtoYUV422(const uchar rgbImg[], int x, int y, uchar Y_buff[], signed char U_buff[], signed char V_buff[])
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i += 2)
        {
            // The first pixel in a pair
            int idx1 = (j * x + i) * 3;
            uchar R1 = rgbImg[idx1];
            uchar G1 = rgbImg[idx1 + 1];
            uchar B1 = rgbImg[idx1 + 2];

            // The second pixel in the pair
            int idx2 = (j * x + i + 1) * 3;
            uchar R2 = rgbImg[idx2];
            uchar G2 = rgbImg[idx2 + 1];
            uchar B2 = rgbImg[idx2 + 2];

            Y_buff[j * x + i]     = (uchar)(0.299 * R1 + 0.587 * G1 + 0.114 * B1 + 0.5);
            Y_buff[j * x + i + 1] = (uchar)(0.299 * R2 + 0.587 * G2 + 0.114 * B2 + 0.5);

            // Average value for Chroma
            double U = ((-0.14713*R1 - 0.28886*G1 + 0.436*B1) + (-0.14713*R2 - 0.28886*G2 + 0.436*B2)) / 2.0;
            double V = ((0.615*R1 - 0.51499*G1 - 0.10001*B1) + (0.615*R2 - 0.51499*G2 - 0.10001*B2)) / 2.0;

            U_buff[j * (x/2) + (i/2)] = (signed char)U;
            V_buff[j * (x/2) + (i/2)] = (signed char)V;
        }
    }
}

void YUV422toRGB(const uchar Y_buff[], const signed char U_buff[], const signed char V_buff[], int x, int y, uchar rgbImg[])
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i += 2)
        {
            double U = U_buff[j * (x/2) + (i/2)];
            double V = V_buff[j * (x/2) + (i/2)];

            for(int p = 0; p < 2; p++) // Za oba piksela u paru
            {
                double Y = Y_buff[j * x + i + p];
                double R = Y + 1.13983 * V;
                double G = Y - 0.39465 * U - 0.58060 * V;
                double B = Y + 2.03211 * U;

                int rgb_idx = (j * x + i + p) * 3;
                rgbImg[rgb_idx]     = (uchar)(R > 255 ? 255 : (R < 0 ? 0 : R));
                rgbImg[rgb_idx + 1] = (uchar)(G > 255 ? 255 : (G < 0 ? 0 : G));
                rgbImg[rgb_idx + 2] = (uchar)(B > 255 ? 255 : (B < 0 ? 0 : B));
            }
        }
    }
}

void processing_YUV422(uchar Y_buff[], signed char U_buff[], signed char V_buff[], int x, int y, double Y, double U, double V)
{
    for(int j = 0; j < y; j++)
    {
        for(int i = 0; i < x; i++)
        {
            double y_val = Y_buff[j * x + i] * Y;
            Y_buff[j * x + i] = (uchar)(y_val > 255 ? 255 : (y_val < 0 ? 0 : y_val));
        }
        for(int i = 0; i < x/2; i++)
        {
            double u_val = U_buff[j * (x/2) + i] * U;
            double v_val = V_buff[j * (x/2) + i] * V;
            U_buff[j * (x/2) + i] = (signed char)(u_val > 127 ? 127 : (u_val < -128 ? -128 : u_val));
            V_buff[j * (x/2) + i] = (signed char)(v_val > 127 ? 127 : (v_val < -128 ? -128 : v_val));
        }
    }
}

/*******************************************************************************************************************************/
/* YUV420 processing */
/*******************************************************************************************************************************/
void RGBtoYUV420(const uchar rgbImg[], int w, int h, uchar Y_buff[], signed char U_buff[], signed char V_buff[])
{
    for(int y = 0; y < h; y += 2)
    {
        for(int x = 0; x < w; x += 2)
        {
            double U_sum = 0, V_sum = 0;

            for(int yb = 0; yb < 2; yb++)
            {
                for(int xb = 0; xb < 2; xb++)
                {
                    int yy = y + yb;
                    int xx = x + xb;
                    int rgb_idx = (yy * w + xx) * 3;

                    uchar R = rgbImg[rgb_idx];
                    uchar G = rgbImg[rgb_idx + 1];
                    uchar B = rgbImg[rgb_idx + 2];

                    Y_buff[yy * w + xx] = (uchar)(0.299 * R + 0.587 * G + 0.114 * B + 0.5);
                    U_sum += (-0.14713 * R - 0.28886 * G + 0.436 * B);
                    V_sum += (0.615 * R - 0.51499 * G - 0.10001 * B);
                }
            }

            int uv_idx = (y / 2) * (w / 2) + (x / 2);
            U_buff[uv_idx] = (signed char)(U_sum / 4.0);
            V_buff[uv_idx] = (signed char)(V_sum / 4.0);
        }
    }
}

void YUV420toRGB(const uchar Y_buff[], const signed char U_buff[], const signed char V_buff[], int w, int h, uchar rgbImg[])
{
    for(int y = 0; y < h; y += 2)
    {
        for(int x = 0; x < w; x += 2)
        {
            int uv_idx = (y / 2) * (w / 2) + (x / 2);
            double U = U_buff[uv_idx];
            double V = V_buff[uv_idx];

            for(int yb = 0; yb < 2; yb++)
            {
                for(int xb = 0; xb < 2; xb++)
                {
                    int yy = y + yb;
                    int xx = x + xb;
                    double Y = Y_buff[yy * w + xx];

                    double R = Y + 1.13983 * V;
                    double G = Y - 0.39465 * U - 0.58060 * V;
                    double B = Y + 2.03211 * U;

                    int rgb_idx = (yy * w + xx) * 3;
                    rgbImg[rgb_idx]     = (uchar)(R > 255 ? 255 : (R < 0 ? 0 : R));
                    rgbImg[rgb_idx + 1] = (uchar)(G > 255 ? 255 : (G < 0 ? 0 : G));
                    rgbImg[rgb_idx + 2] = (uchar)(B > 255 ? 255 : (B < 0 ? 0 : B));
                }
            }
        }
    }
}

void processing_YUV420(uchar Y_buff[], signed char U_buff[], signed char V_buff[], int w, int h, double Y_factor, double U_factor, double V_factor)
{
    for(int i = 0; i < w * h; i++)
    {
        double y_val = Y_buff[i] * Y_factor;
        Y_buff[i] = (uchar)(y_val > 255 ? 255 : (y_val < 0 ? 0 : y_val));
    }
    for(int i = 0; i < (w / 2) * (h / 2); i++)
    {
        double u_val = U_buff[i] * U_factor;
        double v_val = V_buff[i] * V_factor;
        U_buff[i] = (signed char)(u_val > 127 ? 127 : (u_val < -128 ? -128 : u_val));
        V_buff[i] = (signed char)(v_val > 127 ? 127 : (v_val < -128 ? -128 : v_val));
    }
}

/*******************************************************************************************************************************/
/* Y decimation */
/*******************************************************************************************************************************/
void decimate_Y(uchar Y_buff[], int x, int y)
{
    for(int yy = 0; yy < y; yy += 2)
    {
        for(int xx = 0; xx < x; xx += 2)
        {
            uchar Y_val = Y_buff[yy * x + xx];
            Y_buff[(yy + 1) * x + xx]     = Y_val;
            Y_buff[yy * x + (xx + 1)]     = Y_val;
            Y_buff[(yy + 1) * x + (xx + 1)] = Y_val;
        }
    }
}
