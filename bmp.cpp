#include "bmp.h"
bool Bmp::readBmp(char *bmpName)
{

    //二进制读方式打开指定的图像文件
    FILE *fp = fopen(bmpName, "rb");
    if (fp == 0) return 0;


    //跳过位图文件头结构BITMAPFILEHEADER
    fseek(fp, sizeof(BITMAPFILEHEADER), 0);
    //定义位图信息头结构变量，读取位图信息头进内存，存放在变量head中
    BITMAPINFOHEADER head;

    fread(&head, sizeof(BITMAPINFOHEADER), 1, fp);
    //获取图像宽、高、每像素所占位数等信息
    bmpWidth = head.biWidth;
    bmpHeight = head.biHeight;
    biBitCount = head.biBitCount;
    //定义变量，计算图像每行像素所占的字节数（必须是4的倍数）
    //灰度图像有颜色表，且颜色表表项为256
    switch (biBitCount)
    {
    case 1:
    {
		lineByte = ((bmpWidth +7) / 8  + 3) / 4 * 4;

        pColorTable = new RGBQUAD[2];
        fread(pColorTable, sizeof(RGBQUAD), 2, fp);
        break;
    }
    case 4:
    {
		lineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4;

        pColorTable = new RGBQUAD[16];
        fread(pColorTable, sizeof(RGBQUAD), 16, fp);
        break;
    }
    case 8:
    {
		lineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4;
        pColorTable = new RGBQUAD[256];
        fread(pColorTable, sizeof(RGBQUAD), 256, fp);
        break;
    }
    case 24:
    {
		lineByte = (3 * bmpWidth * biBitCount / 24 + 3) / 4 * 4;
        pColorTable = NULL;
        break;
        //fseek(fp, sizeof(RGBQUAD), 1);
    }
    }
    //申请位图数据所需要的空间，读位图数据进内存
    pBmpBuf = new unsigned char[lineByte * bmpHeight];
    fread(pBmpBuf, 1, lineByte * bmpHeight, fp);
    //关闭文件
    fclose(fp);
    return 1;
}
bool Bmp::setPixels()
{
	switch (biBitCount)
	{
	case 1:
	{
		unsigned char * pBmp = new unsigned char[(3 * bmpWidth + 3) / 4 * 4 * bmpHeight];
		for (int i = 0; i < bmpHeight; i++)
		{
			for (int j = 0; j < (bmpWidth + 7) / 8; j++)
			{
				unsigned char temp = pBmpBuf[i*lineByte + j];
				for (int k = 0; (k < 8) &&( (j * 8 + k) <bmpWidth); k++, temp <<= 1)
				{
					pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 24 + k * 3] = pColorTable[temp >> 7].rgbRed;
					pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 24 + k * 3 + 1] = pColorTable[temp >> 7].rgbGreen;
					pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 24 + k * 3 + 2] = pColorTable[temp >> 7].rgbBlue;
				}
			}
		}
		size = (3 * bmpWidth + 3) / 4 * 4 * bmpHeight;
		delete pBmpBuf;
		pBmpBuf = pBmp;
		break;
	}
	case 4:
	{
		unsigned char * pBmp = new unsigned char[(3 * bmpWidth + 3) / 4 * 4 * bmpHeight];
		for (int i = 0; i < bmpHeight; i++)
		{
			for (int j = 0; j < lineByte; j++)
			{
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 6] = pColorTable[pBmpBuf[i*lineByte + j] >> 4].rgbRed;
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 6 + 1] = pColorTable[pBmpBuf[i*lineByte + j] >> 4].rgbGreen;
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 6 + 2] = pColorTable[pBmpBuf[i*lineByte + j] >> 4].rgbBlue;

				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 6 + 3] = pColorTable[pBmpBuf[i*lineByte + j] & 0x0f].rgbRed;
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 6 + 4] = pColorTable[pBmpBuf[i*lineByte + j] & 0x0f].rgbGreen;
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 6 + 5] = pColorTable[pBmpBuf[i*lineByte + j] & 0x0f].rgbBlue;

			}
		}
		delete pBmpBuf;
		pBmpBuf = pBmp;
		break;
	}
	case 8:
	{
		unsigned char * pBmp = new unsigned char[(3 * bmpWidth + 3) / 4 * 4 * bmpHeight];
		for (int i = 0; i < bmpHeight; i++)
		{
			for (int j = 0; j < lineByte; j++)
			{
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 3] = pColorTable[pBmpBuf[i*lineByte + j]].rgbRed;
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 3 + 1] = pColorTable[pBmpBuf[i*lineByte + j]].rgbGreen;
				pBmp[i*((3 * bmpWidth + 3) / 4 * 4) + j * 3 + 2] = pColorTable[pBmpBuf[i*lineByte + j]].rgbBlue;
			}
		}
		delete pBmpBuf;
		pBmpBuf = pBmp;
		break;
	}
	case 24:
	{
		for (int i = 0; i < bmpHeight; i++)
		{
			for (int j = 0; j < lineByte; j+=3)
			{
				BYTE R = pBmpBuf[i*lineByte + j + 2];
				pBmpBuf[i*lineByte + j + 2] = pBmpBuf[i*lineByte + j];
				pBmpBuf[i*lineByte + j] = R;
			}
		}
		size = lineByte * bmpHeight;
		break;
	}
	}
	return 1;
}