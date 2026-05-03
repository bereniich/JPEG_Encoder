#ifndef COLORSPACES_H_
#define COLORSPACES_H_

#include <QDebug>

/*******************************************************************************************************************************/
/* RGB processing */
/*******************************************************************************************************************************/
void processing_RGB(const uchar rgbInputImg[], int x, int y, uchar rgbOutputImg[], double R, double G, double B);

/*******************************************************************************************************************************/
/* YUV444 processing */
/*******************************************************************************************************************************/
// Koristimo signed char za U i V da bismo izbegli artefakte sa opsegom
void RGBtoYUV444(const uchar rgbImg[], int x, int y, uchar Y_buff[], signed char U_buff[], signed char V_buff[]);

void YUV444toRGB(const uchar Y_buff[], const signed char U_buff[], const signed char V_buff[], int x, int y, uchar rgbImg[]);

void processing_YUV444(uchar Y_buff[], signed char U_buff[], signed char V_buff[], int x, int y, double Y, double U, double V);

/*******************************************************************************************************************************/
/* YUV422 processing */
/*******************************************************************************************************************************/
void RGBtoYUV422(const uchar rgbImg[], int x, int y, uchar Y_buff[], signed char U_buff[], signed char V_buff[]);

void YUV422toRGB(const uchar Y_buff[], const signed char U_buff[], const signed char V_buff[], int x, int y, uchar rgbImg[]);

void processing_YUV422(uchar Y_buff[], signed char U_buff[], signed char V_buff[], int x, int y, double Y, double U, double V);

/*******************************************************************************************************************************/
/* YUV420 processing */
/*******************************************************************************************************************************/
void RGBtoYUV420(const uchar rgbImg[], int w, int h, uchar Y_buff[], signed char U_buff[], signed char V_buff[]);

void YUV420toRGB(const uchar Y_buff[], const signed char U_buff[], const signed char V_buff[], int w, int h, uchar rgbImg[]);

void processing_YUV420(uchar Y_buff[], signed char U_buff[], signed char V_buff[], int w, int h, double Y, double U, double V);

/*******************************************************************************************************************************/
/* Y decimation */
/*******************************************************************************************************************************/
void decimate_Y(uchar Y_buff[], int x, int y);

#endif // COLORSPACES_H_
