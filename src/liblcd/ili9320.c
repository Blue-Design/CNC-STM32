#include "ili9320.h"

#ifdef HAS_LCD

#include "ili9320_font.h"

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_SetCursor(uint16_t x, uint16_t y)
{
	*(__IO uint16_t *) (Bank1_LCD_C) = 32;
	*(__IO uint16_t *) (Bank1_LCD_D) = y;

	*(__IO uint16_t *) (Bank1_LCD_C) = 33;
	*(__IO uint16_t *) (Bank1_LCD_D) = LCD_WIDTH - 1 - x;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_WrVcamLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color[])
{
	int i;
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x20;
	*(__IO uint16_t *) (Bank1_LCD_D) = y;
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x21;
	*(__IO uint16_t *) (Bank1_LCD_D) = LCD_WIDTH - 1 - x;
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x22;
	for (i = 0; i < h; i++)
		*(__IO uint16_t *) (Bank1_LCD_D) = color[i];
}

/****************************************************************************
 *
 ****************************************************************************/
void Lcd_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMTimingInitTypeDef  NORSRAMTiming;
	FSMC_NORSRAMInitTypeDef NORSRAMInit;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = LCD_RST_PIN;
	GPIO_Init(LCD_RST_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = LCD_BACKLIGHT_PIN;
	GPIO_Init(LCD_BACKLIGHT_PORT, &GPIO_InitStructure);

	//------------------------- FSMC pins
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = LCD_DATA_D_PINS;
	GPIO_Init(LCD_DATA_D_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD_DATA_E_PINS;
	GPIO_Init(LCD_DATA_E_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = LCD_CTRL_PINS;
	GPIO_Init(LCD_CTRL_PORT, &GPIO_InitStructure);

	//------------------------- FSMC controller
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);

	NORSRAMTiming.FSMC_AddressSetupTime = 0x02;
	NORSRAMTiming.FSMC_AddressHoldTime = 0x01;
	NORSRAMTiming.FSMC_DataSetupTime = 0x05;
	NORSRAMTiming.FSMC_BusTurnAroundDuration = 0x00;
	NORSRAMTiming.FSMC_CLKDivision = 0x01;
	NORSRAMTiming.FSMC_DataLatency = 0x00;
	NORSRAMTiming.FSMC_AccessMode = FSMC_AccessMode_B;

	NORSRAMInit.FSMC_Bank = FSMC_Bank1_NORSRAM1;
	NORSRAMInit.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	NORSRAMInit.FSMC_MemoryType = FSMC_MemoryType_NOR;
	NORSRAMInit.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	NORSRAMInit.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	NORSRAMInit.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	NORSRAMInit.FSMC_WrapMode = FSMC_WrapMode_Disable;
	NORSRAMInit.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	NORSRAMInit.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	NORSRAMInit.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	NORSRAMInit.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	NORSRAMInit.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	NORSRAMInit.FSMC_ReadWriteTimingStruct = &NORSRAMTiming;
	NORSRAMInit.FSMC_WriteTimingStruct = &NORSRAMTiming;
	FSMC_NORSRAMInit(&NORSRAMInit);

	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
}

/****************************************************************************
 *
 ****************************************************************************/
void LCD_WR_REG(uint32_t index)
{
	*(__IO uint16_t *) (Bank1_LCD_C) = index;
}

/****************************************************************************
 *
 ****************************************************************************/
void LCD_WR_CMD(uint32_t index, uint32_t val)
{
	*(__IO uint16_t *) (Bank1_LCD_C) = index;
	*(__IO uint16_t *) (Bank1_LCD_D) = val;
}

/****************************************************************************
 *
 ****************************************************************************/
uint32_t LCD_RD_data(void)
{
	uint32_t a = 0;
	a = (*(__IO uint16_t *) (Bank1_LCD_D));	//Dummy
	a = (*(__IO uint16_t *) (Bank1_LCD_D));	//L
	return(a);
}

/****************************************************************************
 *
 ****************************************************************************/
void LCD_WR_Data(uint32_t val)
{
	*(__IO uint16_t *) (Bank1_LCD_D) = val;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_Delay(__IO uint32_t nCount)
{
	for (; nCount != 0; nCount--)
		__NOP();
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_Initializtion()
{
	LCD_RESET_LOW();
	ili9320_Delay(0xAFFF);
	LCD_RESET_HIGH();
	ili9320_Delay(0xAFFF);

	LCD_WR_CMD(0x00E3, 0x3008); // Set internal timing
	LCD_WR_CMD(0x00E7, 0x0012); // Set internal timing
	LCD_WR_CMD(0x00EF, 0x1231); // Set internal timing
	LCD_WR_CMD(0x0000, 0x0001); // Start Oscillation
	LCD_WR_CMD(0x0001, 0x0100); // set SS and SM bit
	LCD_WR_CMD(0x0002, 0x0700); // set 1 line inversion

	LCD_WR_CMD(0x0003, 0x1030); // set GRAM write direction and BGR=0,262K colors,1 transfers/pixel.
	LCD_WR_CMD(0x0004, 0x0000); // Resize register
	LCD_WR_CMD(0x0008, 0x0202); // set the back porch and front porch
	LCD_WR_CMD(0x0009, 0x0000); // set non-display area refresh cycle ISC[3:0]
	LCD_WR_CMD(0x000A, 0x0000); // FMARK function
	LCD_WR_CMD(0x000C, 0x0000); // RGB interface setting
	LCD_WR_CMD(0x000D, 0x0000); // Frame marker Position
	LCD_WR_CMD(0x000F, 0x0000); // RGB interface polarity

	//Power On sequence
	LCD_WR_CMD(0x0010, 0x0000); // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0011, 0x0007); // DC1[2:0], DC0[2:0], VC[2:0]
	LCD_WR_CMD(0x0012, 0x0000); // VREG1OUT voltage
	LCD_WR_CMD(0x0013, 0x0000); // VDV[4:0] for VCOM amplitude

	ili9320_Delay(200); // Dis-charge capacitor power voltage

	LCD_WR_CMD(0x0010, 0x1690); // SAP, BT[3:0], AP, DSTB, SLP, STB
	LCD_WR_CMD(0x0011, 0x0227); // R11h=0x0221 at VCI=3.3V, DC1[2:0], DC0[2:0], VC[2:0]
	ili9320_Delay(50); // Delay 50ms
	LCD_WR_CMD(0x0012, 0x001C); // External reference voltage= Vci;
	ili9320_Delay(50); // Delay 50ms
	LCD_WR_CMD(0x0013, 0x1800); // R13=1200 when R12=009D;VDV[4:0] for VCOM amplitude
	LCD_WR_CMD(0x0029, 0x001C); // R29=000C when R12=009D;VCM[5:0] for VCOMH
	LCD_WR_CMD(0x002B, 0x000D); // Frame Rate = 91Hz
	ili9320_Delay(50); // Delay 50ms
	LCD_WR_CMD(0x0020, 0x0000); // GRAM horizontal Address
	LCD_WR_CMD(0x0021, 0x0000); // GRAM Vertical Address

	// ----------- Adjust the Gamma Curve ----------//
	LCD_WR_CMD(0x0030, 0x0007);
	LCD_WR_CMD(0x0031, 0x0302);
	LCD_WR_CMD(0x0032, 0x0105);
	LCD_WR_CMD(0x0035, 0x0206);
	LCD_WR_CMD(0x0036, 0x0808);
	LCD_WR_CMD(0x0037, 0x0206);
	LCD_WR_CMD(0x0038, 0x0504);
	LCD_WR_CMD(0x0039, 0x0007);
	LCD_WR_CMD(0x003C, 0x0105);
	LCD_WR_CMD(0x003D, 0x0808);

	//------------------ Set GRAM area ---------------//
	LCD_WR_CMD(0x0050, 0x0000); // Horizontal GRAM Start Address
	LCD_WR_CMD(0x0051, 0x00EF); // Horizontal GRAM End Address
	LCD_WR_CMD(0x0052, 0x0000); // Vertical GRAM Start Address
	LCD_WR_CMD(0x0053, 0x013F); // Vertical GRAM Start Address
	LCD_WR_CMD(0x0060, 0xA700); // Gate Scan Line
	LCD_WR_CMD(0x0061, 0x0001); // NDL,VLE, REV
	LCD_WR_CMD(0x006A, 0x0000); // set scrolling line

	//-------------- Partial Display Control ---------//
	LCD_WR_CMD(0x0080, 0x0000);
	LCD_WR_CMD(0x0081, 0x0000);
	LCD_WR_CMD(0x0082, 0x0000);
	LCD_WR_CMD(0x0083, 0x0000);
	LCD_WR_CMD(0x0084, 0x0000);
	LCD_WR_CMD(0x0085, 0x0000);

	//-------------- Panel Control -------------------//
	LCD_WR_CMD(0x0090, 0x0010);
	LCD_WR_CMD(0x0092, 0x0000);
	LCD_WR_CMD(0x0093, 0x0003);
	LCD_WR_CMD(0x0095, 0x0110);
	LCD_WR_CMD(0x0097, 0x0000);
	LCD_WR_CMD(0x0098, 0x0000);
	LCD_WR_CMD(0x0007, 0x0133); // 262K color and display ON

	LCD_WR_CMD(32, 0);
	LCD_WR_CMD(33, LCD_WIDTH - 1);
	*(__IO uint16_t *) (Bank1_LCD_C) = 34;

	for (int n = 0; n < (LCD_HEIGHT * LCD_WIDTH); n++)
		*(__IO uint16_t *) (Bank1_LCD_D) = 0xF17F;
}

/****************************************************************************
 *
 ****************************************************************************/
__INLINE void ili9320_SetWindows(u16 StartX, u16 StartY, u16 EndX, u16 EndY)
{
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x50;
	*(__IO uint16_t *) (Bank1_LCD_D) = StartY;

	*(__IO uint16_t *) (Bank1_LCD_C) = 0x51;
	*(__IO uint16_t *) (Bank1_LCD_D) = EndX;

	*(__IO uint16_t *) (Bank1_LCD_C) = 0x52;
	*(__IO uint16_t *) (Bank1_LCD_D) = LCD_WIDTH - 1 - StartX;	// 399 ?

	*(__IO uint16_t *) (Bank1_LCD_C) = 0x53;
	*(__IO uint16_t *) (Bank1_LCD_D) = EndY;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_Clear(u16 dat)
{
	uint32_t i;
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x50;
	*(__IO uint16_t *) (Bank1_LCD_D) = 0;

	*(__IO uint16_t *) (Bank1_LCD_C) = 0x51;
	*(__IO uint16_t *) (Bank1_LCD_D) = LCD_HEIGHT - 1;
	
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x52;
	*(__IO uint16_t *) (Bank1_LCD_D) = 0;
	
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x53;
	*(__IO uint16_t *) (Bank1_LCD_D) = LCD_WIDTH - 1;
	
	*(__IO uint16_t *) (Bank1_LCD_C) = 32;
	*(__IO uint16_t *) (Bank1_LCD_D) = 0;
	*(__IO uint16_t *) (Bank1_LCD_C) = 33;
	*(__IO uint16_t *) (Bank1_LCD_D) = 0;

	*(__IO uint16_t *) (Bank1_LCD_C) = 34;
	for (i = 0; i < (LCD_HEIGHT * LCD_WIDTH); i++)
		*(__IO uint16_t *) (Bank1_LCD_D) = dat;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_WriteData(u16 dat)
{
	*(__IO uint16_t *) (Bank1_LCD_D) = dat;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_WriteIndex(u16 idx)
{
	*(__IO uint16_t *) (Bank1_LCD_C) = idx;
}

/****************************************************************************
 *
 ****************************************************************************/
uint16_t ili9320_ReadData(void)
{
	u16 val = 0;
	val = LCD_RD_data();
	return val;
}

/****************************************************************************
 *
 ****************************************************************************/
uint16_t ili9320_ReadRegister(u16 index)
{
	u16 tmp;
	tmp = *(volatile uint32_t *)(Bank1_LCD_C);
	return tmp;
}

void ili9320_WriteRegister(u16 index, u16 dat)
{
	/************************************************************************
	 **                                                                    **
	 ** nCS       ----\__________________________________________/-------  **
	 ** RS        ------\____________/-----------------------------------  **
	 ** nRD       -------------------------------------------------------  **
	 ** nWR       --------\_______/--------\_____/-----------------------  **
	 ** DB[0:15]  ---------[index]----------[data]-----------------------  **
	 **                                                                    **
	 ************************************************************************/
	*(__IO uint16_t *) (Bank1_LCD_C) = index;
	*(__IO uint16_t *) (Bank1_LCD_D) = dat;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_SetPoint(u16 x, u16 y, u16 point)
{
	if ((y >= LCD_HEIGHT) || (x >= LCD_WIDTH)) return;
	*(__IO uint16_t *) (Bank1_LCD_C) = 32;
	*(__IO uint16_t *) (Bank1_LCD_D) = y;

	*(__IO uint16_t *) (Bank1_LCD_C) = 33;
	*(__IO uint16_t *) (Bank1_LCD_D) = (LCD_WIDTH - 1) - x;

	*(__IO uint16_t *) (Bank1_LCD_C) = 34;
	*(__IO uint16_t *) (Bank1_LCD_D) = point;
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_VLine(u16 x0, u16 y0, u16 h, u16 color)
{
	int i;
	ili9320_SetCursor(x0, y0);
	*(__IO uint16_t *) (Bank1_LCD_C) = 0x22;
	for (i = 0; i < h; i++)
	{
		*(__IO uint16_t *) (Bank1_LCD_D) = color;
	}
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_PutChar(u16 x, u16 y, u8 c, u16 charColor, u16 bkColor)
{
	u16 i, j;
	u8 data;
	u8 mask;
	const u8 * datas = &ascii_8x16[((c - ' ') * FONT_STEP_Y)];

	for (i = 0; i < FONT_STEP_Y; i++)
	{
		data = *datas++;
		mask  = 0x80;
#if (FONT_WIDTH == 8)
		while (mask != 0)
		{
			ili9320_SetPoint(x + j, y + i, (data & mask ? charColor : bkColor));
			mask >>= 1;
		}
#else
		for (j = 0; j < FONT_STEP_X; j++)
		{
			ili9320_SetPoint(x + j, y + i, (data & mask ? charColor : bkColor));
			mask >>= 1;
		}
#endif
	}
}

/****************************************************************************
 *
 ****************************************************************************/
void ili9320_BackLight(u8 status)
{
	if (status >= 1)
		LCD_BACKLIGHT_ON();
	else
		LCD_BACKLIGHT_OFF();
}

#endif
