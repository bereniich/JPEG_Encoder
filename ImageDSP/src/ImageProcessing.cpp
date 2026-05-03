
#include "ImageProcessing.h"
#include "ColorSpaces.h"
#include "JPEG.h"

#include <cmath>

#include <QDebug>
#include <QString>
#include <QImage>


void imageProcessingFun(const QString& progName, QImage& outImgs, const QImage& inImgs, const QVector<double>& params)
{
    int X_SIZE = inImgs.width();
    int Y_SIZE = inImgs.height();

    // Allocate memory for Luma (Y) and Subsampled Chroma (U, V) components
    uchar* Y_buff = new uchar[X_SIZE * Y_SIZE];
    signed char* U_buff = new signed char[(X_SIZE / 2) * (Y_SIZE / 2)];
    signed char* V_buff = new signed char[(X_SIZE / 2) * (Y_SIZE / 2)];

    // RGB -> YUV420 Conversion
    // Convert to Format_RGB888 to ensure data is aligned (3 bytes per pixel)
    QImage tempIn = inImgs.convertToFormat(QImage::Format_RGB888);
    RGBtoYUV420(tempIn.bits(), X_SIZE, Y_SIZE, Y_buff, U_buff, V_buff);

    // Execute JPEG Encoding
    if(progName == QString("JPEG Encoder"))
    {
        // The encoder reads the YUV buffers and saves "example.jpg" to disk
        performJPEGEncoding(Y_buff, (char*)U_buff, (char*)V_buff, X_SIZE, Y_SIZE, params[0]);

        // CRITICAL PART: Load the file the encoder just wrote to disk
        // This ensures outImgs contains the image data after JPEG compression losses
        outImgs.load("example.jpg");
    }
    else
    {
        // If not JPEG Encoder, perform standard in-memory reconstruction
        outImgs = QImage(X_SIZE, Y_SIZE, QImage::Format_RGB888);
        YUV420toRGB(Y_buff, U_buff, V_buff, X_SIZE, Y_SIZE, outImgs.bits());
    }

    // Clean up
    delete[] Y_buff;
    delete[] U_buff;
    delete[] V_buff;
}
