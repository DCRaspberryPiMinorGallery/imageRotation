#include "main.h"

#define	KB_SIZE	1024
#define	MB_SIZE	(KB_SIZE*1024)
#define BMPHEADER	54
#define LIMIT_SAVE_BITMAP_DATA_X_VALUE	640	//x pixel value
#define LIMIT_SAVE_BITMAP_DATA_Y_VALUE	480	//y pixel value
#define LIMIT_SAVE_BITMAP_DATA_C_VALUE	3

#define pi 3.141592653589793238462643383279

struct BMP_HEADER_STRUCT{
	char sMagicNumber[2];						
	unsigned int iBitmapFileSize;				
	unsigned int iButmapRealDataOffsetValue;	
	unsigned int iBackendHeaderSize;			
	unsigned int iBitmapXvalue;					
	unsigned int iBitmapYvalue;					
	unsigned short iColorPlane;					
	unsigned short iPixelBitDeepValue;			
	unsigned int iCompressType;					
	unsigned int iImageSize;					
	unsigned int iPixcelPerMeterXvalue;			
	unsigned int iPixcelPerMeterYvalue;			
	unsigned int iColorPalet;					
	unsigned int iCriticalColorCount;			
};

char sMainFrameBuffer[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE];
char sReadBitmapFileData[16 * MB_SIZE];
char sImageBuffer[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE];
char sImageRotationBuffer[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE];
char sBitmapHeaderBuffer[BMPHEADER];
int iTargetImageXsize, iTargetImageYsize;
unsigned int iGlobalTimer = 0;

int fDrawingFrameBufferLoopFunction(void);
int fRotation
(
	char sSourceData[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE],
	char sTargetData[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE],
	int Width, int Height, int iRotation
);

int fMAinFunction(void){
	int x;
	int	y;
	fLoadBitmap();
	for (x = 0; x < LIMIT_SAVE_BITMAP_DATA_X_VALUE; x++){
		for (y = 0; y < LIMIT_SAVE_BITMAP_DATA_Y_VALUE; y++){
			sMainFrameBuffer[x][y][0] = sImageBuffer[x][y][0];
			sMainFrameBuffer[x][y][1] = sImageBuffer[x][y][1];
			sMainFrameBuffer[x][y][2] = sImageBuffer[x][y][2];
		}
	}
	fDrawingFrameBufferLoopFunction();
	return 0;
}

int fDrawingFrameBufferLoopFunction(void){
	void* m_FrameBuffer;
	struct  fb_fix_screeninfo m_FixInfo;
	struct  fb_var_screeninfo m_VarInfo;
	int m_FBFD;
	int int_to_char = 0;
	int x;
	int	y;
	int iFrameBufferSize;
	int iLoopControl = 1;
	int iRotation = 0;
	m_FBFD = open("/dev/fb0", O_RDWR);
	if (m_FBFD < 0){
		return 2;
	}
	if (ioctl(m_FBFD, FBIOGET_FSCREENINFO, &m_FixInfo) < 0){
		close(m_FBFD);
		return 2;
	}
	if (ioctl(m_FBFD, FBIOGET_VSCREENINFO, &m_VarInfo) < 0){
		close(m_FBFD);
		return 2;
	}
	iFrameBufferSize = m_FixInfo.line_length * m_VarInfo.yres;
	m_FrameBuffer = mmap(NULL, iFrameBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_FBFD, 0);
	if (m_FrameBuffer == NULL){
		close(m_FBFD);
		return 2;
	}
	int* p = (int*)m_FrameBuffer;
	while (iLoopControl){
		fRotation(sImageBuffer, sImageRotationBuffer,640,480, iRotation);
		iRotation++;
		if (iRotation == 360){
			iRotation = 0;
		}
		for (y = 0; y < m_VarInfo.yres; y++){
			for (x = 0; x < m_VarInfo.xres; x++){
				int_to_char = (sImageRotationBuffer[x][y][2] << 16) + (sImageRotationBuffer[x][y][1] << 8) + sImageRotationBuffer[x][y][0];
				p[x + y * m_VarInfo.xres] = int_to_char;
			}
		}
	}
	close(m_FBFD);
	return 0;
}

int fFileReadFunction(char* sFileAddress, char* sReadFileData, int iStartIndex, int iReadSize) {
	unsigned int iFileSize = 0;
	FILE* fp = fopen(sFileAddress, "rb");
	if (fp == 0) {
		return -1;
	}
	if (iReadSize == 0) {
		fseek(fp, 0, SEEK_END);
		iFileSize = (unsigned int)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		fread(sReadFileData, sizeof(char), iFileSize, fp);
	}
	else {
		fseek(fp, iStartIndex, SEEK_SET);
		fread(sReadFileData, sizeof(char), iReadSize, fp);
	}
	fclose(fp);
	return 0;
}

int getBMPinfoFuncBinToInteger(unsigned char* sData, int iDataSize, int iOffset) {
	unsigned char data[128] = "";
	unsigned int iResult = 0;
	memset(data, 0x00, sizeof(data));
	memcpy(data, sData + iOffset, iDataSize);
	switch (iDataSize) {
	case 1:
		iResult = data[0];
		break;
	case 2:
		iResult = data[1] + (data[0] * 256);
		break;
	case 3:
		iResult = data[2] + (data[1] * 256) + (data[0] * 65536);
		break;
	case 4:
		iResult = data[0] + (data[1] * 256) + (data[2] * 65536) + (data[3] * 4294967296);
		break;

	default:
		break;
	}
	return iResult;
}

int fExtractBitmapData(char* sDataFromFile, char sExtractingData[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE]){
	struct BMP_HEADER_STRUCT stBitmapHeader;
	int x;
	int	y;
	int	z;
	int res = 0;
	int i_Y_position_after = 0;
	unsigned int index = 0;
	char sOneByteBuffer[256];
	char sBuffer[1] = { 0x00 };
	memcpy(sBitmapHeaderBuffer, sDataFromFile, sizeof(char) * BMPHEADER);
	stBitmapHeader.iBitmapFileSize = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 2);
	stBitmapHeader.iButmapRealDataOffsetValue = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 10);
	stBitmapHeader.iBackendHeaderSize = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 14);
	stBitmapHeader.iBitmapXvalue = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 18);
	stBitmapHeader.iBitmapYvalue = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 22);
	stBitmapHeader.iColorPlane = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(short), 26);
	stBitmapHeader.iPixelBitDeepValue = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(short), 28);
	stBitmapHeader.iCompressType = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 30);
	stBitmapHeader.iImageSize = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 34);
	stBitmapHeader.iPixcelPerMeterXvalue = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 38);
	stBitmapHeader.iPixcelPerMeterYvalue = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 42);
	stBitmapHeader.iColorPalet = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 46);
	stBitmapHeader.iCriticalColorCount = getBMPinfoFuncBinToInteger(sBitmapHeaderBuffer, sizeof(int), 50);
	iTargetImageXsize = stBitmapHeader.iBitmapXvalue;
	iTargetImageYsize = stBitmapHeader.iBitmapYvalue;
	int iBitmapOverRunData = (iTargetImageXsize % 4);
	index = index + 54;
	for (y = 0; y < stBitmapHeader.iBitmapYvalue; y++){
		i_Y_position_after = stBitmapHeader.iBitmapYvalue - y;
		for (x = 0; x < stBitmapHeader.iBitmapXvalue; x++){
			for (z = 0; z < 3; z++){
				sExtractingData[x][i_Y_position_after][z] = sDataFromFile[index];
				index++;
			}
		}
	}
	return 0;
}

int fLoadBitmap(void){
	fFileReadFunction("./image/board.bmp", sReadBitmapFileData,0,0);
	fExtractBitmapData(sReadBitmapFileData, sImageBuffer);
}

int fRotation
(
	char sSourceData[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE],
	char sTargetData[LIMIT_SAVE_BITMAP_DATA_X_VALUE][LIMIT_SAVE_BITMAP_DATA_Y_VALUE][LIMIT_SAVE_BITMAP_DATA_C_VALUE], 
	int iWidth, int iHeight, int iRotation
)
{
	int x;
	int	y;
	int orgx, orgy;
	double rad = (360-iRotation) * pi / 180.0;
	double dcos = cos(rad);
	double dsin = sin(-rad);
	double xcenter = (double)iWidth / 2.0;
	double ycenter = (double)iHeight / 2.0;
	for (y = 0; y < iHeight; y++){
		for (x = 0; x < iWidth; x++){
			orgx = (int)(xcenter + ((double)y - ycenter) * dsin + ((double)x - xcenter) * dcos);
			orgy = (int)(ycenter + ((double)y - ycenter) * dcos - ((double)x - xcenter) * dsin);
			if ((orgy >= 0 && orgy < iHeight) && (orgx >= 0 && orgx < iWidth)){
				sTargetData[x][y][0] = sSourceData[orgx][orgy][0];
				sTargetData[x][y][1] = sSourceData[orgx][orgy][1];
				sTargetData[x][y][2] = sSourceData[orgx][orgy][2];
			}
			else{
				sTargetData[x][y][0] = 0xFF;
				sTargetData[x][y][1] = 0xFF;
				sTargetData[x][y][2] = 0xFF;
			}
		}
	}
	return 0;
}