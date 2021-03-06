#include "jpeg.h"
#include <QMessageBox>
Jpeg::Jpeg(char* filename){

	

	FILE *fp1;
	if ((fp1 = fopen(filename, "rb")) == NULL)
		error("Couldn't open the Jpg file! Please check if the file path contain non-ASCII character.");
	initialize(fp1);
	makeRGB888();
	decode(fp1);
	fclose(fp1);
}
void Jpeg::error(std::string s)
{
	printf("%s\n", s);
	QMessageBox::critical(0, QObject::tr("error"), QString::fromStdString(s));
	exit(0);
}

void Jpeg::makeRGB888()
{
	///BMP位图文件头，总共14个字节
	unsigned long i, j;
	linebytes = widthbytes(24*imgwidth);//每一行的字节数必须是4的整倍数,如果不是，则需要补齐
	//imgwidth是JPEG的行像素
	buffer = new unsigned char[imgheight*linebytes];
	for (i = 0; i < imgheight; i++)
		for (j = 0; j < linebytes; j++)
			buffer[i*linebytes + j] = 0;
			//putchar(0, fp);  //int fputc( int c, FILE *stream ) 虽然是int c但每次只写一个字节到文件fp
	//fp--Pointer to FILE structure    	     
}


void Jpeg::initialize(FILE *fp)   //initialize the JPG format file!
{
	unsigned char *p, *q, hfindex, qtindex, number;//number扫描数据段中图像分量的个数Ns
	int i, j, k, finish = 0, huftab1, huftab2, huftabindex, count;
	unsigned short int length = 0, flag = 0;//Note:"unsigned int", "int" are both 4 bytes(32 bits)!!!!
	//only "short int" is 2 bytes, and "char"is 1 bytes.

	fread(&flag, sizeof(short int), 1, fp);//Here the result of flag is d8ff but I don't why!
	if (flag != 0xd8ff)//判断文件头是不是FFD8。注意在内存中FF是低位，D8是高位	
		error("Error Jpg File format! _____ File Head Error");//error() will output the error information and then terminate running.

	while (!finish)//循环读取每一个标志段
	{
		fread(&flag, sizeof(short int), 1, fp);//每一次读完后，fp指针会自动向后移动
		if (flag != 0xd8ff)
		{
			fread(&length, sizeof(short int), 1, fp);
			length = ((length << 8) | (length >> 8));//注意实际读出的数字高低位与显示的二进制文件高低位是相互颠倒的
		}

		//所以必须交换高地位，length为标志段的长度
		switch (flag)
		{
			//////////////////////////////////////////宋师姐的相机专用
		case 0xe1ff://///////APP0段
			fseek(fp, length - 4, 1); break;
		case 0xd8ff:
			break;
			////////////////////////////////////
		case 0xe0ff://///////APP0段
			fseek(fp, length - 2, 1); break;//将指针移动至APP0段末尾
			//int fseek( FILE *stream, long offset, int origin )
			//SEEK_SET=0 表示文件的开头指针位置,宏SEEK_CUR=1表示当前指针位置 




		case 0xdbff://离散余弦DQT量化表
			p = (unsigned char *)malloc(length - 2);//Allocate the memory space of "length" size.
			//返回一个指向该内存的指针p 
			if (p == NULL)
				printf("Insufficient memory available! The quantization-table may be too large!\n");
			//Always check the return from malloc, even if the  requested memory  is small.

			fread(p, length - 2, 1, fp);//读出量化表的所有数存到p中，注意p首地址为长度字后的第一个字节PqTq
			qtindex = (*p) & 0x0f;//0x0f=00001111与掉了(*p)前面的半个字节（Pq量化表精度0表示8bit，1表示16bit）
			//后面的半个字节（Tq表编号）。qtindex代表第几个量化表

			q = p + 1;//q指向量化表数据开始的第一个字节
			if (length<68)//只有一个量化表
			{
				for (i = 0; i<64; i++)
					Qt[qtindex][i] = (int)*(q++);//读一个字节时不存在高低为颠倒的问题
			}
			else
			{
				for (i = 0; i<64; i++)
					Qt[qtindex][i] = (int)*(q++);
				qtindex = *(q++) & 0x0f;//去掉第二表的PqTq字节的前面的半个字节（Pq量化表精度），Tq为表的编号
				for (i = 0; i<64; i++)
					Qt[qtindex][i] = (int)*(q++);
			}
			free(p); break;


		case 0xc0ff://SOF帧信息段，C0表示Baseline编码方式
			p = (unsigned char *)malloc(length - 2);//Allocate the memory space of "length" size.
			if (p == NULL)
				printf("Insufficient memory available! The picture may be too large!\n");
			//Always check the return from malloc, even if the  requested memory  is small.

			fread(p, length - 2, 1, fp);
			imgheight = ((*(p + 1)) << 8) + (*(p + 2));//图像的行像素。注意在既有char又有其他高级别类型的式子里，char会自动转换为其他类型
			imgwidth = ((*(p + 3)) << 8) + (*(p + 4));//图像的列像素  注意实际图像标明的像素是按XY坐标系划分的，与矩阵的行列相反
			compressnum = *(p + 5);//图像分量的个数
			if ((compressnum != 1) && (compressnum != 3))
				error("Error Jpg File Format!   compressnum error");
			if (compressnum == 3)
			{
				compressindex[0] = *(p + 6);//Y分量
				sampleYH = (*(p + 7)) >> 4;//水平采样因子。YH*YV是指一个MCU中存贮多少个Y分量8*8的块，然后接着再存U分量
				sampleYV = (*(p + 7)) & 0x0f;//垂直采样因子
				YQt = (int*)Qt[*(p + 8)];//使用的量化表编号   C1,H1V1,Tq1

				compressindex[1] = *(p + 9);//U分量
				sampleUH = (*(p + 10)) >> 4;
				sampleUV = (*(p + 10)) & 0x0f;
				UQt = (int*)Qt[*(p + 11)];//C2,H2V2,Tq2

				compressindex[2] = *(p + 12);//V分量
				sampleVH = (*(p + 13)) >> 4;
				sampleVV = (*(p + 13)) & 0x0f;
				VQt = (int*)Qt[*(p + 14)];//C3,H3V3,Tq3
			}
			else//只有一个分量灰度图，默认用同一个表。采样因子为1*1？？？？？
			{
				compressindex[0] = *(p + 6);
				sampleYH = (*(p + 7)) >> 4;
				sampleYV = (*(p + 7)) & 0x0f;
				YQt = (int*)Qt[*(p + 8)];

				compressindex[1] = *(p + 6);
				sampleUH = 1;
				sampleUV = 1;
				VQt = (int*)Qt[*(p + 8)];

				compressindex[2] = *(p + 6);
				sampleVH = 1;
				sampleVV = 1;
				VQt = (int*)Qt[*(p + 8)];
			}
			free(p); break;


		case 0xc4ff:
			p = (unsigned char *)malloc(length - 2);//人为在最后添加一个字节0xff，用以判断循环何时结束		    
			if (p == NULL)
				printf("Insufficient memory available! The picture may be too large!\n");
			//Always check the return from malloc, even if the  requested memory  is small.

			fread(p, length - 2, 1, fp);
			//	p[length-1]=0xff;//人为在最后添加一个字节0xff，用以判断循环何时结束
			//	if (length<0xd0)//？？？？如何判断只有一个表有待改进！0xd0=208 一个huffman表最多能有多少个字节
			//	{
			huftab1 = (int)(*p) >> 4;
			huftab2 = (int)(*p) & 0x0f;
			huftabindex = huftab1 * 2 + huftab2;//直接将Tc|Th看成表的编号：huftabindex=（2*Tc+Th）
			q = p + 1; count = 0;//count计算第一个huf表码的字节数
			for (i = 0; i<16; i++)
			{

				codelen[huftabindex][i] = (int)(*(q++));//codelen表示每个长度i+1的码对应的个数
				count += codelen[huftabindex][i];//codevalue部分的字节数
			}
			j = 0; count += 17;//此时count表示第一个表的字节数
			for (i = 0; i<16; i++)
				if (codelen[huftabindex][i] != 0)	//如果某个长度(i+1)的码的个数不为零				
				{
				k = 0;
				while (k<codelen[huftabindex][i])
				{
					codevalue[huftabindex][k + j] = (int)(*(q++));//把后面value（即行数）段对应的值都取出来
					k++;//codevalue[4][j]=(Z,R)连零数加行号
				}
				j += k;
				}

			i = 0;
			while (codelen[huftabindex][i] == 0) i++;//从第一个码的个数不为零的长度开始
			for (j = 0; j<i; j++)
			{
				hufmin[huftabindex][j] = 0;//j长度的码的开始码（码是用int值表示的）
				hufmax[huftabindex][j] = 0;//j长度的码的结束码（码是用int值表示的）
			}
			hufmin[huftabindex][i] = 0;//第一个码的起始值都规定为0（int型的值）
			hufmax[huftabindex][i] = codelen[huftabindex][i] - 1;
			for (j = i + 1; j<16; j++)
			{
				if (codelen[huftabindex][j - 1] != 0)
				{
					hufmin[huftabindex][j] = (hufmax[huftabindex][j - 1] + 1) << 1;//左移一位，j+1长度的码的开始码                       				        
					if (codelen[huftabindex][j] != 0)
						hufmax[huftabindex][j] = hufmin[huftabindex][j] + codelen[huftabindex][j] - 1; //j+1长度的码的结束码
					else//如果j+1长度的码个数为零则令其最大值与最小值相同，虽然生成了码但不会用到，只是为了衔接后面码的生成
						hufmax[huftabindex][j] = hufmin[huftabindex][j];
				}


				else //如果前面码个数为零，则后面码生成时只移位不加1
				{
					hufmin[huftabindex][j] = (hufmax[huftabindex][j - 1]) << 1;//只移位不加1，j+1长度的码的开始码
					if (codelen[huftabindex][j] != 0)
						hufmax[huftabindex][j] = hufmin[huftabindex][j] + codelen[huftabindex][j] - 1; //j+1长度的码的结束码
					else//如果j+1长度的码个数为零则令其最大值与最小值相同，虽然生成了码但不会用到，只是为了衔接后面码的生成
						hufmax[huftabindex][j] = hufmin[huftabindex][j];
				}

			}
			codepos[huftabindex][0] = 0;//长度为j+1的第一个码所对应的codevalue[]的索引
			for (j = 1; j<16; j++)
				codepos[huftabindex][j] = codelen[huftabindex][j - 1] + codepos[huftabindex][j - 1];
			//}

			tempp = count;
			//	else//FFC4段有多个huf表
			//	if(count<length-2)//FFC4段有多个huf表
			while (tempp<length - 2)
			{

				// unsigned char *tempp;
				//tempp=p;
				//	tempp+=count;//将指针移到第二个表的开始处
				p += count;
				hfindex = *p;
				//	while (hfindex!=0xff)//人为在最后添加一个字节0xff，用以判断表何时结束

				huftab1 = (int)hfindex >> 4;
				huftab2 = (int)hfindex & 0x0f;
				huftabindex = huftab1 * 2 + huftab2;
				q = p + 1; count = 0;//count表示第一个huf表码的字节数，各部分累加得到
				for (i = 0; i<16; i++)
				{
					codelen[huftabindex][i] = (int)(*(q++));
					count += codelen[huftabindex][i];//codevalue部分的字节数
				}
				count += 17; j = 0;
				for (i = 0; i<16; i++)	//下面同上读出第二个huf码表							
					if (codelen[huftabindex][i] != 0)
					{
					k = 0;
					while (k<codelen[huftabindex][i])
					{
						codevalue[huftabindex][k + j] = (int)(*(q++));
						k++;
					}
					j += k;
					}
				i = 0;
				while (codelen[huftabindex][i] == 0) i++;
				for (j = 0; j<i; j++)
				{
					hufmin[huftabindex][j] = 0;
					hufmax[huftabindex][j] = 0;
				}
				hufmin[huftabindex][i] = 0;
				hufmax[huftabindex][i] = codelen[huftabindex][i] - 1;
				for (j = i + 1; j<16; j++)
				{
					if (codelen[huftabindex][j - 1] != 0)
					{
						hufmin[huftabindex][j] = (hufmax[huftabindex][j - 1] + 1) << 1;//左移一位，j+1长度的码的开始码                       				        
						if (codelen[huftabindex][j] != 0)
							hufmax[huftabindex][j] = hufmin[huftabindex][j] + codelen[huftabindex][j] - 1; //j+1长度的码的结束码
						else//如果j+1长度的码个数为零则令其最大值与最小值相同，虽然生成了码但不会用到，只是为了衔接后面码的生成
							hufmax[huftabindex][j] = hufmin[huftabindex][j];
					}

					else //如果前面码个数为零，则后面码生成时只移位不加1
					{
						hufmin[huftabindex][j] = (hufmax[huftabindex][j - 1]) << 1;//只移位不加1，j+1长度的码的开始码
						if (codelen[huftabindex][j] != 0)
							hufmax[huftabindex][j] = hufmin[huftabindex][j] + codelen[huftabindex][j] - 1; //j+1长度的码的结束码
						else//如果j+1长度的码个数为零则令其最大值与最小值相同，虽然生成了码但不会用到，只是为了衔接后面码的生成
							hufmax[huftabindex][j] = hufmin[huftabindex][j];
					}
				}
				codepos[huftabindex][0] = 0;
				for (j = 1; j<16; j++)
					codepos[huftabindex][j] = codelen[huftabindex][j - 1] + codepos[huftabindex][j - 1];
				//	p+=count;
				//	hfindex=*p;//取第三个表的Tc|Th，循环上面的过程 	
				tempp += count;
			}
			p -= (tempp - count);
			free(p);//循环结束，指针归位起始点，释放整个buffer
			break;

		case 0xddff://DRI段，用以设置间隔重置大小Ri，即每个scan里有Ri个MCU
			p = (unsigned char *)malloc(length - 2);
			if (p == NULL)
				printf("Insufficient memory available! The picture may be too large!\n");
			//Always check the return from malloc, even if the  requested memory  is small.
			fread(p, length - 2, 1, fp);
			restart = ((*p) << 8) | (*(p + 1));//restart=Ri（DRI段中的最后两个字节Ri）两个字节表示一个SCAN中每个
			//MCU-Segment里面包含的MCU的个数，两个segment之间是RST标志字，RST=0（从0—7循环）
			//interval表示每个segment中循环到了第几个MCU（interval=0-Ri）
			free(p); break;

		case 0xdaff://SOS段（start of scan）后面紧跟图像数据
			p = (unsigned char *)malloc(length*sizeof(unsigned char) - 2);
			fread(p, length - 2, 1, fp);
			number = *p;//扫描数据段中图像分量的个数Ns
			if (number != compressnum)
				error("Error Jpg File Format! 03");
			q = p + 1;
			for (i = 0; i<compressnum; i++)//表示图像分量的个数
			{
				if (*q == compressindex[0])//亮度图分量，compressindex表示第几个图像的标号
				{
					YDCindex = (*(q + 1)) >> 4;//hufman_DC表的编号，即为0或1
					YACindex = (*(q + 1) & 0x0f) + 2;//hufman_AC表的编号,即为2或3，是按0123来编号的
				}
				else//两个色度图分量用的表是一样的
				{
					UVDCindex = (*(q + 1)) >> 4;
					UVACindex = (*(q + 1) & 0x0f) + 2;
				}
				q += 2;
			}//SOS段后面还有Ss,Se,Ah|Al,根Baseline方式无关，此处没有处理
			finish = 1; free(p); break;

		case 0xd9ff://文件结束段标志
			error("Error Jpg File Format! SOS is not found");
			break;

		default://其他以(FFD?即flag=D?FF)为标志的段此处都没有考虑
			if ((flag & 0xf000) != 0xd000)
				fseek(fp, length - 2, SEEK_CUR);//直接跳过
			break;
		}
	}
}


void Jpeg::savebmp()//此处只存贮一个MCU的图像，在主函数里还有循环
{
	int i, j;
	unsigned char r, g, b;
	long y, u, v, rr, gg, bb;
	for (i = 0; i<sampleYV * 8; i++)//i表示高
	{
		if ((height + i)<imgheight)
		{//height是BMP的图像的高，imgheight是JPEG的高（像素点）
			int seek = (unsigned long)(imgheight - height - i - 1)*linebytes + 3 * width ;
			//fseek(fp, (unsigned long)(imgheight - height - i - 1)*linebytes + 3 * width + 54, 0);//开始height=0，width=0移到了文件的最后一行开始处
			//3*width每个像素是用3个字节来存储的，Width表示每一行的像素，存第一个MCU时Width始终为0，第二个时为1，挨着又从最后一行开始存
			//最后参数0表示指针从文件的起始位置开始读
			//一般来说，.bMP文件的数据从下到上，从左到右的。也就是说，从文件中最先读到的是图象最下面一行的
			//左边第一个象素，然后是左边第二个象素……接下来是倒数第二行左边第一个象素，左边第二个象素……
			//依次类推 ，最后得到的是最上面一行的最右一个象素。
			for (j = 0; j<sampleYH * 8; j++)//j表示宽
			{
				if ((width + j)<imgwidth)
				{
					y = Y[i * 8 * sampleYH + j];//把一个MCU里的第i行的每个点读出来，i代表MCU行，j代表MCU列
					u = U[(i / VYtoU) * 8 * sampleYH + j / HYtoU];//没有采样的行/列用的是与上一行/列相同的值，不是0
					v = V[(i / VYtoV) * 8 * sampleYH + j / HYtoV];

					rr = ((y << 8) + 359 * v) >> 8;//将YUV转化为RGB。此处涉及到小数化成二进制数的问题，有待研究
					gg = ((y << 8) - 88 * u - 183 * v) >> 8;
					bb = ((y << 8) + 301 * u) >> 8;



					//if (rr&0xffffff00)//校正rr大于255和为负数的情况
					if (rr>255)r = 255; else if (rr<0)r = 0; else  r = (unsigned char)rr;
					// if (gg&0xffffff00)
					if (gg>255)g = 255; else if (gg<0)g = 0; else g = (unsigned char)gg;
					// if (bb&0xffffff00)
					if (bb>255)b = 255; else if (bb<0)b = 0; else	 b = (unsigned char)bb;

					//fputc(b, fp); fputc(g, fp); fputc(r, fp);//分别把bgr分配给三个字节的高、中、低byte	
					buffer[seek + j * 3] = r; 
					buffer[seek + j * 3 + 1] = g;
					buffer[seek + j * 3 + 2] = b;//整理成qt的rgb888格式
				}
				else break;
			}
		}
		else break;
	}
}


unsigned char Jpeg::readbyte(FILE *fp)
{
	unsigned char c;
	c = fgetc(fp);

	if (c == 0xff)
		fgetc(fp);//若是帧头FFXX等，则跳过到XX后面去，但C返回的任然是FF

	bitpos = 8; curbyte = c;
	return c;
}

int Jpeg::DecodeElement(FILE *fp)//fp已经指向了需要解码的数据
{///////////////////////////////解一个码出来
	unsigned int codelength;//huf码的长度，codelength-1为该长度的码在数组中的索引值
	unsigned int thiscode, tempcode;
	unsigned short int temp, neww;
	unsigned char hufbyte, runsize, tempsize, sign;
	unsigned char newbyte, lastbyte;
	if (bitpos >= 1)//所解的码不是block的第一个码？？
	{
		bitpos--;
		thiscode = (unsigned char)curbyte >> bitpos;
		curbyte = curbyte&And[bitpos];
	}
	else//所解的码为一个block的第一个码。bitpos和curbyte初始值为0
	{
		lastbyte = readbyte(fp);
		bitpos--;
		newbyte = curbyte&And[bitpos];//表示取后多少位1（一位），3（两位），7（三位），15（四位）
		thiscode = lastbyte >> 7;//thiscode取了所读字节的第1位
		curbyte = newbyte;//保留后7位
	}

	codelength = 1;
	while ((thiscode<hufmin[HufTabindex][codelength - 1]) || (codelen[HufTabindex][codelength - 1] == 0) || (thiscode>hufmax[HufTabindex][codelength - 1]))
	{//thiscode在huf表中没有这个码则往下执行
		if (bitpos >= 1)
		{
			bitpos--;
			tempcode = (unsigned char)curbyte >> bitpos;//取第2位
			curbyte = curbyte&And[bitpos];//保留后6位
		}
		else
		{
			lastbyte = readbyte(fp);
			bitpos--;
			newbyte = curbyte&And[bitpos];//newbyte取了所读字节的后七位
			tempcode = (unsigned char)lastbyte >> 7;//tempcode取了所读字节的前一位
			curbyte = newbyte;
		}
		thiscode = (thiscode << 1) + tempcode;//thiscode=把取的第一位和取的第二位结合在一起
		codelength++;
		if (codelength>16)
			error("Error Jpg File Format!   04");
	}

	temp = thiscode - hufmin[HufTabindex][codelength - 1] + codepos[HufTabindex][codelength - 1];
	//temp表示当前解出的码thiscode所在的行对应的数组codevalue[4][256]索引号，数组号与行号不一定对应
	//codepos[HufTabindex][codelength-1]表示长度为codelength的第一个码所在的行(即对应的codevalue的值，行值从0开始)
	//codelen表示每个长度的码对应的个数。
	hufbyte = (unsigned char)codevalue[HufTabindex][temp];//hufbyte表示所在行，数组号=(Z,R)连零数加行号
	run = (int)(hufbyte >> 4);//取行号的高4位,表示连零的个数Z。(Z,R)|C
	runsize = hufbyte & 0x0f;//取行号的低4位，可能就是表示行号
	if (runsize == 0)//0行对应的value只能是0
	{
		value = 0;//value表示这个码解出来的值
		return 1;
	}
	tempsize = runsize;//runsize取行号的低4位，表示行号1—16
	if (bitpos >= runsize)//所读出的一个字节还剩bitpos位，判断够不够往后再移runsize位
	{
		bitpos -= runsize;//bitpos往后再移runsize位（等于行数），因为(R,Z)|C中列号C的位数与行数R是相同的
		neww = (unsigned char)curbyte >> bitpos;//neww即为列号C所代表的码
		curbyte = curbyte&And[bitpos];//保留剩下的码位
	}
	else
	{
		neww = (unsigned short int)curbyte;
		tempsize -= bitpos;
		while (tempsize>8)
		{
			lastbyte = readbyte(fp);//再读一个字节出来
			neww = (neww << 8) + (unsigned char)lastbyte;
			tempsize -= 8;
		}
		lastbyte = readbyte(fp);
		bitpos -= tempsize;
		neww = (neww << tempsize) + (lastbyte >> bitpos);
		curbyte = lastbyte&And[bitpos];
	}
	sign = neww >> (runsize - 1);//取neww的第一位。runsize表示行号的低4位,可能就是表示行号
	if (sign)//即如果为正数则直接赋值，sign判断正负，因为正数的列号第一位肯定是1，可以验证
		value = neww;//value表示每个码解出来的值
	else
	{
		neww = neww ^ 0xffff;//异或，等于取反。因为同一行中正负数对应的列号恰好等于互相取反
		temp = 0xffff << runsize;
		value = -(int)(neww^temp);//把高位取反生成的1变为0。value表示每个码解出来的值
	}
	return 1;
}


int Jpeg::HufBlock(FILE *fp, unsigned char dchufindex, unsigned char achufindex)
{////根据不同的DC和AC编码表把fp当前所指的block块解码到
	//blockbuffer[64]中存起来，fp也相应地向后移动一个块指向下一个块
	int i, count = 0;
	HufTabindex = dchufindex;//直流表的索引值赋给HufTabindex
	if (DecodeElement(fp) != 1)//用直流表解一个直流码出来
		return 0;
	blockbuffer[count++] = value;//blockbuffer[64]存贮一个块,把value赋给blockbuffer[0]
	//value表示每个码解出来的值，如果是第一个码则表示直流分量差分后的值，
	//在DecodeMCUBlock（）还要相加才能得到真正的直流系数
	HufTabindex = achufindex;//交流表的索引值赋给HufTabindex
	while (count<64)//解该Block中第二个至最后一个值
	{              //循环把一个block中的63个交流数据解出来
		if (DecodeElement(fp) != 1)
			return 0;
		if ((run == 0) && (value == 0))//交流系数都为零。run表示连零的个数
		{
			for (i = count; i<64; i++)
				blockbuffer[i] = 0;
			count = 64;
		}
		else
		{
			for (i = 0; i<run; i++)//表示每个交流系数之前连0的个数
				blockbuffer[count++] = 0;
			blockbuffer[count++] = value;
		}
	}
	return 1;
}

int Jpeg::DecodeMCUBlock(FILE *fp)//fp1已经移到了MCU块处，把一个MCU的数据码全部解码出来
{
	int i, j, *pMCUBuffer;
	if (intervalflag)//表示scan中一个segment结束，另一个segment开始前的一些初始化初始化
	{
		fseek(fp, 2, 1);//宏SEEK_CUR=1表示当前指针位置，表示跳过第二个segment开始前的RST（0-7循环）标识两个字节
		//int fseek( FILE *stream, long offset, int origin );
		//If successful, fseek returns 0. Otherwise, it returns a nonzero value
		ycoef = ucoef = vcoef = 0;//第一个没有差分的直流分量的系数，在一个segment里面直流进行差分，第二segment里面又重新进行差分？？？？？？
		//?????????????????????????????????????????????????????????????????可以通过试验来确认，将上面一行注释掉	
		bitpos = 0; curbyte = 0;
	}

	switch (compressnum)
	{
	case 3://彩色图像
		pMCUBuffer = MCUbuffer;//MCUbuffer[10*64]一个MCU中最多只能有10个8*8块，注意只是个一维数组
		for (i = 0; i<sampleYH*sampleYV; i++)//对一个MCU中的Y分量的sampleYH*sampleYV个块解码
		{
			if (HufBlock(fp, YDCindex, YACindex) != 1)//根据不同的DC和AC编码表把fp当前所指的block块解码到
				//blockbuffer[64]中存起来，fp也相应地向后移动一个块指向下一个块
				return 0;
			blockbuffer[0] = blockbuffer[0] + ycoef;//blockbuffer[0]初始值是HufBlock（）里的value
			ycoef = blockbuffer[0];//ycoef是每个块的第一个数，表示每个块的直流分量，直流分量
			//是差分编码，所以要相加才能得到后一个直流分量
			for (j = 0; j<64; j++)
				*pMCUBuffer++ = blockbuffer[j];//将MCU中的第j个block赋给指针，注意pMCUBuffer[64*10]只是个一维数组				
			//一个MCU中的所有元素都顺序存贮在一维数组pMCUBuffer中
		}
		for (i = 0; i<sampleUH*sampleUV; i++)
		{
			if (HufBlock(fp, UVDCindex, UVACindex) != 1)//U和V分量都用一样的交流和直流表
				return 0;
			blockbuffer[0] = blockbuffer[0] + ucoef;
			ucoef = blockbuffer[0];
			for (j = 0; j<64; j++)
				*pMCUBuffer++ = blockbuffer[j];
		}

		for (i = 0; i<sampleVH*sampleVV; i++)
		{
			if (HufBlock(fp, UVDCindex, UVACindex) != 1)
				return 0;
			blockbuffer[0] = blockbuffer[0] + vcoef;
			vcoef = blockbuffer[0];
			for (j = 0; j<64; j++)
				*pMCUBuffer++ = blockbuffer[j];
		}
		break;

	case 1://黑白图像
		pMCUBuffer = MCUbuffer;//不涉及到采样因子，一个Y分量块就相当于一个MCU
		if (HufBlock(fp, YDCindex, YACindex) != 1)
			return 0;
		blockbuffer[0] = blockbuffer[0] + ycoef;
		ycoef = blockbuffer[0];
		for (j = 0; j<64; j++)
			*pMCUBuffer++ = blockbuffer[j];
		for (i = 0; i<128; i++)
			*pMCUBuffer++ = 0;//也是按Y，U，V三分量存储，只不过后面两个分量都存的是0			
		break;

	default:
		error("Error Jpg File Format!   05");
	}
	return 1;
}

void Jpeg::IQtZBlock(int *s, long*d, int *pQt, int correct)
{
	int i, j, tag;
	long *pbuffer, buffer[8][8];
	for (i = 0; i<8; i++)
		for (j = 0; j<8; j++)
		{
		tag = Z[i][j];         //Z[i][j]存储的是zig-zag排列的顺序索引
		buffer[i][j] = (long)s[tag] * (long)pQt[tag];//注意buffer里面的顺序已经变换成原块的每行每列的正常存贮了
		}
	pbuffer = (long *)buffer;
	MathUtil::IDCTint(pbuffer);//反离散余弦变换
	for (i = 0; i<8; i++)
		for (j = 0; j<8; j++)
			d[i * 8 + j] = (buffer[i][j] >> 3) + correct;//？？？？？？？？？？？?????>>3
}

void Jpeg::IQtZMCU(int xx, int yy, int offset, int *pQt, int correct)
{
	int i, j, *pMCUBuffer;//反量化之前的buffer
	long *pQtZMCUBuffer;//反量化之后的buffer
	pMCUBuffer = MCUbuffer + offset;//offset表示一个MCU中Y（U，V）分量的起始位置
	pQtZMCUBuffer = QtZMCUbuffer + offset;
	for (i = 0; i<yy; i++)//垂直采样率
		for (j = 0; j<xx; j++)//水平采样率
			IQtZBlock(pMCUBuffer + (i*xx + j) * 64, pQtZMCUBuffer + (i*xx + j) * 64, pQt, correct);
}

void Jpeg::getYUV(int xx, int yy, long *buf, int offset)
{//	getYUV(sampleYH,sampleYV,Y,Yinbuf)
	//Yinbuf表示在MCU中Y分量的起始位置
	int i, j, k, n;
	long *pQtZMCU;
	pQtZMCU = QtZMCUbuffer + offset;
	for (i = 0; i<yy; i++)//垂直块数sampleYV
		for (j = 0; j<xx; j++)//水平块数sampleYH。
			for (k = 0; k<8; k++)//行
				for (n = 0; n<8; n++)//列
					buf[(i * 8 + k)*sampleYH * 8 + j * 8 + n] = *pQtZMCU++;//（绝对正确）将MCU中的不同分量读到不同的buf里面去
	//注意pQtZMCU里面的顺序已经在IQtZBlock（）中变换成原块的每行每列的正常存贮了
}

void Jpeg::decode(FILE *fp1)
{
	int Yinbuf, Uinbuf, Vinbuf;
	YinMCU = sampleYH*sampleYV;//sampleXX 是block数目
	UinMCU = sampleUH*sampleUV;
	VinMCU = sampleVH*sampleVV;

	HYtoU = sampleYH / sampleUH;//HYtoU是在一个MCU中垂直采样方向YBlock数目与UBlock数目的比值
	VYtoU = sampleYV / sampleUV;
	HYtoV = sampleYH / sampleVH;
	VYtoV = sampleYV / sampleVV;

	Yinbuf = 0;//在MCU中Y分量的起始位置
	Uinbuf = YinMCU * 64;//在MCU中U分量的起始位置
	Vinbuf = (YinMCU + UinMCU) * 64;//在MCU中V分量的起始位置

	while (DecodeMCUBlock(fp1))//fp1已经移到了MCU块处，循环解码每个MCU，
	{                         //当只有Y分量时（黑白图像），一个block块就相当与一个MCU
		interval++;
		if ((restart) && (interval%restart == 0))//restart=Ri（DRI段中的最后两个字节）两个字节表示一个SCAN中每个
			//segment里面包含的MCU的个数，MCU是从1开始（MCU1）两个segment之间是RST标志字，该标志字从0—7循环
			//restart=0表示不使用间隔重置方式存贮数据，同时segment之间也没有RST标志字，即SCAN中所有的MCU连续存贮
			//interval表示每个segment中循环到了第几个MCU（interval=1-Ri）
			intervalflag = 1;//intervalflag=1表示scan中一个segment结束，另一个segment开始的标识，等于0表示还没结束
		else
			intervalflag = 0;

		IQtZMCU(sampleYH, sampleYV, Yinbuf, YQt, 128);//反量化和反离散余弦变换
		IQtZMCU(sampleUH, sampleUV, Uinbuf, UQt, 0);//离散余弦变换只接受-128—127的值，变换之前都减去了128
		IQtZMCU(sampleVH, sampleVV, Vinbuf, VQt, 0);//为什么只有Y分量要加128呢？？？？？？等待验证

		getYUV(sampleYH, sampleYV, Y, Yinbuf);//反量化获得YUV分量
		getYUV(sampleUH, sampleUV, U, Uinbuf);
		getYUV(sampleVH, sampleVV, V, Vinbuf);
		savebmp();//此处只存贮一个MCU的图像
		width += sampleYH * 8;
		if (width >= imgwidth)
		{
			width = 0;
			height += sampleYV * 8;
		}
		if ((width == 0) && (height >= imgheight))
			break;
	}
}

