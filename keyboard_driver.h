/*******************************************************************************
* Module Name : keyboard_driver.h
* Description : Contains function declarations for keyboard_driver.h
*******************************************************************************/
#ifndef _KEYBOARD_DRIVER_

/* Normal non-display ascii Keys returned by ConsoleGetChar*/
#define REX_KEY_BELL		'\a'
#define REX_KEY_TAB			'\t'
#define REX_KEY_BACKSPACE	'\b'
#define REX_KEY_RETURN		'\r'
#define REX_KEY_NEWLINE		'\n'
#define REX_KEY_SPACE		' '
#define	REX_KEY_ESCAPE		27 
#define REX_KEY_ASCII_DEL	127
#define REX_KEY_ESC_SEQ		'['		/* Used in Terminal Escape Sequence */


/* Special non-ascii Keys returned by ConsoleGetChar*/
#define REX_KEY_UP		0xFF00
#define REX_KEY_DOWN		0xFF01
#define REX_KEY_LEFT		0xFF02
#define REX_KEY_RIGHT		0xFF03

#define REX_KEY_PGUP		0xFF10
#define REX_KEY_PGDN		0xFF11

#define REX_KEY_DEL		0xFF20
#define REX_KEY_INS		0xFF21

#define REX_KEY_HOME		0xFF30
#define REX_KEY_END		0xFF31

#define REX_KEY_F1		0xFF80
#define REX_KEY_F2		0xFF81
#define REX_KEY_F3		0xFF82
#define REX_KEY_F4		0xFF83

/* Check the following values are correct for WIN32/DOS */
#define REX_KEY_F5		0xFF84
#define REX_KEY_F6		0xFF85
#define REX_KEY_F7		0xFF86
#define REX_KEY_F8		0xFF87
#define REX_KEY_F9		0xFF88
#define REX_KEY_F10		0xFF89
#define REX_KEY_F11		0xFF8A
#define REX_KEY_F12		0xFF8B

#define SSH_BACKSPACE		127

#define PROMPT_STR_LEN		0
// Maximum Line Width of console
#define LINE_LEN		250
#define MAX_CMD_SIZE		255


void OpenConsole(int rawmode);
void ConsoleClear(void);
void CloseConsole(void);
unsigned short GetCmdLine(char *CmdLine, unsigned short Index, int isPassword);
void GetWindowSize(void);
void HandleWindowResize(int signal);
void ConsolePutStr(char *Str);

#endif