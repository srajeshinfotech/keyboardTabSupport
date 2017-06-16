/*******************************************************************************
* Module Name : keyboard_driver.c
* Description : Contains function declarations for keyboard_driver.c
********************************************************************************/
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include "keyboard_driver.h"

static int RawConsole = 0;
static int Opened = 0;
static struct termios orgt;
static int peek = -1;

// holds the window column size
int g_ColumnLen = 80;

/******************************************************************************
* Function Name : Unix_getch
* Parameters    : NULL
* Description   :
* Return Value  : NULL
******************************************************************************/

static int Unix_getch(void)
{
	int ch;
	/* Check if we already peek the character using kbhit */
	if (peek != -1)
	{
		ch = peek;
		peek = -1;
		return ch;
	}
	return getchar();
}

/******************************************************************************
* Function Name : Termios_Unix_kbhit
* Parameters    : NULL
* Description   :
* Return Value  : NULL
******************************************************************************/

static int Termios_Unix_kbhit(void)
{
	struct termios oldt,newt;
	int ch;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_cc[VMIN] = 1;
	newt.c_cc[VTIME] = 0;
	newt.c_cc[VINTR] = _POSIX_VDISABLE;

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	ch = getchar();
	tcsetattr(STDIN_FILENO,TCSANOW, &oldt);
	if (ch != EOF)
	{
		peek = ch;
		return 1;
	}
	return 0;
}

/******************************************************************************
* Function Name : Unix_kbhit
* Parameters    : NULL
* Description   :
* Return Value  : NULL
******************************************************************************/
static int Unix_kbhit(void)
{
	if (peek != -1) {
		return 1;
	}
	return Termios_Unix_kbhit();
}

/******************************************************************************
* Function Name : ConsolePutChar
* Parameters    : [in] ch - character to be put on the console
* Description   : Puts the given character in the console
* Return Value  : NULL
******************************************************************************/

void ConsolePutChar(unsigned short ch)
{

	if (!Opened)
	{
		fprintf(stdout,"%c",ch);
		fflush(stdout);
		return;
	}
	if (isascii(ch))
	{
		fprintf(stdout,"%c",ch);
		fflush(stdout);
	}
}

/******************************************************************************
* Function Name : ConsoleBell
* Parameters    : NULL
* Description   : Puts a bell on the console
* Return Value  : NULL
******************************************************************************/
void ConsoleBell(void)
{
	ConsolePutChar(REX_KEY_BELL);
}

/******************************************************************************
* Function Name : CloseConsole
* Parameters    : NULL
* Description   : Closes a opened console
* Return Value  : NULL
******************************************************************************/
void CloseConsole(void)
{
	Opened = 0;
	tcsetattr( STDIN_FILENO, TCSANOW, &orgt );
}

/******************************************************************************
* Function Name : OpenConsole
* Parameters    : [in] rawmode - mode in which the console is to be opened
* Description   : Opens a console for the client(user) in the specific mode
* Return Value  : NULL
******************************************************************************/

void OpenConsole(int rawmode)
{
	struct termios newt;

	if (Opened == 1)
	{
		CloseConsole();
	}
	Opened = 1;
	RawConsole=rawmode;

	tcgetattr( STDIN_FILENO, &orgt);
	newt = orgt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
}


/******************************************************************************
* Function Name : ConsoleClear
* Parameters    : NULL
* Description   : Clears the console
* Return Value  : NULL
******************************************************************************/
void ConsoleClear(void)
{
	/* Clear Screen */
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	ConsolePutChar('2');
	ConsolePutChar('J');

	/* Position Cursor to top left */
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	ConsolePutChar('H');
}

/******************************************************************************
* Function Name : ConsoleIsKeyAvail
* Parameters    : NULL
* Description   : Checks whether the key stroke is available
* Return Value  : NULL
******************************************************************************/

unsigned char ConsoleIsKeyAvail(void)
{
	return Unix_kbhit();
}

/******************************************************************************
* Function Name : Termios_ConsoleGetChar
* Parameters    : NULL
* Description   : Gets the character from the console
* Return Value  : NULL
******************************************************************************/
static unsigned short Termios_ConsoleGetChar(void)
{
	int ch;
	int EscSeq;

	/* If Raw console, return it */
	if (RawConsole)
	{
		return (unsigned short)Unix_getch();
	}

	/* Non Raw Mode, Process Escape Sequence Keys */
	ch = Unix_getch();

	/* If normal Key return */
	if (ch != REX_KEY_ESCAPE)
	{
		/* Convert Carriage Return to NewLine */
		if (ch == REX_KEY_RETURN)
		{
			ch = REX_KEY_NEWLINE;
		}
		return ch;
	}

	/* Escape sequence should come immediatly */
	if (!Unix_kbhit())
	{
		return ch;
	}

	/* For Escape Sequence this should be '[' */
	/* If not we return the orginal character and ignore the new char */
	EscSeq = Unix_getch();

	/* Note: Some Terminals send some Keys as ESC O Sequence
                instead of ESC [ . So added a extra check */
	if ((EscSeq != REX_KEY_ESC_SEQ) && (EscSeq != 'O'))
	{
		return ch;
	}

	EscSeq = Unix_getch();

	/* Some Terminals send an extra ~ for some special keys
 	   Remove them here so that it will not be send to console */

	switch (EscSeq)
	{
		case 'D':
			return REX_KEY_LEFT;
		case 'C':
			return REX_KEY_RIGHT;
		case 'A':
			return REX_KEY_UP;
		case 'B':
			return REX_KEY_DOWN;

		case '5':
			Unix_getch();
			return REX_KEY_PGUP;

		case '6':
			Unix_getch();
			return REX_KEY_PGDN;

		case '1':		/* ESC [ 1 */
			Unix_getch();
			return 0;
		case 'H':		/* ESC O H */
			return REX_KEY_HOME;

		case '4':		/* ESC [ 4 */
			Unix_getch();
			return 0;
		case 'F':		/* ESC O F */
			return REX_KEY_END;

		case '2':
			return REX_KEY_INS;

		case '3':
			Unix_getch();
			return REX_KEY_DEL;

		case 'P':		/* ESC O P or ESC [ P */
			return REX_KEY_F1;
		case 'Q':		/* ESC O Q or ESC [ Q */
			return REX_KEY_F2;
		case 'R':		/* ESC 0 R or ESC [ R */
			return REX_KEY_F3;
		case 'S':		/* ESC 0 S or ESC [ S */
			return REX_KEY_F4;

		default:	/* Unknown Key */
			fprintf(stdout,"Unknown Esc Seq Code %d\n", ch);
			return 0;
	}
	return 0;
}



/******************************************************************************
* Function Name : ConsoleGetChar
* Parameters    : NULL
* Description   : Gets the character from the console
* Return Value  : NULL
******************************************************************************/
unsigned short ConsoleGetChar(void)
{
	return Termios_ConsoleGetChar();
}


/******************************************************************************
* Function Name : ConsoleCheckKey
* Parameters    : NULL
* Description   : Checks the key stroke in the console
* Return Value  : NULL
******************************************************************************/
unsigned short ConsoleCheckKey(void)
{
	if (!Unix_kbhit())
	{
		return 0;
	}
	return ConsoleGetChar();
}

/*
 Adding multiline editing feature is done using the following reference:
 - Referenced ANSI Terminal Emulation ESCape sequences for cursor movement
   Link : http://ascii-table.com/ansi-escape-sequences.php

 Note: 'ConsolePutChar(REX_KEY_BACKSPACE)'
        This piece of code won't behaves like 'backspace' KEY.
        It just moves the cursor one column backward.
*/

/******************************************************************************
* Function Name : ForwardCursor
* Parameters    : [in] column - number of columns
* Description   : Moves the cursor position forward to the specified
*                 no. of columns.
*                 Note: if the cursor is already at right end of a line,
*                       it won't move further
* Return Value  : NULL
******************************************************************************/
// Ansi Code : "ESC[nC" where n is number of position to move

inline static void ForwardCursor(int column)
{
	char tmp[26] ={0};
	int i = 0;
	sprintf(tmp,"%d",column);
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	while( tmp[i] != '\0')
	{
		ConsolePutChar(tmp[i++]);
	}
	ConsolePutChar('C');
}

/******************************************************************************
* Function Name : BackwardCursor
* Parameters    : [in] column - number of columns
* Description   : Moves the cursor position backward to the specified
*                 no. of columns.
*                 Note: if the cursor is already at left end of a line,
*                       it won't move further
* Return Value  : NULL
******************************************************************************/
// Ansi Code : "ESC[nD" where n is number of position to move

inline static void BackwardCursor(int column)
{
	char tmp[26] ={0};
	int i = 0;
	sprintf(tmp,"%d",column);
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	while( tmp[i] != '\0')
	{
		ConsolePutChar(tmp[i++]);
	}
	ConsolePutChar('D');
}

/******************************************************************************
* Function Name : MoveCursorOneLineUp
* Parameters    : NULL
* Description   : Moves up the cursor position to one line up
*                 Note: if the cursor is already at first line,
*                       it won't move further
* Return Value  : NULL
******************************************************************************/
// Ansi Code : "ESC[nA" where n is number of lines to move

inline static void MoveCursorOneLineUp()
{
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	ConsolePutChar('1');
	ConsolePutChar('A');
}

/******************************************************************************
* Function Name : MoveCursorOneLineDown
* Parameters    : NULL
* Description   : Moves down the cursor position to one line down
*                 Note: if the cursor is already at the last line,
*                       it won't move further
* Return Value  : NULL
******************************************************************************/
// Ansi Code : "ESC[nB" where n is number of lines to move

inline static void MoveCursorOneLineDown()
{
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	ConsolePutChar('1');
	ConsolePutChar('B');
}

/******************************************************************************
* Function Name : DelCharFromCursorToEndOfLine
* Parameters    : NULL
* Description   : Deletes the character from current cursor position
*                 to end of the line
* Return Value  : NULL
******************************************************************************/
// Ansi Code : "ESC[K"

inline static void DelCharFromCursorToEndOfLine()
{
	ConsolePutChar(REX_KEY_ESCAPE);
	ConsolePutChar('[');
	ConsolePutChar('K');
}

/******************************************************************************
* Function Name : EraseChar
* Parameters    : NULL
* Description   : Erases a character in the console
* Return Value  : NULL
******************************************************************************/

static void EraseChar(void)
{
	ConsolePutChar(REX_KEY_BACKSPACE);
	ConsolePutChar(REX_KEY_SPACE);
	ConsolePutChar(REX_KEY_BACKSPACE);
}

/******************************************************************************
* Function Name : EraseDelChar
* Parameters    : [in] cmdline - holds the command line
*                 [in] curIndex - holds the current index to delete
*                 [in] Index - holds the next index
* Description   : Erases the character pointed by the cursor position
* Return Value  : NULL
******************************************************************************/
//To make left and right cursor movement
static void EraseDelChar(char *CmdLine,
						 unsigned short curIndex,
						 unsigned short Index)
{
	unsigned short delIndex=curIndex;

	// To prevent buffer overflow
	if ( curIndex >= MAX_CMD_SIZE || Index >= MAX_CMD_SIZE )
		return;

	if (delIndex >= Index)
	{
		EraseChar();
		CmdLine[Index-1] = 0;
	}
	else if (delIndex < Index)
	{
		while (delIndex < Index)
		{
			CmdLine[delIndex] = CmdLine[delIndex+1];
			ConsolePutChar(CmdLine[delIndex]);
			delIndex++;
		}
		// erases the last char by giving 'space'
		ConsolePutChar(REX_KEY_SPACE);
		delIndex=Index-curIndex;

		// go back to cursor position by giving 'backspace'
		if (delIndex != 0)
		{
			ConsolePutChar(REX_KEY_BACKSPACE);
			delIndex--;
		}

		while (delIndex)
		{
			// when the cursor is at extreme left position of a line
			// (excluding the first line), then normal backspace won't
			// move the cursor to end of previous line.
			// So,do a line up & move the cursor to end of line.
			if (((curIndex + PROMPT_STR_LEN + delIndex) % g_ColumnLen) == 0)
			{
				MoveCursorOneLineUp();
				ForwardCursor(g_ColumnLen-1);
			}
			else
			{
				ConsolePutChar(REX_KEY_BACKSPACE);
			}
			delIndex--;
		}
	}
}

/******************************************************************************
* Function Name : EraseCmdLine
* Parameters    : [in] Index - holds the index to delete
* Description   : Erases the entire command line by each characters by
*                 character with the index
* Return Value  : NULL
******************************************************************************/

static void EraseCmdLine(unsigned short Index)
{
	// decrease the indicies and remove the characters
	// check whether the any cursor adjustments are needed if so
	// make those adjustments and delete
	while (Index--)
	{
		if (((Index + PROMPT_STR_LEN + 1) % g_ColumnLen) == 0)
		{
			MoveCursorOneLineUp();
			ForwardCursor(g_ColumnLen);
			DelCharFromCursorToEndOfLine();
		}
		else
		{
			EraseChar();
		}
	}
}

/******************************************************************************
* Function Name : PutCmdLine
* Parameters    : [in] cmdline - holds the command line
*                 [in] Index - holds the index to insert
*                 [in] curIndex - holds the current index
*                 [in] StartIndex - holds the starting index of the cmdline
*                 [in] delIndex - holds the original index position
* Description   : Puts the remaining command string in order to insert a
*                 letter in between the commands and moves back the cursor
*                 to its original position as specified by delIndex
* Return Value  : NULL
******************************************************************************/
// To make left and right cursor movement

void PutCmdLine(char *CmdLine,
				unsigned short Index,
				unsigned short curIndex,
				unsigned short StartIndex,
				unsigned short delIndex)
{
	int tmpCurIndex = curIndex ;

	curIndex += StartIndex;

	//puts the remaining letters
	while(curIndex<=Index)
	{
		ConsolePutChar(CmdLine[curIndex]);
		curIndex++;
	}

	if (delIndex != 0)
	{
		ConsolePutChar(REX_KEY_BACKSPACE);
		delIndex--;
	}

	// brings the cursor to its position
	// when bringing up the cursor check for cursor adjustments and make
	// changes accordingly
	while (delIndex)
	{
		// when the cursor is at left end of the a line
		// (excluding the first line), then normal backspace
		// won't move the cursor to end of previous line.
		// So,do a line up & move the cursor to end of line.
		if (((tmpCurIndex + PROMPT_STR_LEN + delIndex+1) % g_ColumnLen) == 0)
		{
			MoveCursorOneLineUp();
			ForwardCursor(g_ColumnLen-1);
		}
		else
		{
			ConsolePutChar(REX_KEY_BACKSPACE);
		}
		delIndex--;
	}
}

/******************************************************************************
* Function Name : ConsolePutStr
* Parameters    : [in] Str - holds the string
* Description   : Puts the String in the console
* Return Value  : NULL
******************************************************************************/
void ConsolePutStr(char *Str)
{
	while (*Str)
	{
		ConsolePutChar(*Str++);
	}
}


/******************************************************************************
 * Function Name : GetWindowSize
 * Parameters    : NULL
 * Description   : Gets the size of the windows and sets the global col length.
 * Return Value  : NULL
******************************************************************************/

void GetWindowSize(void)
{
	struct winsize ws;
	g_ColumnLen = 80;//default TERM column Length
	memset(&ws, 0, sizeof(struct winsize));
	//Set Column Length if ioctl success and ws.ws_col is positive and non-zero.
	if(!ioctl(1, TIOCGWINSZ, &ws) && ws.ws_col)
		g_ColumnLen = ws.ws_col;
}

/******************************************************************************
 * Function Name : HandleWindowResize
 * Parameters    : [in] signal - signal that is captured
 * Description   : Signal handler function that is used to capture the windows
 *                 resize signal and changes the windows size accordinlgy.
 * Return Value  : NULL
******************************************************************************/

void HandleWindowResize(int signal)
{
	GetWindowSize();
}

/******************************************************************************
* Function Name : GetCmdLine
* Parameters    : [in] CmdLine - holds the command line
*                 [in] Index - holds the current index
*                 [in] isPassword - flag representing the password
* Description   : Gets the entire command line string given by the user.
*                 Main module that gets the entire command and also adjusts
*                 the cursor according to key actions
* Return Value  : NULL
******************************************************************************/

unsigned short GetCmdLine(char *CmdLine, unsigned short Index, int isPassword)
{
	unsigned short ch = 0;
	unsigned short StartIndex = 0,curIndex = Index;

	// read the console char input from the user until
	// the user types "enter" button to exit
	while(1)
	{
		// get the char from the console
		ch = ConsoleGetChar();

		//Added for home key
		// home key is pressed so go to begining of the line
		if (ch == REX_KEY_HOME)
		{
			while (curIndex)
			{
				// when the cursor is at starting position of the next line,
				// then normal backspace won't move the cursor to end of
				// previous line. So, do a line up & move the cursor to
				// end of line.
				if (((curIndex + PROMPT_STR_LEN) % g_ColumnLen) == 0)
				{
					MoveCursorOneLineUp();
					ForwardCursor(g_ColumnLen - 1);
				}
				else
				{
					ConsolePutChar(REX_KEY_BACKSPACE);
				}
				curIndex--;
			}
			continue;
		}

		//Added for End key
		// end key is pressed so go to end of the line
		if(ch == REX_KEY_END)
		{
			PutCmdLine(CmdLine, Index, curIndex, StartIndex, 0);
			curIndex = Index - StartIndex;
			continue;
		}
		
		/* Check if any history keys are pressed */
		/*
		if ((ch == REX_KEY_UP) ||
			(ch == REX_KEY_DOWN) ||
			(ch == REX_KEY_PGUP) ||
			(ch == REX_KEY_PGDN)) {

			if(curIndex != Index) {
				while(curIndex<=Index)
				{
					ConsolePutChar(CmdLine[curIndex]);
					curIndex++;
				}
			}
			EraseCmdLine(Index);
			
			if ( Index >= (MAX_CMD_SIZE-1) )
				return 0;

			CmdLine[Index++]= 0;
			return ch;
		}
		*/

		/* If escape, clear the line */
		if (ch == REX_KEY_ESCAPE)
		{
			if (StartIndex != 0)
			{
				continue;
			}

			if (curIndex != Index)
			{
				while(curIndex<=Index)
				{
					ConsolePutChar(CmdLine[curIndex]);
					curIndex++;
				}
			}
			EraseCmdLine(Index);
			Index = 0;
			curIndex = 0;
			continue;
		}
		
		//To perform left cursor movement 
		// Moves the cursor left if left arrow key is pressed
		if (ch == REX_KEY_LEFT)
		{
			if ((Index > StartIndex) && (curIndex != 0))
			{
				// if the cursor is at extreme left end of a line
				// (excluding the first line), then normal backspace
				// won't move the cursor to end of previous line.
				// So, do a line up & move the cursor to end of line.
				if (((curIndex + PROMPT_STR_LEN ) % (g_ColumnLen))  == 0)
				{
					MoveCursorOneLineUp();
					ForwardCursor(g_ColumnLen -1);
				}
				else
				{
					ConsolePutChar(REX_KEY_BACKSPACE);
				}
				curIndex--;
			}
			continue;
		}

		// Moves the cursor right if right arrow key is pressed
		if (ch == REX_KEY_RIGHT)
		{
			if (Index > (curIndex + StartIndex))
			{
				// if the cursor is at extreme right end of a line,
				// do a line down & move the cursor to begining of a line.
				if (((curIndex + PROMPT_STR_LEN + 1) % g_ColumnLen) == 0)
				{
					MoveCursorOneLineDown();
					BackwardCursor(g_ColumnLen - 1);
				}
				else
				{
					ForwardCursor(1);
				}
				curIndex++;
			}
			continue;
		}

		//If Backspace erase a character from screen and from buffer
		//To add backspace facility on ssh.
		if ((ch == REX_KEY_BACKSPACE) ||
			(ch == SSH_BACKSPACE))
		{
			if ((Index > StartIndex) && (curIndex != 0))
			{
				// when the cursor is at middle of a command
				if ((curIndex + StartIndex) != Index)
				{
					// if the cursor is at extreme left end of a line
					// (excluding the first line), then do a line up & move
					// the cursor to end of line.
					if (((curIndex + PROMPT_STR_LEN) % g_ColumnLen) == 0)
					{
						MoveCursorOneLineUp();
						ForwardCursor(g_ColumnLen -1);
					}
					else
					{
						ConsolePutChar(REX_KEY_BACKSPACE);
					}
					EraseDelChar(CmdLine,(curIndex-1+StartIndex),Index);
				}
				else
				{
					// when the cursor is at end of a command
					// if the cursor is at extreme right end of a line
					// (excluding the first line), then "EraseChar" function
					// can't be used to delete the char in prev line.
					// So, do a line up & move the cursor to end of line &
					// delete the character.
					if (((Index + PROMPT_STR_LEN) % g_ColumnLen) == 0)
					{
						MoveCursorOneLineUp();
						ForwardCursor(g_ColumnLen -1);
						DelCharFromCursorToEndOfLine();
					}
					else
					{
						EraseChar();
					}

					if ( Index > MAX_CMD_SIZE )
				                return 0;

					CmdLine[Index-1]=0;
				}
				curIndex--;
				Index--;
			}
			else
			{
				ConsoleBell();
			}
			continue;
		}

		// Deletes the cursor position character
		if(ch == REX_KEY_DEL)
		{
			if ((Index > StartIndex) && (curIndex != Index))
			{
				EraseDelChar(CmdLine, curIndex+StartIndex, Index);
				if (Index == (StartIndex+curIndex))
				{
					curIndex--;
				}	
				Index--;
			}
			continue;
		}

		/* If Tab , Insert Spaces for Tab */
		if (ch == REX_KEY_TAB)
		{
			//TODO for TabSupported Features
			curIndex = Index;
			continue;
		}

		// if newline, end of input
		if (ch == REX_KEY_NEWLINE)
		{
			// If a \ preceds the newline, then it is line continuation */
			if (Index > StartIndex)
			{
				if (CmdLine[Index-1] == '\\')
				{
					if(Index == 1)
					{
						EraseChar();
						Index=0; 
						curIndex=0;
						continue;
					}
					StartIndex = --Index;
					curIndex--;
					curIndex += StartIndex;
					if(curIndex != Index)
					{
						while(curIndex<=Index)
						{
							ConsolePutChar(CmdLine[curIndex]);
							curIndex++;
						}
					}
					curIndex = 0;
					ConsolePutChar(REX_KEY_NEWLINE);
					continue;
				}
			}

			if ( Index >= (MAX_CMD_SIZE-1) )
                                    return 0;

			CmdLine[Index++] = 0;		// Terminate String
			ConsolePutChar(REX_KEY_NEWLINE);
			// Put newlines depending upon the no. of lines the
			// characters are entered in.
			// Note: The current cursor position may be at
			// start/end/middle of ANY line.
			{
				int temp1 = 0 , temp2 = 0 , temp3 = 0 ;
				temp1 = (Index + PROMPT_STR_LEN) / g_ColumnLen ;
				temp2 = (curIndex + PROMPT_STR_LEN) / g_ColumnLen;
				temp3 = temp1 - temp2;
				while (temp3--)
				{
					ConsolePutChar(REX_KEY_NEWLINE);
				}
			}
			break;
		}


		/* Ignore all other non printable and control characters*/
		if (ch & 0xFF00)
		{
			ConsoleBell();
			continue;
		}

		if (!isprint(ch))
		{
			ConsoleBell();
			continue;
		}

		// For all other normal characters, If Line len is less than
		// the maximum len put the character into the string
		if (Index != LINE_LEN)
		{
			int i;
			curIndex += StartIndex;
			if(Index != curIndex)
			{
				for(i=Index;i>=curIndex;i--)
				{
					CmdLine[i+1]=CmdLine[i];
				}
			}
			CmdLine[curIndex++] = (char)(ch &0xFF);
			
			Index++;
			CmdLine[Index] = 0;
			curIndex -= StartIndex;
			if(Index > curIndex+StartIndex)
			{
				PutCmdLine (CmdLine, Index-1, curIndex-1,
					StartIndex, Index-(curIndex+StartIndex));
			}
			else
			{
				if(isPassword)
				{
					ConsolePutChar('*');
				}
				else
				{
					ConsolePutChar(ch);
					// Printing a character at the right end of line will
					// not blink/move the cursor to next line. To do this,
					// just give a space & move the cursor back.
					if (((curIndex + PROMPT_STR_LEN) % g_ColumnLen) == 0)
					{
						fprintf(stdout," ");
						fflush(stdout);
						BackwardCursor(1);
					}
				}
			}
		}
		else
		{
			ConsoleBell();
		}
	}	/* while (1) */
	return 0;
}

int main()
{
    char username[MAX_CMD_SIZE] = {0}, password[MAX_CMD_SIZE] = {0};

    // open a new console for the user
    OpenConsole(0);

    printf("\nUsername : ");  
    GetCmdLine(username, 0, 0);

    printf("\nPassword : ");     
        GetCmdLine(password, 0, 1);  

    printf("\nOUTPUT :- \nUSERNAME = %s\nPASSWORD = %s\n", username, password);
    // close the current console and restores back the default console
    CloseConsole();

    return 0;
}

