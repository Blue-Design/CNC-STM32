#include <string.h>
#include <stdbool.h>
#include "global.h"
#include "screen_io.h"

#define CONF_FILE_NAME "sm.conf"

#define MAX_FILE_LIST_SZ 256
#define MAX_FILE_NAME_SZ 80
#define FILE_LIST_ROWS 8

void SystemStartup(void);
uint8_t SystemProcess(void);
void SystemStatus(uint32_t seconds);

char fileList[MAX_FILE_LIST_SZ][MAX_FILE_NAME_SZ];
char loadedFileName[MAX_FILE_NAME_SZ];
int fileListSz = 0, currentFile = 0, firstFileInWin = 0;
FATFS fatfs;

/***************************************************
 *	Replace CR/LF to EOL char.
 */
char *str_trim(char *str)
{
	char ch, *p = str;
	while ((ch = *p) != '\0')
	{
		if (ch == '\n' || ch == '\r')
		{
			*p = 0;
			break;
		}
		p++;
	}
	return str;
}
/***************************************************
 *	Initialize _smParam and load from config file
 */
void initSmParam(void)
{
	_smParam.smoothStartF_from0[0] = SM_SMOOTH_START_X*K_FRQ;
	_smParam.smoothStartF_from0[1] = SM_SMOOTH_START_Y*K_FRQ;
	_smParam.smoothStartF_from0[2] = SM_SMOOTH_START_Z*K_FRQ;
	_smParam.smoothStopF_to0[0] = SM_SMOOTH_STOP_X*K_FRQ;
	_smParam.smoothStopF_to0[1] = SM_SMOOTH_STOP_Y*K_FRQ;
	_smParam.smoothStopF_to0[2] = SM_SMOOTH_STOP_Z*K_FRQ;
	_smParam.smoothAF[0] = SM_SMOOTH_DFEED_X*SM_X_STEPS_PER_MM*SM_SMOOTH_TFEED*K_FRQ / 1000;
	_smParam.smoothAF[1] = SM_SMOOTH_DFEED_Y*SM_Y_STEPS_PER_MM*SM_SMOOTH_TFEED*K_FRQ / 1000;
	_smParam.smoothAF[2] = SM_SMOOTH_DFEED_Z*SM_Z_STEPS_PER_MM*SM_SMOOTH_TFEED / 1000 * K_FRQ;
	_smParam.maxFeedRate[0] = SM_X_MAX_STEPS_PER_SEC*K_FRQ;
	_smParam.maxFeedRate[1] = SM_Y_MAX_STEPS_PER_SEC*K_FRQ;
	_smParam.maxFeedRate[2] = SM_Z_MAX_STEPS_PER_SEC*K_FRQ;
	_smParam.maxSpindleTemperature = MAX_SPINDEL_TEMPERATURE;

#if (USE_SDCARD == 1)
	FIL fid;
	char str[256], *p;
	int i;
	FRESULT fres = f_open(&fid, CONF_FILE_NAME, FA_READ);
	if (fres == FR_OK)
	{
		scr_printf("\nloading %s", CONF_FILE_NAME);
		for (i = 0; i < 3; i++)
		{
			scr_puts(".");
			if (f_gets(str, sizeof(str), &fid) == NULL)
				break;
			DBG("\nc:%d:'%s'", i, str);
			if (f_gets(str, sizeof(str), &fid) == NULL)
				break;
			DBG("\nd:%d:'%s'", i, str);
			p = str;
			_smParam.smoothStartF_from0[i] = strtod_M(p, &p);
			_smParam.smoothStopF_to0[i] = strtod_M(p, &p);
			_smParam.smoothAF[i] = strtod_M(p, &p);
			_smParam.maxFeedRate[i] = strtod_M(p, &p);
		}
		if (f_gets(str, sizeof(str), &fid) != NULL)
		{
			DBG("t:'%s'", str);
			if (f_gets(str, sizeof(str), &fid) != NULL)
				_smParam.maxSpindleTemperature = strtod_M(str, &p);
		}
		scr_puts("*");
		f_close(&fid);
		scr_puts(" OK");
	}
#endif
}

/***************************************************
 *	Save _smParam to config file
 */
void saveSmParam(void)
{
#if (USE_SDCARD == 1)
	FIL fid;
	int i;

	f_unlink(CONF_FILE_NAME);

	FRESULT fres = f_open(&fid, CONF_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);
	if (fres != FR_OK)
	{
		win_showErrorWin();
		scr_printf("Error save file:'%s'\n Status:%d [%d/%d]", CONF_FILE_NAME, fres, SD_errno, SD_step);
		scr_puts("\n\n\t PRESS C-KEY");
		WAIT_KEY_C();
		return;
	}

	scr_printf("\nSave into %s", CONF_FILE_NAME);
	for (i = 0; i < 3; i++)
	{
		f_printf(&fid, "Crd%d (F:steps*%d/sec): 'smoothStartF_from0,smoothStopF_to0,smoothAF,maxFeedRate\n", i, K_FRQ);
		f_printf(&fid, "%d,%d,%d,%d,%d\n",
			_smParam.smoothStartF_from0[i],
			_smParam.smoothStopF_to0[i],
			_smParam.smoothAF[i],
			_smParam.maxFeedRate[i]
		);
		scr_puts(".");
	}
	f_printf(&fid, "Spindle switch-off temperature (C.degree)\n");
	f_printf(&fid, "%d\n", _smParam.maxSpindleTemperature);
	scr_puts("*");
	f_close(&fid);
	scr_puts(" - OK");
#endif
}

/***************************************************
 *	Show status
 */
void showStatusString(void)
{
	static uint8_t sec = 0, limits = 0;

	if (Seconds() != sec)
	{	// Display every second
		scr_setfullTextWindow();
		scr_gotoxy(2, TEXT_Y_MAX - 1);
		scr_fontColor(White, Black);
#if (USE_RTC == 1)
		RTC_t rtc;
		rtc_gettime(&rtc);
		scr_printf("%02d.%02d.%02d %02d:%02d:%02d ", rtc.mday, rtc.month, rtc.year - 2000, rtc.hour, rtc.min, rtc.sec);
#else
		scr_printf("%02d:%02d:%02d ", ((Seconds() / 3600) % 24), ((Seconds() / 60) % 60), (Seconds() % 60));
#endif

#if (USE_EXTRUDER == 1)
		scr_fontColor(_smParam.maxSpindleTemperature > extrudT_getTemperatureReal() ? Green : Red, Black);
		scr_printf("t:%dC", extrudT_getTemperatureReal());
#endif
		SystemStatus(sec);

		// scr_clrEndl();
		sec = Seconds();
		limits = 0xFF;
	}

	if (limits != limits_chk())
	{
		limits = limits_chk();
		scr_Rectangle(283, 224, 293, 239, limitX_chk() ? Red : Green, true);
		scr_Rectangle(296, 224, 306, 239, limitY_chk() ? Red : Green, true);
		scr_Rectangle(309, 224, 319, 239, limitZ_chk() ? Red : Green, true);
	}
}

/***************************************************
 *	Show critical status
 */
void showCriticalStatus(char *msg, int st)
{
	win_showErrorWin();
	scr_gotoxy(2, 2);
	scr_printf(msg, st);
	WAIT_KEY_C();
}

/***************************************************
 *	Show critical status
 */
void showNonCriticalStatus(char *msg, int st)
{
	win_showErrorWin();
	scr_gotoxy(2, 2);
	scr_printf(msg, st);
	WAIT_KEY_C();
}

/***************************************************
 *	Read file list
 */
const char path[] = "0:";
void readFileList(void)
{
#if (USE_SDCARD == 1) || defined( USE_USB_OTG_HS )
	FRESULT fres;
	FILINFO finfo;
	DIR dirs;
	static char lfn[_MAX_LFN + 1];
	int i;

	win_showMsgWin();
	scr_gotoxy(2, 0);
	scr_printf("Open SD dir.. ");
	if ((fres = f_opendir(&dirs, path)) != FR_OK)
	{
		showCriticalStatus(" f_opendir()\n  error [code:%d]\n  Only RESET possible at now", fres);
		WAIT_KEY_C();
	}

	scr_puts("\nRead file list");
	finfo.lfname = lfn;
	finfo.lfsize = sizeof(lfn);
	for (fileListSz = 0; f_readdir(&dirs, &finfo) == FR_OK && fileListSz < MAX_FILE_LIST_SZ;)
	{
		scr_gotoxy(0, 3);
		scr_printf("files:[%d]", fileListSz);
		if (!finfo.fname[0])
			break;
		if (finfo.fname[0] == '.')
			continue;
		if (!(finfo.fattrib & AM_DIR) && strcmp(CONF_FILE_NAME, *finfo.lfname ? finfo.lfname : finfo.fname) != 0)
			strncpy(&fileList[fileListSz++][0], *finfo.lfname ? finfo.lfname : finfo.fname, MAX_FILE_NAME_SZ);
	}
	
	if (loadedFileName[0] != 0)
	{	// set last loaded file as selected file
		scr_printf("\nselect:'%s'..", loadedFileName);
		for (i = 0; i < fileListSz; i++)
		{
			if (!strcmp(&fileList[i][0], &loadedFileName[0]))
			{
				loadedFileName[0] = 0; // reset for next dir reload
				currentFile = i;
				if (currentFile < FILE_LIST_ROWS)
					firstFileInWin = 0;
				else if ((fileListSz - currentFile) < FILE_LIST_ROWS)
					firstFileInWin = fileListSz - FILE_LIST_ROWS;
				else
					firstFileInWin = currentFile;
				break;
			}
		}
		scr_printf("\npos in win/cur file:%d/%d", firstFileInWin, currentFile);
	}
	scr_puts("\n---- OK -----");
#endif
}
/***************************************************
 *	Draw file list
 */
void drawFileList(void)
{
	if (currentFile >= fileListSz)
		currentFile = 0;
	if (currentFile < 0)
		currentFile = fileListSz - 1;
	if ((firstFileInWin + currentFile) >= FILE_LIST_ROWS)
		firstFileInWin = currentFile - FILE_LIST_ROWS + 1;
	if (firstFileInWin > currentFile)
		firstFileInWin = currentFile;
	if (firstFileInWin < 0)
		firstFileInWin = 0;
	win_showMenuScroll(0, 0, 38, FILE_LIST_ROWS, firstFileInWin, currentFile, fileListSz);
	for (int i = 0; i < fileListSz; i++)
	{
		if (i == currentFile)
			scr_fontColorInvers();
		else
			scr_fontColorNormal();
		scr_printf("%s\n", &fileList[i][0]);
	}
}

#if (USE_KEYBOARD == 2)
const TPKey_t TPKeyYes = TPKEY( 16, 145, 100, 216, KEY_D, "YES" );
const TPKey_t TPKeyNo  = TPKEY(104, 145, 188, 216, KEY_C, "NO");
const TPKey_p kbdQuestion[] = {
	&TPKeyYes,
	&TPKeyNo,
	NULL
};
#endif

#if (USE_KEYBOARD != 0)
uint8_t questionYesNo(char *msg, char *param)
{
	win_showMsgWin();
	scr_printf(msg, param);
	scr_gotoxy(5, 6); scr_puts("'D' - OK,  'C' - Cancel");
	SetTouchKeys(kbdQuestion);
	while (true)
		switch(kbd_getKey())
		{
			case KEY_D:
				return true;
			case KEY_C:
				return false;
		}
}

#if (USE_RTC == 1)
void setTime(void)
{
	RTC_t rtc;
	int c = -1, pos = 0, v = 0;
	
	win_showMsgWin();
	scr_setScroll(false);
	rtc_gettime(&rtc);
	scr_puts("D-ENTER C-CANCEL A-Up B-Down");
	scr_puts("\n'*' -Left '#' -RIGHT");
	scr_printf("\n\nTime: %02d.%02d.%02d %02d:%02d:%02d", rtc.mday, rtc.month, rtc.year - 2000, rtc.hour, rtc.min, rtc.sec);
	do
	{
		switch (pos)
		{
		case 0: v = rtc.mday; break;
		case 1: v = rtc.month; break;
		case 2: v = rtc.year - 2000; break;
		case 3: v = rtc.hour; break;
		case 4: v = rtc.min; break;
		case 5: v = rtc.sec; break;
		}
		if (c == KEY_STAR)	pos = pos <= 0 ? 5 : pos - 1;
		if (c == KEY_DIES) pos = pos >= 5 ? 0 : pos + 1;

		scr_gotoxy(0, 4);
		scr_fontColorNormal();
		scr_printf(" New: %02d.%02d.%02d %02d:%02d:%02d", rtc.mday, rtc.month, rtc.year - 2000, rtc.hour, rtc.min, rtc.sec);
		scr_fontColorInvers();
		scr_gotoxy(pos * 3 + 6, 3);
		scr_printf("%02d", v);
		while ((c = kbd_getKey()) < 0);

		if (c == KEY_A) v++;
		if (c == KEY_B)	v--;

		switch (pos) {
		case 0:
			if (v >= 1 && v <= 31) rtc.mday = v;
			break;
		case 1:
			if (v >= 1 && v <= 12) rtc.month = v;
			break;
		case 2:
			if (v >= 12 && v <= 30) rtc.year = v + 2000;
			break;
		case 3:
			if (v >= 0 && v <= 23) rtc.hour = v;
			break;
		case 4:
			if (c >= 0 && v <= 59)	rtc.min = v;
			break;
		case 5:
			if (v >= 0 && v <= 59)	rtc.sec = v;
			break;
		}
	} while (c != KEY_C && c != KEY_D);

	if (c == KEY_D) rtc_settime(&rtc);
}
#endif	/* USE_RTC == 1 */
#endif	/* USE_KEYBOARD == 1*/

#if (USE_KEYBOARD == 2)
const TPKey_t TPKeyDown	= TPKEY(  0, 145,  76, 216, KEY_B, "DOWN");
const TPKey_t TPKeyUp	= TPKEY( 84, 145, 156, 216, KEY_A, "UP");
const TPKey_t TPKey5	= TPKEY(164, 145, 236, 216, KEY_5, "INFO");
const TPKey_t TPKey0	= TPKEY(244, 145, 319, 216, KEY_0, "START");
const TPKey_p kbdSelectFile[] = {
	&TPKeyDown,
	&TPKeyUp,
	&TPKey5,
	&TPKey0,
	NULL
};

const TPKey_t TPKeyC	= TPKEY(  0, 208,  319, 239, KEY_C, "Press C");
const TPKey_p kbdLast2Lines[] = {
	&TPKeyC,
	NULL
};

#endif

/***************************************************
 *	Main
 */
int main()
{
	bool rereadDir = false, redrawScr = true, redrawDir = false;

	SystemStartup();

#if (USE_SDCARD == 1)
	rereadDir = true;
	FRESULT fres = f_mount(0, &fatfs);
	if (fres != FR_OK)
	{
		showCriticalStatus(
" Mount SD error [code:%d]\n"
" SD card used for any CNC process\n"
" Only RESET possible at now", fres);
		WAIT_KEY_C();
	}
#elif (USE_SDCARD == 2)
	FRESULT fres;
#endif

	initSmParam();

	while (1)
	{
		if (SystemProcess() == SYS_READ_FLASH)
			rereadDir = true;

		if (rereadDir)
		{
			rereadDir = false;
			readFileList();
			redrawScr = true;
		}

		if (redrawScr)
		{
			redrawScr = false;
			LCD_Clear(Black);
#if (USE_KEYBOARD == 1)
			win_showMenu(0, 144, 40, 4);
			scr_puts(
				"0 - start gcode   1 - manual mode\n"
				"2 - show gcode    3 - delete file\n"
				"4 - set time      5 - file info\n"
				//"6 - scan mode\t "
				"7 - save conf.file (v1.1)");
#endif
#if (USE_KEYBOARD == 2)
			SetTouchKeys(kbdSelectFile);
#endif
			redrawDir = true;
		}
		if (redrawDir)
		{
			redrawDir = false;
			drawFileList();
		}

		showStatusString();

#if (USE_KEYBOARD != 0)
		switch (kbd_getKey())
		{
		case KEY_A:
			currentFile--;
			redrawDir = true;
			break;
		case KEY_B:
			currentFile++;
			redrawDir = true;
			break;
	#if (USE_KEYBOARD == 1)
		case KEY_STAR:
			currentFile += FILE_LIST_ROWS;
			redrawDir = true;
			break;
		case KEY_DIES:
			currentFile -= FILE_LIST_ROWS;
			redrawDir = true;
			break;
	#endif
	//
	// Start GCode
	//
		case KEY_0:
		{
			FLASH_KEYS();

#if (USE_LCD == 1)
			uint32_t stime;
			stime = Seconds();
#endif
			cnc_gfile(&fileList[currentFile][0], GFILE_MODE_MASK_EXEC);
			while (stepm_inProc())
			{
				scr_fontColor(Yellow, Blue);
				scr_gotoxy(1, 13);
				scr_printf(" remain moves: %d", stepm_getRemainLines());
				scr_clrEndl();
			}
			stepm_EmergeStop();

#if (USE_LCD == 1)
			scr_fontColor(Yellow, Blue);
			scr_gotoxy(0, 13);
			scr_puts("   FINISH. PRESS C-KEY");
			scr_clrEndl();
			
			stime = Seconds() - stime;
			scr_fontColor(Yellow, Blue);
			scr_gotoxy(0, 14);
			scr_printf("   work time: %02d:%02d", stime / 60, stime % 60);
			scr_clrEndl();
#endif
#if (USE_LCD == 2)
			SetTouchKeys(kbdLast2Lines);
#endif
			FLASH_KEYS();
			WAIT_KEY_C();

			redrawScr = true;
			break;
		}
	//
	// Manual Mode
	//
		case KEY_1:
			manualMode();
			redrawScr = true;
			break;
	#if (USE_SDCARD == 1)
	//
	// Show GCode
	//
		case KEY_2:
			FLASH_KEYS();
			cnc_gfile(&fileList[currentFile][0], GFILE_MODE_MASK_SHOW | GFILE_MODE_MASK_CHK);
			scr_printf("\n              PRESS C-KEY");
			FLASH_KEYS();
			while (kbd_getKey() != KEY_C);
			redrawScr = true;
			break;
	//
	// Delete file
	//
		case KEY_3:
			if (questionYesNo("Delete file:\n'%s'?", &fileList[currentFile][0]))
			{
				rereadDir = true;
				f_unlink(&fileList[currentFile][0]);
			}
			else
				redrawScr = true;
			break;
	#endif
	#if (USE_RTC == 1)
	//
	// Set time
	//
		case KEY_4:
			setTime();
			redrawScr = true;
			break;
	//
	// Save conf. file
	//
		case KEY_7:
			win_showMsgWin();
			saveSmParam();
			scr_printf("\n\n\n        PRESS C-KEY");
			WAIT_KEY_C();
			redrawScr = true;
			break;
	#endif
	#if (USE_SDCARD != 0)
	//
	// File info
	//
		case KEY_5:
		{
			FILINFO finf;
			FIL fid;
			int c, n;

			memset(&finf, 0, sizeof(finf));

			win_showMsgWin();
			scr_setScroll(false);
			scr_printf("File:%s", &fileList[currentFile][0]);
			f_stat(&fileList[currentFile][0], &finf);
			scr_gotoxy(0, 1);
			scr_printf("Size:%d\n", (uint32_t)finf.fsize);

			fres = f_open(&fid, &fileList[currentFile][0], FA_READ);
			if (fres != FR_OK)
			{
	#if (USE_SDCARD == 1)
				scr_printf("Error open file: '%s'\nStatus:%d [%d]", &fileList[currentFile][0], fres, SD_errno);
	#elif (USE_SDCARD == 2)
				scr_printf("Error open file: '%s'\nStatus:%d", &fileList[currentFile][0], fres);
	#endif
			}
			else
			{
				char str[150];
				scr_fontColorInvers();
				scr_setScroll(false);
				// Read lines from 3 to 6 from file and display it
				for (n = 2; n < 7 && f_gets(str, sizeof(str), &fid) != NULL; n++)
				{
					scr_gotoxy(0, n);
					scr_puts(str_trim(str));
				}
			}
			scr_fontColorNormal();
			scr_gotoxy(8, 7);
			scr_printf("PRESS C-KEY");
			redrawScr = true;
			do {
				c = kbd_getKey();
				if (c == KEY_B)
				{
					char str[150];
					scr_fontColorInvers();
					for (n = 2; n < 7 && f_gets(str, sizeof(str), &fid) != NULL; n++)
					{
						scr_gotoxy(0, n);
						scr_puts(str_trim(str));
						scr_clrEndl();
					}
				}
			} while (c != KEY_C);
			f_close(&fid);
			rereadDir = true;
		}
		break;
	#endif
		}
#endif
	}
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
	/* User can add his own implementation to report the file name and line number,
		ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* Infinite loop */
	while (1)
	{ }
}

#endif
