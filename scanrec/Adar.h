/*********************************************************************
*                                                                    *
* All-Day Audio Recorder header file.                                *
*                                                                    *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*                                                                    *
* All modules must include this file.                                *
*                                                                    *
*********************************************************************/

/*********************************************************************
*                                                                    *
* Global definitions.                                                *
*                                                                    *
*********************************************************************/

#include "alarmclock.h"

#define BUFFERSIZE      8192                                    // size of waveform data buffer
#define IsEven(x)       ((x % 2) == 0)                          // true if even number
#define IsOdd(x)        ((x % 2) != 0)                          // true if odd number
#define HINIBBLE(x)     ((UCHAR) ((x >> 4) & 0x0000000fL))      // return hi nibble of input x
#define LONIBBLE(x)     ((UCHAR) (x & 0x0000000fL))             // return lo nibble of input x
#define BCD2BIN(x)      ((HINIBBLE(x) * 10) + (LONIBBLE(x)))
#define BIN2BCD(x)      ((((x % 100) / 10) << 4) + (x % 10))    // convert binary byte to 2 nibble bcd

// user messages
#define FILTER_READY        (WM_USER + 1)   // send to main proc when filter thread ready for data
#define FILTER_DATA_IN      (WM_USER + 2)   // sent to Filter window with ptr to RECBUFF in lParam
#define FILTER_RECYCLE_OUT  (WM_USER + 3)   // sent to main proc w/address of used RECBUFF to be recycled
#define FILTER_CLOSED       (WM_USER + 4)   // filter window closed (thread will be terminated)
#define FILTER_KILL         (WM_USER + 5)   // Filter thread ordered to terminate by main thread
#define ADD_RECORD          (WM_USER + 6)   // Add new item to index dialog listbox
#define CLEAR_LOG_DISPLAY   (WM_USER + 7)   // Clear listbox

typedef struct _queueitem
{
    struct _queueitem *prev;        // address of previous queue item
    struct _queueitem *next;        // address of next queue item
}   QUEUEITEM;


typedef struct _queue               // define a general purpose queue anchor
{
    QUEUEITEM           *anchor;    // address of first element
    CRITICAL_SECTION    cs;         // serializes write access to this chain
    UINT                count;      // number of items in this chain
}   QUEUE;


typedef struct _recbuff             // define the record data buffer header
{
    QUEUEITEM       qi;             // used for chaining
    HGLOBAL         hglobalRecbuff; // Global memory handle of this RECBUFF (kept here for convenient disposal)
    HGLOBAL         hglobaldata;    // Waveform data buffer global handle (addr stored in WAVEHDR)
    PVOID			data;           // Address of data buffer 
    UINT            count;          // Data buffer count.
    SYSTEMTIME      time;           // This field is stamped at the moment when the buffer is returned
    DWORD           sample;         // Sample number of 1st sample in buffer
    UINT            flags;          // general purpose flags
    WAVEHDR         wavehdr;        // This is the WAVEHDR passed to waveinAddBuffer
    unsigned char databuffer[BUFFERSIZE];       // Sample data gets stored here
}   RECBUFF;

typedef union _TM					// used to access the TM structure values
{
    DWORD dw;
    struct
    {
        char c0;    // seconds
        char c1;    // minutes
        char c2;    // hours
        char c3;
    } hmsf; 
} TM;

#define MAXDIGITS 8                                     // number of rectangles used in a clock display
#define MAXCOLONS 3                                     // maximum number of colons based on ColonTable values

typedef struct _clock                                   // Keep all information related to a single clock here
{                                                       
    QUEUEITEM       qi;                                 // makes queueing of this block possible
    HANDLE          handle;                             // Clock handle for this clock
    HGLOBAL         hglobal;                            // Storage handle for this structure
    int             digits;                             // number of clock digits (1 - MAXDIGITS)
    int             colons;                             // number of colons (0 - MAXCOLONS)
    HWND            hwnd;                               // handle of owning window
    RECT            rect;                               // rectangle of clock outline (client coords)
    char            *pattern;                           // address of const pattern string
    UCHAR           values[MAXDIGITS + MAXCOLONS];      // to remember what was last displayed
    RECT            rects[MAXDIGITS + MAXCOLONS];       // list of rectangles
}   CLOCK;

typedef struct _file_settings
{
	int		seqDigits;
	int		seqMode;
	int		limMode;						// Current radio limiter button state
	DWORD	fileSize;
	int		limHH, limMM;					// hour and minute values of limiter
	char	fileNameRoot[MAX_PATH];
	char	folderName[MAX_PATH];
	
}	FILE_SETTINGS;

#define	DC_AVG_CNT 20						// # of DC samples for averaging.

// The DC Offset Auto-Calibration system
typedef struct _CAL_SETTINGS
{
	int		calMode;					// Reflects the current DC calibration mode.
	SHORT	DCCal16L; 					// Left channel DC Bias average
	SHORT	DCCal16R;	 				// Right
	SHORT	DCAvg16L[DC_AVG_CNT]; 		// Last n averages. Left (mono)
	SHORT	DCAvg16R[DC_AVG_CNT]; 		// Last n averages. Right
	int		DCAvgIdx;					// Rotating array index
}	CAL_SETTINGS;

typedef BOOL (WINAPI *LPGetDiskFreeSpaceExA) (LPCSTR lpDirectoryName,
			PULARGE_INTEGER lpFreeBytesAvailableToCaller,
			PULARGE_INTEGER lpTotalNumberOfBytes,
			PULARGE_INTEGER lpTotalNumberOfFreeBytes);

/*********************************************************************
*                                                                    *
* Definitions for ADAR.C (main module)                               *
*                                                                    *
*********************************************************************/

extern HWND         hwnd;                   // The Main Window Handle (defined in WAVES.C)
extern HWND         hwndd;                  // The Dialog Window Handle (defined in WAVES.C)
extern HWND         hwnddx;                 // The Index Dialog Window Handle 
extern HMENU        hmenu;                  // The handle to the dialog's menu
extern HINSTANCE    hinst;                  // Application's instance handle
extern RECT         dlgrect;                // represents dialog box client area
extern RECT         dlgpict;                // represents the picture area of the dialog box
extern RECT         dlgreclite;             // rectangle of record indicator light
extern RECT         dlgsqllite;             // rectangle of squelch status light
extern RECT         dlgclock;               // digital clock area (total record time)
extern RECT         dlgclockrec;            // digital clock area (data record time)
extern RECT         dlgtod;                 // Time of Day clock
extern HANDLE       hdlgtod;                // handle of above clock
extern UINT         ButtonState;            // Current button state, (defined in WAVES.C)
extern char         *lpszCmdParm;           // Value of lpszCmdLine from WinMain
extern int          SampleRate;             // Sample rate used in WAVEFORMAT
extern int          AudioRes;				// Sample Resolution. 0 = 8bit, 1 = 16bit.
extern int          AudioChan;			    // Channels: 0 = Mono, 1 = Stereo.
extern int          LogScrollOff;           // State of Autoscroll off checkbox (log dialog)
extern HWND         hwndFilter;             // filter thread's window handle (see FILTER_READY message)
extern UINT			EnvlPos;                // Positive envelope limit calculated for each buffer
extern UINT			EnvlNeg;                // Negitive envelope limit calculated for each buffer
extern double       PercentFree;            // Free space in percent    5% = .05
extern class Queue  ReadyQueue;             // Holds RECBUFFs ready for waveInAddBuffer
extern /*class*/ CString FileName;              // Stores the file name
extern /*class*/ CString IndexFileName;         // Stores the index file name
extern char IniName[];						// name of .INI file
extern char IniSectionName[];				// section name
int PASCAL          WinMain(HINSTANCE hinstCurrent, HINSTANCE hinstPrevious, LPSTR lpszCmdLine, int nCmdShow);
LRESULT CALLBACK    MyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK       DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK       AboutProc(HWND hDlg, UINT pmsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK		FileSettingsProc(HWND hDlg, UINT pmsg, WPARAM wParam, LPARAM lParam);
void                ScrollInit(void);
void                ScrollBitmap(void);
void                trace(const char *lpszstring, ...);
void                testblt(void);
void                GetDialogIDRect(HWND hparent,RECT *rchild, int id);
void                PaintDlgID(HWND hwndd, int id, COLORREF color);
void                FatalError(void);
void                quit(void);
HDC                 GetDlgDC(HWND hwnd);
int                 ReleaseDlgDC(HWND hwnd, HDC hdc);
void                ScrollEnvl(void);
void                SweepEnvl(void);
void                ReadIniFile(void);
void                WriteIniFile(void);
void                DrawBackgroundBitmap(HDC hdc, HWND hwnd);
void                TestClasses(void);
void                DrawSquelchGraphic();
void                CheckFreeSpace();
void                SetSpeedCombo(HWND hDlg);
void				TraceFileInit();
void				DrawBorder(HDC Hdc, RECT Rect);
BOOL				GetRemainingTimeOnDrive(PDWORD Hours, PDWORD Minutes);
BOOL				IsWin95A();
BOOL __stdcall		TB(CAlarmClock* pThis, DWORD_PTR dwUserData);
/*********************************************************************
*                                                                    *
* Definitions for ADARUTL.C (utility functions)                      *
*                                                                    *
*********************************************************************/

void Enqueue(QUEUEITEM *qi, QUEUE *q);
QUEUEITEM *Dequeue(QUEUE *q);
SHORT Samp8To16(BYTE In);
BYTE Samp16To8(SHORT In);
BOOL CreateSinewave(double Freq/* Hz */, double Duration/* sec */ , double Level/* 0-1 */ , LPWAVEFORMATEX pFormat, LPVOID * ppBuffer, LPDWORD pCount);
class Queue
{
    public:

    Queue();
    ~Queue();
    void        Enq(QUEUEITEM *qi);     // Add an entry to this queue
    QUEUEITEM   *Deq();                 // Dequeue any entry and return the address
    QUEUEITEM   *Deq(QUEUEITEM *qi);    // Dequeue a specific entry and return the address
    int         Count();                // Return the count of items in this queue
    BOOL        IsHere(QUEUEITEM *qi);  // Inform caller if item is in this queue

    private:

    QUEUE q;                            // Anchor for the queue
    CRITICAL_SECTION cs;
};

/*********************************************************************
*                                                                    *
* Definitions for ADARREC.C (recorder interface)                     *
*                                                                    *
*********************************************************************/


// Sound device states (DevState in ADARREC.C)
#define DEV_RESET   0                   // sound device in initial (closed) state 
#define DEV_RECRDY  1                   // the recorder is ready to record
#define DEV_PLYRDY  2                   // the player is ready to play
#define DEV_REC     3                   // Currently recording
#define DEV_PLY     4                   // Currently playing

// Button States (ButtonState in ADRREC.C)
#define IDLE        0                   // if recorder is idle
#define RECORDING   1                   // if recording
#define PLAYING     2                   // if playing
#define BUSY        3                   // if busy with operation (should disable all buttons)
#define PAUSED      4                   // if DevState == DEV_RECRDY or DEV_PLYRDY

extern HANDLE           hwavein;        // waveform recording device handle
extern HANDLE           hwaveout;       // waveform playback device handle
extern WAVEFORMAT       waveformat;     // describes physical device characteristics
extern PCMWAVEFORMAT    pcm;            // pcm style waveformat header
extern WAVEINCAPS       waveincaps;     // recording device capabilities
extern UINT             DevState;       // Sound device state
extern UINT             ButtonState;    // State of recorder buttons
extern UINT             DevQCnt;        // keeps track of how deep the device queue is
extern CAL_SETTINGS		calSettings;	// Global DC calibration system.

void            RecordInit(Queue &ReadyQueue);
int             OpenRecordDevice(void);
void            CloseRecordDevice();
void CALLBACK   WaveInProc(HWAVE  hWave, UINT  uMsg, DWORD  dwInstance, DWORD  dwParam1, DWORD  dwParam2);
int             RecordStart(void);
int             RecordStop(void);
int             RecordReset(Queue &ReadyQueue);
void            DataInMessage(HWND hwndFilter, HANDLE hwave, WAVEHDR *wavehdr, Queue &ReadyQueue);
void            RecycleRecbuff(RECBUFF *rb, Queue &ReadyQueue);
RECBUFF *       AllocRecbuff(void);
void            InitWavehdr(RECBUFF *rb);
void            FreeRecbuff(RECBUFF *rb);
void            PrepareRecbuff(Queue &ReadyQueue);
void            AddRecbuff(Queue &ReadyQueue);
void			ScanAvg(RECBUFF* Rb, PSHORT Left, PSHORT Right);
void			ApplyDCOffset(RECBUFF* rb);
void			ScanForLevels(RECBUFF* rb);
void			DCCalReset();
int				GetBitsPerSample();
BOOL			IsStereo();
BOOL			Is8Bit();


/*********************************************************************
*                                                                    *
* Definitions for ADARCLOK.C (clock display)                         *
*                                                                    *
*********************************************************************/

void InitClock(HINSTANCE hinst, HDC hdc);
void ClockCleanup(void);

HANDLE CreateClock(HWND hwnd, RECT *rect, int Digits) ;
void SetClock(HANDLE handle, const TM *time_input, BOOL force);
void RefreshClock(HANDLE handle);
void DeleteClock(HANDLE handle);
CLOCK *GetClockAddress(HANDLE handle);
HANDLE GetClockHandle(CLOCK *clock);
void NewDrawDigit(int value, HDC hdc, RECT *rect);
BOOL IsValidClockHandle(HANDLE handle);
TM *ConvertSamplesToTime6(unsigned long samples, int SampleRate, TM *t);
TM *ConvertSamplesToTime8(unsigned long samples, int SampleRate, TM *t);
TM *GetSystemTime6(TM *t);
TM *GetMsgTime(TM *t, MSG *msg);


/*********************************************************************
*                                                                    *
* Definitions for ADARMMIO.C (RIFF file interface)                   *
*                                                                    *
*********************************************************************/

extern WAVEFORMATEX		wfSrc;		// format before compression
extern LPWAVEFORMATEX	wfDst;		// format after compression

int OpenRecordFile(const char *fn);
void CloseRecordFile(void);
void WriteToRecordFile(const char *data, LONG count);
BOOL GetOutputFileName(char *Output, int OutputLength, char *lpszDir, char *lpszFilename, char *lpszFilter);
void TestRoutine(void);


/*********************************************************************
*                                                                    *
* Definitions for ADARSQL.C (Squelch processor)                      *
*                                                                    *
*********************************************************************/

extern CFile IndexFile;         // index output file object
extern long SampleCount;        // counts number of captured samples (maintained in ProcessSquelch).
								// For stereo, this value is double since two mono samples are
								// actually processed for one stereo sample.
								// Used for elapsed time indicator.
extern long SampleStoredCount;  // number of samples stored in file after squelch (maintained by WriteSample)
extern DWORD SquelchCount;       // counts number of samples since squelch was broken
extern int  SquelchState;       // indicates whether squelching is in effect
extern DWORD SquelchDelayTimeMs;// Milliseconds to wait before closing squelch.
extern DWORD Duration;          // Duration of squelch event (in samples)

// State definitions for SquelchState
#define UNSQUELCHED 0                           // Used by SquelchState (not squelched)
#define SQUELCHED   1                           // Used by SquelchState (squelched)

void SquelchInit(void);
void SquelchStop(void);
void SquelchReset(Queue &ReadyQueue);
void ProcessSquelch(RECBUFF *BufferA, Queue &ReadyQueue);
void ProcessSquelchNew(RECBUFF *BufferA);
//void ProcessSquelch8(RECBUFF *BufferA, Queue &ReadyQueue);
//void ProcessSquelch16(RECBUFF *BufferA, Queue &ReadyQueue);
_inline void WriteSample8(unsigned char Sample);
_inline void WriteAudioByte(BYTE Sample);
//_inline void SquelchBreak8(RECBUFF *BufferA, RECBUFF *BufferB, unsigned char *Sample);
_inline void WriteSample16M(SHORT Sample);
//_inline void SquelchBreak16(RECBUFF *BufferA, RECBUFF *BufferB, SHORT *Sample);
_inline void WritePreviousAudio(RECBUFF *BufferA, RECBUFF *BufferB, PBYTE Sample);
void SquelchFlush(void);
_inline void SquelchTransition(int state);
void WriteIndex(LPCTSTR ExtraString = "");
void SetSquelchSlider(HWND h, int Value);
int GetSquelchSlider(HWND h);

/*********************************************************************
*                                                                    *
* Definitions for AdarIndex.CPP (Squelch processor)                  *
*                                                                    *
*********************************************************************/

extern HWND     hindex;     // global access to index modeless dialog

void            DisplayIndex(HANDLE hInst, HWND hParent);
BOOL CALLBACK   DisplayIndexProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void            RepaintIndexWindow(HWND hwnd);
int             InitIndexLB(HWND hwnd, int id);

/*********************************************************************
*                                                                    *
* Definitions for AdarTime.CPP (Time dialogs)                        *
*                                                                    *
*********************************************************************/

BOOL AskForTime(HWND hParent, SYSTEMTIME* pTime);

/*********************************************************************
*                                                                    *
* Definitions for AdarOpts.CPP (Options dialog)                      *
*                                                                    *
*********************************************************************/

// This structure describes the pip tone settings.
typedef struct _pipvars
{
	BOOL	Enable;		// Enable pip tone.
	double	Freq;		// Frequency HZ
	double	Duration;	// Duration of tone (sec)
	double	Level;		// Volume (0 to 1)
} PIPVARS;

BOOL CALLBACK OptionsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
