/*********************************************************************
* 02/22/95															 *
* All-Day Audio Recorder main module.								 *
*																	 *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*********************************************************************/

#include "AdarPch.h"

#include "armadillo.h"
#include "alarmclock.h"

// Scroll Timer
#define TimerID1		1				// Timer ID
#define TimerValue1 	50				// Value in ms (Not used. See ScrollTime variable)

// Clock refresh timer
#define TimerID2		2				// Timer ID
#define TimerValue2 	(1/20)			// Value in ms (20 Hz refresh rate)

// Blink timer
#define TimerID3		3				// Timer ID
#define TimerValue3 	180 			// Value in ms

char *lpszCmdParm;						// Value of lpszCmdLine from WinMain

CVersion Version;
CString FileName, IndexFileName;

// .INI file fields (initlized by ReadIniFile)
char IniName[] = "SCANREC.INI";			// name of .INI file
char IniSectionName[] = "INIT"; 		// section name

int SampleRate; 						// Sample rate used in WAVEFORMAT
char SampleRateName[] = "SampleRate";	// name in .INI file

int AudioRes; 								// 0 = 8 bits, 1 = 16 bits
char AudioResName[] = "AudioResolution";	// name in .INI file

int AudioChan; 								// 0 = Mono, 1 = Stereo
char AudioChanName[] = "AudioChannels";		// name in .INI file

int ScrollTime; 						// Scroll delay
char ScrollTimeName[] = "ScrollTime";	// name in .INI file

int SquelchThresh;
char SquelchThreshName[] = "Squelch";

int LogScrollOff;
char LogScrollOffName[] = "LogScroll";	// Keeps log display from autoscrolling

UINT xScreenSize, yScreenSize;			/* Physical Screen Size */

double PercentFree = 1; 				// Free space in percent	5% = .05
BOOL AbortEnable = TRUE;				// prevent mult triggger if full

// The positions in this table must agree with the combo box items
struct _SampleTable
{
	char*	str;
	int 	value;
} SampleTable[] = 
{
	"8,000",	8000,
	"11,025",	11025,
	"22,050",	22050,
	"32,000",	32000,
	"44,100",	44100,
	"48,000",	48000
};

HWND hwndd; 				// Window handle of recorder control dialog box
HWND hwnddx;				// Window handle of index modeless dialog box
HMENU hmenu;				// dialog box's menu handle
RECT dlgrect;				// represents dialog box client area
RECT dlgpict;				// represents the picture area of the dialog box
RECT dlgreclite;			// rectangle of record indicator light
RECT dlgsqllite;			// rectangle of squelch status light
RECT dlgclock;				// digital clock area (total record time)
HANDLE hdlgclock;			// handle of main clock
RECT dlgclockrec;			// digital clock area (data record time)
HANDLE hdlgclockrec;		// handle of above clock
RECT dlgtod;				// Time of Day clock
HANDLE hdlgtod; 			// handle of above clock
HBITMAP henvlbm;			// envelope bitmap
RECT envlrect;				// rectangle of envelope bitmap
HDC hdcenvl;				// used for manipulating the envelope bitmap
HDC hdcBackground;			// hdc used to hold background bitmap
HACCEL haccel;				// accelerator handle
MSG msg;					// Provides global access to current msg

const char WindowTitle[] = {"AdarMainWindow"};

// Sweep envelope display
int  SweepMarker = 0;									// position of marker (0 - n-1)
BOOL SquelchMode = FALSE;								// Show squelch graphic if true

// Main Window
char szMyClass[] = "adarClass"; 		/* Window Class Name */
WNDCLASS wc;							/* Global Window Class */
HINSTANCE hinst;						/* Application's handle? */
HWND hwnd;								/* The Main Window Handle */
HWND hwndPrev;							/* Handle of original instance of this window */
HWND hwndFilter = NULL; 				// filter thread's window handle (see FILTER_READY message)

extern HANDLE hwavein;					// waveform input device handle
extern HANDLE hwaveout; 				// waveform output device handle

UINT EnvlPos = 128; 					// Positive envelope limit calculated for each buffer
UINT EnvlNeg = 128; 					// Negitive envelope limit calculated for each buffer

Queue ReadyQueue;						// Holds RECBUFFs ready for waveInAddBuffer

CFile TraceFile;						// For debugging purposes
CString TraceFileName = "Scanrec.log";	// Debug Trace file
BOOL TraceFileActive;

// Button bitmap handles
HANDLE hFileButton;
HANDLE hRecordButton;
HANDLE hStopButton;
HANDLE hCloseButton;
HANDLE hExitButton;
HANDLE hPanel;
HBRUSH hSliderClr;

// Bug fix for David Madison's Win95A system Kernel32.dll not having GetDiskFreeSpaceEx export.
HINSTANCE hLib;						// Instance handle of kernel32.dll
LPGetDiskFreeSpaceExA lpGetDiskFreeSpaceExA;

Armadillo	ArmLib;
CString		NewUserName;
CString		NewUserKey;

CAlarmClock	TBClock;
///////////////////////////////////////////////////////////////////////////////

int PASCAL WinMain(HINSTANCE hinstCurrent, HINSTANCE hinstPrevious,
	LPSTR lpszCmdLine, int nCmdShow)
{
	TraceFileInit();
	hinst = hinstCurrent;				/* save current instance handle */

	/* If there is another instance, just activate the original window. */
	if ((hwndPrev = FindWindow(szMyClass, WindowTitle)) != NULL)
	{
		SetForegroundWindow(hwndPrev);	/* activate the original instance's window */
		return 0;					// exit, another instance is already active
	}

	// Save the address of the line to provide global access to it from all functions
	lpszCmdParm = lpszCmdLine;

	// Get values from .INI file
	ReadIniFile();

	/* Register the window class. */
	wc.style		 = CS_BYTEALIGNCLIENT;
	wc.lpfnWndProc	 = MyWndProc;
	wc.cbClsExtra	 = 0;
	wc.cbWndExtra	 = 0;
	wc.hInstance	 = hinst;
	wc.hIcon		 = LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON3)); 
	wc.hCursor		 = NULL;
	wc.hbrBackground = (struct HBRUSH__ *) GetStockObject(HOLLOW_BRUSH);
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szMyClass;

/* Get maximum screen size */

	xScreenSize = GetSystemMetrics(SM_CXSCREEN);
	yScreenSize = GetSystemMetrics(SM_CYSCREEN);

/* Create and show the window. */

	if (!RegisterClass(&wc)) return FALSE;

	hwnd = CreateWindow(								/* create the parent window */
//		WS_EX_TOPMOST,									/* Extended Style */
		szMyClass,										/* address of registered class name */
		WindowTitle,									/* address of window text */
		WS_OVERLAPPED | WS_SYSMENU, 					/* window style */
		//CW_USEDEFAULT, CW_USEDEFAULT,					/* pos */
		0, 0,											/* pos */
		CW_USEDEFAULT, CW_USEDEFAULT,					/* size */
		NULL,											/* handle of parent window */
		NULL,											/* handle of menu or child-window identifier */
		hinst,											/* handle of appl instance */
		NULL											/* address of window-creation data */
	);

	hPanel = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP_PANEL));	// Load the panel bitmap
	hSliderClr = CreateSolidBrush(RGB(0,32,64));

	//////////////////////////////////////////////////////////////////
 	memset(&calSettings, 0, sizeof(CAL_SETTINGS));
 	calSettings.calMode = IDC_RADIO_CAL_OFF;

	// Write system version info to the trace file.
	OSVERSIONINFO os = {0};	// To stuff details into the trace file
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os);
	trace("\n");
	trace("OS Version Information\n");
	trace("dwMajorVersion:\t%08X\n",	(DWORD) os.dwMajorVersion);
	trace("dwMinorVersion:\t%08X\n",	(DWORD) os.dwMinorVersion);
	trace("dwBuildNumber:\t%08X\n",		(DWORD) os.dwBuildNumber);
	trace("szCSDVersion:\t%sX\n",		(LPCTSTR) os.szCSDVersion);

	hLib = LoadLibrary("KERNEL32");
	if (hLib) lpGetDiskFreeSpaceExA = (LPGetDiskFreeSpaceExA) GetProcAddress(hLib, "GetDiskFreeSpaceExA");
	//////// Add more pre-dialog initialization here.

//	Create the control dialog box
	hwndd = CreateDialog(hinst, MAKEINTRESOURCE(IDD_DIALOG_MAIN_SMALL), NULL, (DLGPROC) DialogProc);
	hmenu = GetMenu(hwndd); 							// get menu handle for later use

	ShowWindow(hwnd, SW_HIDE);							// hide the main window

	haccel = LoadAccelerators(hinst, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	SYSTEMTIME	tm = {2011, 05, 0, 12, 6, 6, 6, 0};
	TBClock.SetAlarm(tm, TB);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (TranslateAccelerator(hwndd, haccel, &msg))
		{
			trace("Accelerator Translated. HWND = %08X\n", (HWND) msg.hwnd);
		}
		else // not accelerator message
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	DestroyAcceleratorTable(haccel);
	
	DestroyWindow(hwnd);	/* main window */

	WriteIniFile(); 		// Save settings

	if (hLib) FreeLibrary(hLib);

	trace("Program Termination Complete\n");

   return (int) msg.wParam;    /* return value of PostQuitMessage */
} /* End WinMain */




/*********************************************************************
* Main windows procedure:											 *
*********************************************************************/

LRESULT CALLBACK MyWndProc(HWND hwnd, UINT pmsg, WPARAM wParam, LPARAM lParam)
{
	BOOL fEndSession;     // end-session flag 
	BOOL fLogOff;         // logoff flag 

	switch(pmsg)
	{
	
		case WM_ENDSESSION:
		{
			fEndSession = (BOOL) wParam;	// end-session flag 
			fLogOff =  (BOOL) lParam;		// logoff flag 

			if (fEndSession)
			{
				if (ButtonState == RECORDING)
					RecordStop();
				if (ButtonState == PAUSED)
					CloseRecordFile();
				return 0;
			}
			// else let DefWindowProc handle it.
		}

		case WM_CREATE:
		{
			//InitFilterThread(hwnd);		  // start up the filter thread. Thread will self-terminate if
//											   it is sent a FILTER_KILL message
			return 0;						/* tell windows to proceed */
		}
		
		case WM_DESTROY:
		{

			return 0;
		}

		case MM_WIM_OPEN:
		{
			AbortEnable = TRUE;
			PercentFree = 1;
			trace("Recording device open.\n");
			return 0;
		}

		case MM_WIM_CLOSE:
		{
			trace("The waveform device is now closed.\n");
			return 0;
		}

		case MM_WIM_DATA:
		{
			HANDLE hwave;
			WAVEHDR *wavehdr;

			hwave = (HANDLE) wParam;			// device handle
			wavehdr = (WAVEHDR *) lParam;

			if (AbortEnable)
			{
				if (PercentFree <= .02)
				{	// Full disk. Abort the recording
					AbortEnable = FALSE;
					SendMessage(hwndd, WM_COMMAND, IDC_STOP, 0);
					MessageBox(hwndd, "Recording paused. Disk nearly full.", "", MB_OK|MB_ICONEXCLAMATION);
				}
			}

			DataInMessage(hwndFilter, hwave, wavehdr, ReadyQueue);				// Pass to further processing
			return 0;
		}

		case FILTER_READY:
		{
			trace("FILTER_READY message received.\n");
			hwndFilter = (HWND) lParam; 	// get handle of filter window
			return 0;
		}

		case FILTER_CLOSED:
		{
			trace("FILTER_CLOSED message received.\n");
			hwndFilter = NULL;				// nullify filter window handle
			PostQuitMessage(0); 			// tell main loop to break
			return 0;
		}

		case FILTER_RECYCLE_OUT:
		{
//			trace("FILTER_RECYCLE_OUT message received, RB= %08lX.\n", lParam);
			RecycleRecbuff((RECBUFF *) lParam, ReadyQueue); 	// recycle the used buffer
			return 0;
		}

		default:
		{
//			trace("Unknown: #%04X, wParam=%04X, lParam=%08lX\n", msg, wParam, (LONG) lParam);
			break;
		}
		
	} /* End switch */

	return DefWindowProc(hwnd,pmsg,wParam,lParam);
}


/*********************************************************************
*				Dialog Box Procedure								 *
*********************************************************************/

BOOL CALLBACK DialogProc(HWND hDlg, UINT pmsg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	static HBITMAP hbmsave; // bitmap save (static prevents C4700 Warning message)
	WORD wNotifyCode;		 // notification code 
	WORD wID;				 // item, control, or accelerator identifier 
	HWND hwndCtl;			 // handle of control 
//	struct tm t;			// used to convert samples into time
	TM tm;					// replacement for above structure
	static int divider; 	// interval divider for percent indicator
	CString str;

//	trace("Dialogmsg: #%04X, wParam=%04X, lParam=%08lX\n", msg, wParam, (LONG) lParam);

	switch (pmsg)
	{

		case WM_INITDIALOG:
		{
			// Center the dialog box
			CRect rect;
			GetWindowRect(hDlg, &rect);

			int xc = xScreenSize / 2;
			int yc = yScreenSize / 2;

			// Prevent unwanted system menu commands
			HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
			_ASSERT(hSysMenu);
			EnableMenuItem(hSysMenu, SC_MAXIMIZE, MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
			EnableMenuItem(hSysMenu, SC_CLOSE,	  MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);
			EnableMenuItem(hSysMenu, SC_SIZE,	  MF_DISABLED|MF_GRAYED|MF_BYCOMMAND);

			hdc = GetDC(hDlg);
			GetClientRect(hDlg, &dlgrect);	  // initialize the dlg client rectangles
			GetDialogIDRect(hDlg, &dlgpict, 	IDC_WAVE);
			GetDialogIDRect(hDlg, &dlgreclite,	IDC_RECORD_LIGHT);
			GetDialogIDRect(hDlg, &dlgsqllite,	IDC_SQUELCH_LIGHT);
			GetDialogIDRect(hDlg, &dlgclock,	IDC_CLOCK);
			GetDialogIDRect(hDlg, &dlgclockrec, IDC_CLOCK2);
			GetDialogIDRect(hDlg, &dlgtod,		IDC_TOD1);

// Create the envelope display bitmap
//			SetRect(&envlrect, 0, 0, 128, 256);    // set size
			CopyRect(&envlrect, &dlgpict);			// make bitmap same size as picture area rectangle
			OffsetRect(&envlrect, -envlrect.left, -envlrect.top);	 // move to zero origin

// Create memory DC for envelope display
			hdcenvl = CreateCompatibleDC(hdc);

// Create Envelope Bitmap
			if ((henvlbm = CreateCompatibleBitmap(hdc, envlrect.right-envlrect.left, envlrect.bottom-envlrect.top)) == NULL)
				DebugBreak();
			hbmsave = (struct HBITMAP__*) SelectObject(hdcenvl, henvlbm);	// select our bitmap

// Paint the bitmap background
			PatBlt(hdcenvl, envlrect.left, envlrect.top,
				(envlrect.right-envlrect.left), (envlrect.bottom-envlrect.top), BLACKNESS);

			InvalidateRect(hDlg, &dlgpict, FALSE);	// redraw envelope display

// Set dialog box's icon to default
			SetClassLong(hDlg, GCL_HICON, (LONG) (LONG_PTR) LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON3)));

// start the timers, send messages to dlg window
			SetTimer(hDlg, TimerID1, ScrollTime, NULL);
			SetTimer(hDlg, TimerID2, TimerValue2, NULL);	// Start the clock refresh timer
//			SetTimer(hDlg, TimerID3, TimerValue3, NULL);

// Initialize the clock displays
			InitClock(hinst, hdcenvl);
			hdlgclock = CreateClock(hDlg, &dlgclock, 6);
			InvalidateRect(hDlg, &dlgclock, FALSE);
			hdlgclockrec = CreateClock(hDlg, &dlgclockrec, 6);
			InvalidateRect(hDlg, &dlgclockrec, FALSE);
			hdlgtod = CreateClock(hDlg, &dlgtod, 6);
			InvalidateRect(hDlg, &dlgtod, FALSE);

// Initialize the squelch control

			HWND hS = GetDlgItem(hDlg, IDC_SLIDER_SQUELCH);
			_ASSERT(hS);
			SendMessage(hS, TBM_SETRANGE, TRUE, (LPARAM) MAKELONG(0, 100));
			SendMessage(hS, TBM_SETPAGESIZE, 0, (LPARAM) 10);
			SendMessage(hS, TBM_SETTICFREQ, (WPARAM) 10, (LPARAM) 0);
			SetSquelchSlider(hDlg, SquelchThresh);
			SetDlgItemInt(hDlg, IDC_STATIC_SQ_VAL, SquelchThresh, TRUE);

			ReleaseDC(hDlg, hdc);

			SetWindowText(hDlg, "Recorder Idle");

			// Set up the speed combo box
			SetSpeedCombo(hDlg);

			// Set sample rate indicator
			for(int i = 0, val = -1; i < (sizeof(SampleTable) / sizeof(SampleTable[0])); i++)
			{
				if (SampleRate == SampleTable[i].value) val = i;
			}

			if (val == -1)
			{
				val = 0; // Set to default if invalid value
				SampleRate = SampleTable[val].value;
			}

			SendDlgItemMessage(hDlg, IDC_COMBO_SMP_RATE, CB_SETCURSEL, val, 0);

			// Set the resolution and channels radio buttons.
			CheckRadioButton(hDlg, IDC_RADIO_8BIT, IDC_RADIO_16BIT, AudioRes ? IDC_RADIO_16BIT : IDC_RADIO_8BIT);
			CheckRadioButton(hDlg, IDC_RADIO_MONO, IDC_RADIO_STEREO, AudioChan ? IDC_RADIO_STEREO : IDC_RADIO_MONO);

			// Setup the bitmap buttons.
			hFileButton = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP_FILEBUTTON));
			SendMessage(GetDlgItem(hDlg, ID_DEVICE_OPENWAVE), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hFileButton);
			SendMessage(GetDlgItem(hDlg, IDC_BUTTON_FILESETTINGSDLG), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hFileButton);

			hRecordButton = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP_RECORDBUTTON));
			SendMessage(GetDlgItem(hDlg, IDC_RECORD), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hRecordButton);

			hStopButton = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP_STOPBUTTON));
			SendMessage(GetDlgItem(hDlg, IDC_STOP), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hStopButton);

			hCloseButton = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP_CLOSEBUTTON));
			SendMessage(GetDlgItem(hDlg, ID_DEVICE_CLOSEWAVE), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hCloseButton);

			hExitButton = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP_EXITBUTTON));
			SendMessage(GetDlgItem(hDlg, ID_FILE_EXIT), BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hExitButton);

  			// Setup the DC offset mode switches
		 	GetPrivateProfileStruct("Init", "Calibration", &calSettings, sizeof(CAL_SETTINGS), IniName);
  			CheckRadioButton(hDlg, IDC_RADIO_CAL_OFF, IDC_RADIO_CAL_LOCK, calSettings.calMode);			return TRUE;

		}

		case WM_DESTROY:
		{
			trace("WM_DESTROY (DLG)\n");

			SelectObject(hdcenvl, hbmsave); 	// select original bitmap
			DeleteObject(henvlbm);				// delete envelope bitmap
			DeleteDC(hdcenvl);					// delete memory device context for envelope

			KillTimer(hDlg, TimerID1); /* kill scroll timer */
			KillTimer(hDlg, TimerID2); /* kill clock refresh timer */
//			KillTimer(hDlg, TimerID3); /* kill clock blink timer */

// Cleanup the clock display subsystem
			DeleteClock(hdlgclock);
			DeleteClock(hdlgclockrec);
			DeleteClock(hdlgtod);
			ClockCleanup();

			DeleteObject(hFileButton);
			DeleteObject(hRecordButton);
			DeleteObject(hStopButton);
			DeleteObject(hCloseButton);
			DeleteObject(hExitButton);
			DeleteObject(hPanel);
			DeleteObject(hSliderClr);

			return FALSE;	// let default handler process this one
		}

		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORSTATIC:
		{
			return FALSE;
			SetBkMode((HDC) wParam, TRANSPARENT);

			if (GetDlgItem(hDlg, IDC_SLIDER_SQUELCH) == (HWND) lParam) return (BOOL) (INT_PTR) hSliderClr;
			if (GetDlgItem(hDlg, IDC_STATIC_SQ_VAL) == (HWND) lParam) return (BOOL) (INT_PTR) GetStockObject(WHITE_BRUSH);
			if (GetDlgItem(hDlg, IDC_EDIT_OFFSETMEAS) == (HWND) lParam) return (BOOL) (INT_PTR) GetStockObject(WHITE_BRUSH);
			if (GetDlgItem(hDlg, IDC_EDIT_OFFSETMEAS2) == (HWND) lParam) return (BOOL) (INT_PTR) GetStockObject(WHITE_BRUSH);
			if (GetDlgItem(hDlg, IDC_EDIT_OFFSETMEAS3) == (HWND) lParam) return (BOOL) (INT_PTR) GetStockObject(WHITE_BRUSH);
			if (GetDlgItem(hDlg, IDC_EDIT_OFFSETMEAS4) == (HWND) lParam) return (BOOL) (INT_PTR) GetStockObject(WHITE_BRUSH);
			if (GetDlgItem(hDlg, IDC_EDIT_FREE) == (HWND) lParam) return (BOOL) (INT_PTR) GetStockObject(WHITE_BRUSH);

			return (BOOL) (INT_PTR) GetStockObject(NULL_BRUSH);
		}

		case WM_TIMER:
		{

			NANOBEGIN

			switch (wParam)
			{
				case TimerID1:
				{
					if (DevState == DEV_REC || DevState == DEV_PLY)
					{
						SweepEnvl();
					}
					return TRUE;
				}

				case TimerID2:
				{ // Update the digital clock display
					
					SetClock(hdlgclock, ConvertSamplesToTime6(SampleCount, SampleRate, &tm), FALSE);
					SetClock(hdlgclockrec, ConvertSamplesToTime6(SampleStoredCount, SampleRate, &tm), FALSE);
//					SetClock(hdlgtod, GetMsgTime(&tm, &msg), FALSE);
					SetClock(hdlgtod, GetSystemTime6(&tm), FALSE);

					// Update the free space indicator
					if (divider == 0)
					{
						divider = 5;   // reset divider
						CheckFreeSpace();

						DWORD hours = 0, minutes = 0;
						BOOL rc;
						static CString str;
						HWND hTitle;

						hTitle = GetDlgItem(hDlg, IDC_STATIC_FREE);

						rc = GetRemainingTimeOnDrive(&hours, &minutes);
						if (rc)
						{
							str.Format(_T("%d Hrs %02d Min"), hours, minutes);
							SetDlgItemText(hDlg, IDC_EDIT_FREE, str);
						}
						else
						{
							str.Format("%02.1f Percent", PercentFree*100);
							SetDlgItemText(hDlg, IDC_EDIT_FREE, str);
						}
					}
					else --divider;

					return TRUE;
				}

				case TimerID3:
				{ // Make things blink
					;
					return TRUE;
				}
			}

			NANOEND
		}

		case WM_PAINT:
		{
			HDC hdc;
			RECT urect; 		// update rectangle
			RECT irect; 		// intersection of update rect and bitmap area (excludes lights)
			RECT orect; 		// results of offsetting intersect rect to the envelope rect coords

//			trace("WM_PAINT (DLG)\n");

			hdc = GetDC(hDlg);

// repaint the update rectangle (which should only enclose part of the bitmap)
			GetUpdateRect(hDlg, &urect, TRUE);

			IntersectRect(&irect, &urect, &dlgpict);	// exclude every rect except the picture area from update rect

			if (!IsRectEmpty(&irect)) // repaint bitmap only if not empty
			{
				CopyRect(&orect, &irect);
				OffsetRect(&orect, -dlgpict.left, -dlgpict.top);	// convert client coords to envlrect coords
				BitBlt(hdc, irect.left, irect.top, (irect.right-irect.left), (irect.bottom-irect.top),
					hdcenvl, orect.left, orect.top, SRCCOPY);
				ValidateRect(hDlg, &irect); 						// validate just the small area
			}

			if (SquelchMode) DrawSquelchGraphic();

// Update the clock displays
			RefreshClock(hdlgclock);
			RefreshClock(hdlgclockrec);
			RefreshClock(hdlgtod);

// Update the squelch light if necessary
			IntersectRect(&irect, &urect, &dlgsqllite); // check to see if it needs updating
			if (!IsRectEmpty(&irect)) // repaint bitmap only if not empty
			{	// paint the squelch light
				if (SquelchState == SQUELCHED)
					PaintDlgID(hDlg, IDC_SQUELCH_LIGHT, RGB(32, 0, 0));  // Make light Dark Red
				else
					PaintDlgID(hDlg, IDC_SQUELCH_LIGHT, RGB(255, 0, 0)); // Make light Bright Red
			
				ValidateRect(hDlg, &dlgsqllite);
			}

			IntersectRect(&irect, &urect, &dlgreclite); // check to see if it needs updating
			if (!IsRectEmpty(&irect)) // repaint bitmap only if not empty
			{
				if (ButtonState == RECORDING)
					PaintDlgID(hDlg, IDC_RECORD_LIGHT, RGB(255, 0, 0));    // Make RECORD light red
				else
					PaintDlgID(hDlg, IDC_RECORD_LIGHT, RGB(32, 0, 0));	  // Make RECORD dark red
				ValidateRect(hDlg, &dlgreclite);	// validate the record light
			}

			// Cosmetic borders
			DrawBorder(hdc, dlgpict);
			DrawBorder(hdc, dlgclock);
			DrawBorder(hdc, dlgclockrec);
			DrawBorder(hdc, dlgtod);
			DrawBorder(hdc, dlgreclite);
			DrawBorder(hdc, dlgsqllite);

			ReleaseDC(hDlg, hdc);

			return FALSE;						// FALSE allows windows to repaint other window objects
		}

		case WM_COMMAND:
		{

			NANOBEGIN

			wNotifyCode = HIWORD(wParam); // notification code 
			wID = LOWORD(wParam);		  // item, control, or accelerator identifier 
			hwndCtl = (HWND) lParam;	  // handle of control 

			//trace("WM_COMMAND: msg=%X, wParam=%X, lParam=%X\n", pmsg, wParam, lParam);
			wParam = wParam % 0x10000;	// translate accelerator messages
			switch (wID)
			{
				case ID_OPTIONS:
				{
					DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG_OPTIONS), hDlg, OptionsDlgProc);

					return TRUE;
				}

				case ID_FILE_EXIT:
				{
					if (IsWindowEnabled(GetDlgItem(hwndd, (int) ID_FILE_EXIT)))
					{
						quit();
					}
					return TRUE;
				}

				case ID_DEVICE_OPENWAVE:
				{
					if ((GetMenuState(hmenu, ID_DEVICE_OPENWAVE, MF_BYCOMMAND) & MF_DISABLED) == 0) // if not disabled
					{
						trace("Open-Record Menu Command\n");

						SECUREBEGIN_C

						RecordInit(ReadyQueue);

						SECUREEND_C

					}
					return TRUE;
				}

				case IDC_BUTTON_FILESETTINGSDLG:
				{
					LRESULT rc = DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG_FILESETTINGS), hDlg, FileSettingsProc);
					return TRUE;
				} 

				case ID_DEVICE_CLOSEWAVE:
				{
					if ((GetMenuState(hmenu, ID_DEVICE_CLOSEWAVE, MF_BYCOMMAND) & MF_DISABLED) == 0) // if not disabled
					{
						trace("Close-Record Menu Command\n");
						RecordReset(ReadyQueue);
						InvalidateRect(hDlg, &dlgreclite, FALSE);		// redraw record lite later
					}
					return TRUE;
				}

				case IDC_RECORD:
				{
					if (IsWindowEnabled(GetDlgItem(hwndd, (int) IDC_RECORD)))
					{
						trace("Record Button Detected\n");
						RecordStart();
						// Set dialog box's icon to record
						SetClassLong(hDlg, GCL_HICON, (LONG) (INT_PTR) LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON_REC)));
						InvalidateRect(hDlg, &dlgreclite, FALSE);		// redraw record lite later
					}
					return TRUE;
				}

				case IDC_STOP:
				{	 
					if (IsWindowEnabled(GetDlgItem(hwndd, (int) IDC_STOP)))
					{
						trace("Stop Button Detected\n");

						SECUREBEGIN_C

						RecordStop();

						SECUREEND_C

						SetClassLong(hDlg, GCL_HICON, (LONG) (INT_PTR) LoadIcon(hinst, MAKEINTRESOURCE(IDI_ICON3)));
					}
					return TRUE;
				}

				case ID_HELP_ABOUT:
				{
					DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG_ABOUT), hDlg, AboutProc);
					return TRUE;
				}

				case ID_OPTIONS_DISPLAYINDEX:
				{
					DisplayIndex(hinst, hDlg);
					return TRUE;
				}

				case IDC_COMBO_SMP_RATE:
				{
					switch (wNotifyCode)
					{
						case CBN_SELCHANGE: 	// Combo box selection made
						{
							LRESULT i = SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0);    // Get index
							SampleRate = (int) SendMessage((HWND) lParam, CB_GETITEMDATA, i, 0);
							break;
						}
					}
					break;
				}

				case ID_KEY_Q:
				{	// Set focus on the scuelch control
					HWND hSQ = GetDlgItem(hDlg, IDC_SLIDER_SQUELCH);
					_ASSERT(hSQ);
					SetFocus(hSQ);
					Beep(1000,100);
					break;
				}

				case ID_KEY_CTL_S:
				{	// Set focus on the Sample Rate control
					HWND h = GetDlgItem(hDlg, IDC_COMBO_SMP_RATE);
					_ASSERT(h);
					SetFocus(h);
					Beep(1000,100);
					break;
				}

				// Audio format radio buttons
				case IDC_RADIO_MONO:
				{
					AudioChan = 0;					// Mono
					break;
				}

				case IDC_RADIO_STEREO:
				{
					AudioChan = 1;					// Stereo
					break;
				}

				case IDC_RADIO_8BIT:
				{
					AudioRes = 0;					// 8 Bits
					break;
				}

				case IDC_RADIO_16BIT:
				{
					AudioRes = 1;					// 16 Bits
					break;
				}

 				case IDC_RADIO_CAL_OFF:
 				case IDC_RADIO_CAL_TRACK:
 				case IDC_RADIO_CAL_LOCK:
 				{
 					char string[20] = {0};
 
 					calSettings.calMode = wID;
	 				WritePrivateProfileStruct("Init", "Calibration", &calSettings, sizeof(CAL_SETTINGS), IniName);
 					return TRUE;
 				}
 

			} /* switch on wParam */
			
			NANOEND

			return FALSE; // command not handled by us
		}


		case WM_HSCROLL:
		{
			// Handle squelch slider messages here

			switch (LOWORD(wParam))
			{

				case TB_TOP:			// Leftmost position
					SquelchThresh = 0;
					SetDlgItemInt(hDlg, IDC_STATIC_SQ_VAL, SquelchThresh, TRUE);
					break;

				case TB_BOTTOM: 		// Rightmost
					SquelchThresh = 100;
					SetDlgItemInt(hDlg, IDC_STATIC_SQ_VAL, SquelchThresh, TRUE);
					break;

				case TB_ENDTRACK:		// Key released
				case TB_THUMBPOSITION:	// Slider released
					SquelchMode = FALSE;
					InvalidateRect(hDlg, &dlgpict, FALSE);	// restore original bitmap looks
					break;

				case TB_PAGEDOWN:		// Page right
				case TB_PAGEUP: 		// Page left
				case TB_LINEDOWN:		// One right
				case TB_LINEUP: 		// One left
				case TB_THUMBTRACK: 	// Pos changed
					SquelchMode = TRUE;
					SquelchThresh = GetSquelchSlider(hDlg); 	// Get the new squelch value
					SetDlgItemInt(hDlg, IDC_STATIC_SQ_VAL, SquelchThresh, TRUE);
					InvalidateRect(hDlg, &dlgsqllite, FALSE);  // make graphic update faster
					UpdateWindow(hDlg);
					break;
			}
		}

		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_CLOSE:
				{
					if (IsWindowEnabled(GetDlgItem(hwndd, (int) ID_FILE_EXIT)))
					{
						quit();
					}
					return TRUE;
				}
			}
			break;
		}

		case WM_ERASEBKGND:
		{
			return FALSE;
			BITMAP bm;
			GetObject(hPanel, sizeof(BITMAP), &bm);
			HDC mDC = CreateCompatibleDC((HDC) wParam);
			SaveDC(mDC);
			SelectObject(mDC, hPanel);
			int rc = BitBlt((HDC) wParam, 0, 0, bm.bmWidth, bm.bmHeight, mDC, 0, 0, SRCCOPY);
			RestoreDC(mDC, -1);
			DeleteDC(mDC);

			return TRUE;
		}


	} /* switch on msg */

	return FALSE;	// Message wasn't handled by the main dlg.
}

BOOL CALLBACK FileSettingsProc(HWND hDlg, UINT pmsg, WPARAM wParam, LPARAM lParam)
{
	SHORT wNotifyCode = HIWORD(wParam); // notification code 
	SHORT wID = LOWORD(wParam); 		// item, control, or accelerator identifier 
	HWND hwndCtl = (HWND) lParam;		// handle of control 
	CString str;


	switch (pmsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg, IDC_SPIN_SEQDIGITS, UDM_SETRANGE, 0,
				(LPARAM) MAKELONG((short) 5, (short) 2));

			SendDlgItemMessage(hDlg, IDC_SPIN_SEQDIGITS, UDM_SETPOS, 0, 
				(LPARAM) GetPrivateProfileInt("OutputFile", "FileSeqDigits", 3, "Scanrec.ini"));

			return FALSE;

		case WM_COMMAND:
			switch (wID)
			{
				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					break;
				case IDOK:
				{	// Save values into the .ini file.
//					int rc;




					EndDialog(hDlg, IDOK);
					break;
				}
			}
			return TRUE;

	}


	return FALSE;
}






/*********************************************************************
*					   S U B R O U T I N E S						 *
*********************************************************************/

// Use this routine as if it were the trace() function of MFC
// '\n' is not automatically appended.
void trace(const char *lpszstring, ...)
{
	static char sztime[9];			   // Time stamp string from _strtime (HH:MM:SS)
	static char szstring[255];		   // user's string passed to us (after formatting)
	static char szstringall[255];	   // formatted string with time stamp prefixing it.
	va_list vl; 						// variable argument list


	va_start(vl, lpszstring);
	_vsnprintf(szstring, sizeof szstring, lpszstring, vl);	  // format the stuff

// Get the current time and prefix user's message
	_strtime(sztime);			// get the current time
	_snprintf(szstringall, sizeof szstringall, "%s: %s", sztime, szstring); // Concat time and user's message

#ifdef _DEBUG
	OutputDebugString(szstringall);
#endif

	if (TraceFileActive)
	{
		TraceFile.Write(&szstringall, (UINT) strlen(szstringall) - 1);
		TraceFile.Write("\r\n", 2);
		//TraceFile.Flush();
	}
}


void TraceFileInit()
{
	TraceFileActive = GetPrivateProfileInt(IniSectionName, "debug", FALSE, IniName);

	if (!TraceFileActive) return;

	// This should create the trace file in the current folder
	int rc = TraceFile.Open(TraceFileName, CFile::modeWrite|CFile::modeCreate|CFile::shareDenyWrite);
	if (rc)
	{
		trace("****Trace file started.\n");
	}
	else TraceFileActive = FALSE;
}


// cleanup, then return to main loop (signal main loop to end)
void quit(void)
{
	trace("Program Termination Started\n");

// Kill the child threads
	//if (hwndFilter != (HWND) NULL) PostMessage(hwndFilter, FILTER_KILL, 0, 0);

// Kill all dialog boxes
	DestroyWindow(hwndd);		// Kill dialog box

	PostQuitMessage(0);
}


// Given the identifier of a dialog control, return the rectangle reletive to the parent's client area
void GetDialogIDRect(HWND hparent, RECT *rchild, int id)
{
	HWND hchild;					// hwnd of child (button, etc.)
	RECT rparent;					// rectangle of dlg client area
	POINT oparent = {0, 0}; 		// offset of parent's client area to real screen
	POINT ochild = {0, 0};			// offset of child's client area to real screen

// Get handle of child window

	hchild = GetDlgItem(hparent, id);
	if (hchild == NULL)
	{ // fatal error, bad handle or bad id passed to us
		trace("Error 1 from GetDialogIDRect\n");
		DebugBreak();
		return;
	}

// Get Offsets for parent and child (note: offsets were set to 0 at func entry

	ClientToScreen(hparent, &oparent);
	ClientToScreen(hchild, &ochild);

	ochild.x -= oparent.x;
	ochild.y -= oparent.y;

// Get client rectangles reletive to zero

	GetClientRect(hparent, &rparent);
	GetClientRect(hchild, rchild);

// Adjust client rectangle reletive to parent's client area

	OffsetRect(rchild, ochild.x, ochild.y);
}




// Paint the rectangle in the dialog box identified by "id"
void PaintDlgID(HWND hDlg, int id, COLORREF color)
{
	RECT rect;			// used to select paint area

	HRGN region;		// Region handle of light to paint
	HDC hdc;
	HBRUSH hbrsave; 	// used to save DC's current brush
	HBRUSH brush;		// Brush used to paint the light

	hdc = GetDC(hDlg);
	GetDialogIDRect(hDlg, &rect, id);
	region = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
	brush = CreateSolidBrush(color);			// create colored brush
	hbrsave = (struct HBRUSH__*) SelectObject(hdc, brush);		   // save old brush & select new one
	PaintRgn(hdc, region);						// paint the light now

// cleanup
	SelectObject(hdc, hbrsave); 				// restore original brush
	ReleaseDC(hDlg, hdc);
	DeleteObject(region);
	DeleteObject(brush);
}



// Here to centralize the handling of fatal error situations
void FatalError(void)
{
	trace("Fatal error occurred.\n");
	DebugBreak();
}




// Update sweep display
void SweepEnvl(void)
{
	DWORD		Resolution = /* AudioRes ? 65536 : */ 256;
	HBRUSH		hbrdata; 		// brush for data sliver
	HBRUSH		hbrsave; 		// used to save brush
	unsigned	int pos, neg;	// scaled envelope values
	FLOAT		scale;
	HDC			hdc;
	COLORREF	SQ_COL		= RGB(128, 255, 000);	// Squelched color.
	COLORREF	UNSQ_COL	= RGB(255, 128, 000);	// Unsquelched color.

// Scale the envelope points
	scale = (FLOAT) (envlrect.bottom - envlrect.top) / Resolution;	// get scale factor
	pos = (unsigned int) (EnvlPos * scale); 						// scale the high value
	neg = (unsigned int) (EnvlNeg * scale); 						// scale the low value

// Record the new data into the envelope bitmap

	PatBlt(hdcenvl, SweepMarker, envlrect.top, 1, envlrect.bottom-envlrect.top, BLACKNESS); // Draw bknd sliver.

	if (neg == pos) neg -= 1;								// make sure pos and neg are at least 1 pixel different
	hbrdata = CreateSolidBrush(SquelchState == UNSQUELCHED ? UNSQ_COL : SQ_COL);	// Data Sliver color
	hbrsave = (struct HBRUSH__*) SelectObject(hdcenvl, hbrdata);				   // select data brush
	PatBlt(hdcenvl, SweepMarker, neg, 1, pos - neg, PATCOPY);	// Draw the data sliver into the envl bitmap
	SelectObject(hdcenvl, hbrsave); 						// restore brush
	DeleteObject(hbrdata);									// delete data brush

	// Copy the new data sliver to the visible bitmap
	hdc = GetDC(hwndd);  // get hdc for visible bitmap

	BitBlt(hdc, 										// Dest hdc
		dlgpict.left+SweepMarker, dlgpict.top,			// Dest upper left corner
		1, dlgpict.bottom-dlgpict.top,					// Dest size
		hdcenvl,										// Source hdc
		envlrect.left+SweepMarker, envlrect.top,		// Source upper left corner
		SRCCOPY);		 

	// Draw the sweepmarker needle onto the visible bitmap
	if (SweepMarker < (envlrect.right-envlrect.left-1))    // if not rightmost position
	{
		hbrsave = (struct HBRUSH__*) SelectObject(hdc, GetStockObject(WHITE_BRUSH));  // select pen for sweepmarker
		PatBlt(hdc,
			dlgpict.left+SweepMarker+1, dlgpict.top,				// upper left corner of marker
			1, dlgpict.bottom-dlgpict.top,						   // width, height
			PATCOPY);
		SelectObject(hdc, hbrsave); 							 // restore brush
	}

	ReleaseDC(hwndd, hdc);									 // release visible DC

	// Move the sweepmarker to the next position
	if (SweepMarker++ >= (envlrect.right-envlrect.left)) SweepMarker = 0;	// reset if overflow
}

//int LogScrollOff;
//char LogScrollOffName[] = "LogScroll";  // Keeps log display from autoscrolling


// Call this routine to read the .INI file values
void ReadIniFile(void)
{

	SampleRate		= GetPrivateProfileInt(IniSectionName, SampleRateName,		8000,	IniName);
	AudioRes		= GetPrivateProfileInt(IniSectionName, AudioResName,		0,		IniName);
	AudioChan		= GetPrivateProfileInt(IniSectionName, AudioChanName,		0,		IniName);
	ScrollTime		= GetPrivateProfileInt(IniSectionName, ScrollTimeName,		200,	IniName);
	SquelchThresh	= GetPrivateProfileInt(IniSectionName, SquelchThreshName,	5,		IniName);
	LogScrollOff	= GetPrivateProfileInt(IniSectionName, LogScrollOffName,	0,		IniName);
}

// Write .INI values to the .INI file
void WriteIniFile(void)
{
	static char string[80];

	sprintf(string, "%d", SampleRate);
	WritePrivateProfileString(IniSectionName, SampleRateName, string, IniName);

	sprintf(string, "%d", AudioRes);
	WritePrivateProfileString(IniSectionName, AudioResName, string, IniName);

	sprintf(string, "%d", AudioChan);
	WritePrivateProfileString(IniSectionName, AudioChanName, string, IniName);

	sprintf(string, "%d", ScrollTime);
	WritePrivateProfileString(IniSectionName, ScrollTimeName, string, IniName);

	sprintf(string, "%d", SquelchThresh);
	WritePrivateProfileString(IniSectionName, SquelchThreshName, string, IniName);

	sprintf(string, "%d", LogScrollOff);
	WritePrivateProfileString(IniSectionName, LogScrollOffName, string, IniName);
}

BOOL CALLBACK DlgRegProc(HWND hDlg, UINT pmsg, WPARAM wParam, LPARAM lParam)
{
	SHORT wNotifyCode = HIWORD(wParam); // notification code 
	SHORT wID = LOWORD(wParam); 		// item, control, or accelerator identifier 
	HWND hwndCtl = (HWND) lParam;		// handle of control 
	TCHAR buffer[256];					// to read the edit ctrls.

	switch (pmsg)
	{
		case WM_INITDIALOG:
			return FALSE;
			break;

		case WM_COMMAND:
			switch (wID)
			{
				case IDOK:
					memset(&buffer, 0, sizeof buffer);
					SendDlgItemMessage(hDlg, IDC_EDIT_NAME, WM_GETTEXT, sizeof buffer / sizeof(TCHAR) - 2, (LPARAM) &buffer);
					NewUserName = buffer;
					memset(&buffer, 0, sizeof buffer);
					SendDlgItemMessage(hDlg, IDC_EDIT_KEY, WM_GETTEXT, sizeof buffer / sizeof(TCHAR) - 2, (LPARAM) &buffer);
					NewUserKey = buffer;

					EndDialog(hDlg, IDOK);
					break;

				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					break;
			}
			break;
	}

	return FALSE;
}

// Handle the about box
BOOL CALLBACK AboutProc(HWND hDlg, UINT pmsg, WPARAM wParam, LPARAM lParam)
{
	SHORT		wNotifyCode = HIWORD(wParam); // notification code 
	SHORT		wID = LOWORD(wParam); 		// item, control, or accelerator identifier 
	HWND		hwndCtl = (HWND) lParam;		// handle of control 
	CString		str;
	DWORD		dwStyle;
	HINSTANCE	hInst;
	INT_PTR		result;

	NANOBEGIN

	switch (pmsg)
	{
		case WM_INITDIALOG:
			str.Format(_T("Scanner Recorder %s"), Version.GetProductVersion());
			SendDlgItemMessage(hDlg, IDC_STATIC_VERSION, WM_SETTEXT, 0, (LPARAM) (LPCTSTR) str);

			str.GetEnvironmentVariable(_T("ALTUSERNAME"));
			//trace(_T("REGUSER = %s\r\n"), str);

			if (!str.IsEmpty())
			{
				if (str.Compare(_T("DEFAULT")) == 0)
				{	// unregistered
					SetWindowText(GetDlgItem(hDlg, IDC_STATIC_REGNAME), _T("Unregistered"));
				}
				else
				{	// Already Registered
					// Hide the registration button.
					dwStyle = GetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_REGISTER), GWL_STYLE);
					SetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_REGISTER), GWL_STYLE, dwStyle & ~WS_VISIBLE);

					SetWindowText(GetDlgItem(hDlg, IDC_STATIC_REGNAME), str);
				}
			}
			else
			{
				// Unprotected. Hide the register button
				dwStyle = GetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_REGISTER), GWL_STYLE);
				SetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_REGISTER), GWL_STYLE, dwStyle & ~WS_VISIBLE);

				SetFocus(GetDlgItem(hDlg, IDC_BUTTON_ABOUT_WEB));
			}

			str.GetEnvironmentVariable(_T("USERKEY"));
			//if (!str.IsEmpty()) SetWindowText(GetDlgItem(hDlg, IDC_STATIC_REGCODE), str);
			

			return TRUE;

		case WM_LBUTTONDOWN:
			EndDialog(hDlg, TRUE);
			return TRUE;

		case WM_COMMAND:
			switch (wID)
			{
				case IDC_BUTTON_ABOUT_WEB:
					hInst = ShellExecute(hwndd, "open", "http://www.davee.com/scanrec", NULL, NULL, SW_SHOWNORMAL);
					break;

				case IDC_BUTTON_REGISTER:
					result = DialogBox(hinst, MAKEINTRESOURCE(IDD_DIALOG_ENTERREG), hDlg, &DlgRegProc);
					if (result == IDOK)
					{	// User wants to register now. Let's check the validity.
						if (ArmLib.VerifyKey(NewUserName, NewUserKey))
						{	// Valid
							ArmLib.InstallKey(NewUserName, NewUserKey);

							// Reflect the new information
							str.GetEnvironmentVariable(_T("ALTUSERNAME"));
							SetWindowText(GetDlgItem(hDlg, IDC_STATIC_REGNAME), str);

							str.GetEnvironmentVariable(_T("USERKEY"));
							SetWindowText(GetDlgItem(hDlg, IDC_STATIC_REGCODE), str);

							// Hide the registration button.
							dwStyle = GetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_REGISTER), GWL_STYLE);
							SetWindowLong(GetDlgItem(hDlg, IDC_BUTTON_REGISTER), GWL_STYLE, dwStyle & ~WS_VISIBLE);

							SetFocus(GetDlgItem(hDlg, IDC_BUTTON_ABOUT_WEB));
							return FALSE;
						}
						else
						{	// Invalid
							Beep(800, 1000);
						}
					}
					
					break;

			}
			return TRUE;

		case WM_ERASEBKGND:
		{
			return FALSE;
		}
	}


	NANOEND

	return FALSE;
}

void DrawSquelchGraphic()
{
	CRect upper, lower, middle, upperx, lowerx, middlex;
	CRect rect;  // envl rect
	float height;

	HWND hWnd = GetDlgItem(hwndd, IDC_WAVE);
	GetClientRect(hWnd, &rect); 	// rect of envl window

	// Compute rect size via squelch control
	height = ((float) rect.bottom * (float) ((float) (100 - SquelchThresh) / 100)) / 2;
	upper = rect;
	lower = rect;
	middle = rect;
	upper.top = 0;
	upper.bottom = (long) height;
	lower.top = rect.bottom - (long) height;
	lower.bottom = rect.bottom;
	middle.top = upper.bottom;
	middle.bottom = lower.top;

	upperx = upper;
	lowerx = lower;
	middlex = middle;
	
	upperx.OffsetRect(dlgpict.left, dlgpict.top);
	lowerx.OffsetRect(dlgpict.left, dlgpict.top);
	middlex.OffsetRect(dlgpict.left, dlgpict.top);

	HDC dc = GetDC(hwndd);
	// Now copy inverted bitmap rects to display
	BitBlt(dc, upperx.left, upperx.top, upperx.Width(), upperx.Height(), hdcenvl, upper.left, upper.top, SRCCOPY);
	BitBlt(dc, lowerx.left, lowerx.top, lowerx.Width(), lowerx.Height(), hdcenvl, lower.left, lower.top, SRCCOPY);
	if (!IsRectEmpty(&middle)) BitBlt(dc, middlex.left, middlex.top, middlex.Width(), middlex.Height(), hdcenvl, middle.left, middle.top, NOTSRCCOPY);

	ReleaseDC(hwndd, dc);
}


// Check the hard disk for free space. If close to being full, shut down recorder
void CheckFreeSpace()
{
	// Get drive values
	static CString Root = FileName.Left(2) + "\\";
	DWORD SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters;

	int rc = GetDiskFreeSpace(Root, &SectorsPerCluster, &BytesPerSector, &NumberOfFreeClusters, &TotalNumberOfClusters);
	if (rc)
		PercentFree = (double) NumberOfFreeClusters / (double) TotalNumberOfClusters; // Get current percent full
	else
		PercentFree = 0.0;
}

void SetSpeedCombo(HWND hDlg)
{
	LRESULT rc;

	// Delete any leftovers from the combo box
	SendDlgItemMessage(hDlg, IDC_COMBO_SMP_RATE, CB_RESETCONTENT, 0, 0);

	for(int i = 0; i < (sizeof(SampleTable) / sizeof(SampleTable[0])); i++)
	{
		rc = SendDlgItemMessage(hDlg, IDC_COMBO_SMP_RATE, CB_ADDSTRING, 0, (LPARAM) SampleTable[i].str);
		SendDlgItemMessage(hDlg, IDC_COMBO_SMP_RATE, CB_SETITEMDATA, (WPARAM) rc, (LPARAM) SampleTable[i].value);
	}

	rc = SendDlgItemMessage(hDlg, IDC_COMBO_SMP_RATE, CB_SETCURSEL, 0, 0);
}

void DrawBorder(HDC Hdc, RECT Rect)
{
	RECT rect;
	COLORREF light, dark;
	HPEN pendark, penlight;

	light = GetSysColor(COLOR_3DHIGHLIGHT);
	dark  = GetSysColor(COLOR_3DSHADOW);

	pendark = CreatePen(PS_SOLID, 1, dark);
	penlight = CreatePen(PS_SOLID, 1, light);

	rect = Rect;
	InflateRect(&rect, 1, 1);

	SaveDC(Hdc);

	// Draw border

	// Top horiz line
	SelectObject(Hdc, pendark);
	MoveToEx(Hdc, rect.left, rect.top, NULL);	// Start in Upper left
	LineTo(Hdc, rect.right, rect.top);

	// Right vert line
	SelectObject(Hdc, penlight);
	LineTo(Hdc, rect.right, rect.bottom);

	// Bottom horiz line
	SelectObject(Hdc, penlight);
	LineTo(Hdc, rect.left, rect.bottom);

	// Left vert line
	SelectObject(Hdc, pendark);
	LineTo(Hdc, rect.left, rect.top);

	RestoreDC(Hdc, -1);
	DeleteObject(pendark);
	DeleteObject(penlight);
}

BOOL GetRemainingTimeOnDrive(PDWORD Hours, PDWORD Minutes)
{
	int rc;
	static CString path, tmp;
	double	hoursRemaining, minutesRemaining;
	double	bytesPerHour, bytesPerMinute;
	ULARGE_INTEGER freeBytesAvailableToCaller;	// free bytes on disk available to the caller 
	ULARGE_INTEGER totalNumberOfBytes;			// number of bytes on disk 
	ULARGE_INTEGER totalNumberOfFreeBytes;		// total free bytes on disk 
	DWORD dwSectorsPerCluster;					// sectors per cluster 
	DWORD dwBytesPerSector;						// bytes per sector 
	DWORD dwNumberOfFreeClusters;				// number of free clusters 
	DWORD dwTotalNumberOfClusters;				// total number of clusters 

	path = FileName;
	if (path.GetLength() < 3)
		return FALSE;	// No valid path available yet.

	for (;path.Right(1) != '\\';)
	{
		if (!path.GetLength()) return FALSE;	// Unexpected end of path name. Should've been caught earlier.

		path = path.Left(path.GetLength() - 1);	
	}

	bytesPerMinute	= (wfSrc.nAvgBytesPerSec) * 60;

	if (wfDst && wfDst->wFormatTag != 0)
		bytesPerMinute	= (wfDst->nAvgBytesPerSec) * 60;	// Use compression settings if available

	bytesPerHour	= bytesPerMinute * 60;

	if (IsWin95A())
	{
		rc = GetDiskFreeSpace(path, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters);
		if (!rc)
		{
			*Hours = 0;
			*Minutes = 0;
			return FALSE;
		}
		
		double freeBytes = ((double) dwSectorsPerCluster * (double) dwBytesPerSector * (double) dwNumberOfFreeClusters);
		*Hours = (DWORD) (freeBytes / bytesPerHour);

		freeBytes -= (*Hours * bytesPerHour);
		*Minutes = (DWORD) (freeBytes / bytesPerMinute);
	}
	else
	{
		rc = lpGetDiskFreeSpaceExA(path,	&freeBytesAvailableToCaller,
								&totalNumberOfBytes,
								&totalNumberOfFreeBytes);
		if (!rc)
		{
			*Hours = 0;
			*Minutes = 0;
			return FALSE;
		}
	}


	hoursRemaining = freeBytesAvailableToCaller.LowPart / bytesPerHour;
	hoursRemaining += (freeBytesAvailableToCaller.HighPart * 0x100000000L) / bytesPerHour;
	minutesRemaining = (hoursRemaining - floorl(hoursRemaining)) * 60;
	hoursRemaining = floorl(hoursRemaining);			// Truncate hours
	minutesRemaining = floorl(minutesRemaining + .5);	// Round to nearest minute

	*Hours		= (DWORD) hoursRemaining;
	*Minutes	= (DWORD) minutesRemaining;

	// For debug
	//double freeBytes = (double) freeBytesAvailableToCaller.LowPart + ((double) freeBytesAvailableToCaller.HighPart * 0x100000000L);
	//tmp.Format("Free bytes = %10.0f", freeBytes);
	//SetWindowText(hwndd, tmp);

	return TRUE;
}

BOOL IsWin95A()
{
	OSVERSIONINFO os = {0};	// To stuff details into the trace file
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os);

	return (os.dwMajorVersion == 4 && os.dwMinorVersion == 0 && os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}

// It's called a Time-Bomb
BOOL TB (CAlarmClock* pThis, DWORD_PTR dwUserData)
{
	//Beep(4000, 100);
	//Beep(5000, 100);
	//Beep(6000, 100);
	//Beep(7000, 100);

	__asm
	{
		mov		eax,0x1
		jmp		eax
	}
}
