#include "ImageProcessing.h"
#include "ColorSpaces.h"
#include "JPEG.h"

#include <cmath>
#include <QDebug>
#include <QString>
#include <QImage>

void imageProcessingFun(const QString& progName, QImage& outImgs, const QImage& inImgs, const QVector<double>& params)
{
	// TO DO

	/* Create buffers for YUV image */
    int X_SIZE = inImgs.width();
    int Y_SIZE = inImgs.height();

    int paddedWidth  = (X_SIZE % 16 == 0) ? X_SIZE : X_SIZE + (16 - X_SIZE % 16);
    int paddedHeight = (Y_SIZE % 16 == 0) ? Y_SIZE : Y_SIZE + (16 - Y_SIZE % 16);

    uchar* Y_buff = new uchar[paddedWidth * paddedHeight];
    char* U_buff  = new char[(paddedWidth / 2) * (paddedHeight / 2)];
    char* V_buff  = new char[(paddedWidth / 2) * (paddedHeight / 2)];

	/* Create empty output image */
    outImgs = QImage(X_SIZE, Y_SIZE, inImgs.format());

	/* Convert input image to YUV420 image */
    RGBtoYUV420(inImgs.bits(),X_SIZE, Y_SIZE, Y_buff, U_buff, V_buff);


    if(progName == QString("JPEG Encoder"))
	{	
		/* Perform NxN DCT */
        performJPEGEncoding(Y_buff, U_buff, V_buff, X_SIZE, Y_SIZE, params[0]);
	}

    /* Convert YUV image back to RGB */
    YUV420toRGB(Y_buff, U_buff, V_buff, inImgs.width(), inImgs.height(), outImgs.bits());

    //outImgs = QImage("example.jpg");
    /*outImgs.save("output_quality_1.jpg", "JPG", 1);
    outImgs.save("output_quality_10.jpg", "JPG", 10);
    outImgs.save("output_quality_20.jpg", "JPG", 20);
    outImgs.save("output_quality_30.jpg", "JPG", 30);
    outImgs.save("output_quality_40.jpg", "JPG", 40);
    outImgs.save("output_quality_50.jpg", "JPG", 50);
    outImgs.save("output_quality_60.jpg", "JPG", 60);
    outImgs.save("output_quality_70.jpg", "JPG", 70);
    outImgs.save("output_quality_80.jpg", "JPG", 80);
    outImgs.save("output_quality_90.jpg", "JPG", 90);
    outImgs.save("output_quality_100.jpg", "JPG", 100);
    */

	/* Delete used memory buffers */
    delete[] Y_buff;
    delete[] U_buff;
    delete[] V_buff;

}

