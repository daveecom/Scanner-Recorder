/*********************************************************************
*                                                                    *
* All-Day Audio Recorder Clock Display Handler                       *
*                                                                    *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*********************************************************************/

#include "AdarPch.h"

Queue Clocks;                           // queue of clock information blocks

HDC hdctemp;                            // memory DC used for bitmap ops
HBITMAP hBitmapOld;                     // save HDC's orig bitmap handle here

HBITMAP     h0;                         // 0
HBITMAP     h1;                         // 1
HBITMAP     h2;                         // 2
HBITMAP     h3;                         // 3
HBITMAP     h4;                         // 4
HBITMAP     h5;                         // 5
HBITMAP     h6;                         // 6
HBITMAP     h7;                         // 7
HBITMAP     h8;                         // 8
HBITMAP     h9;                         // 9
HBITMAP     hc;                         // colon

// digit bitmap handles pointer table colon must always be last entry
HBITMAP *DigitTable[] = {&h0, &h1, &h2, &h3, &h4, &h5, &h6, &h7, &h8, &h9, &hc};
#define MAXBITMAPS (sizeof DigitTable / sizeof DigitTable[0])  // total number of image bitmaps
#define COLONBITMAP (MAXBITMAPS - 1)    // index of colon bitmap

// table converts # digits to formatting information
struct _FmtTable
{
    int Colons;             // number of colons needed for this many digits
    char *Pattern;          // string of 'D' and 'C' characters to describe the sequence of digits / colons
    int shift;              // number of input nibbles to shift left based on number of digits
} FmtTable[] =
{                           
    0,  "0",              7,    // 1 digit
    0,  "00",             6,    // 2 digits
    1,  "00:0",           5,    // 3 digits
    1,  "00:00",          4,    // 4 digits
    2,  "00:00:0",        3,    // 5 digits
    2,  "00:00:00",       2,    // 6 digits
    3,  "00:00:00:0",     1,    // 7 digits
    3,  "00:00:00:00",    0,    // 8 digits
};


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


// initialize the digital clock bitmaps (Use ClockCleanup to delete the handles upon termination)
void InitClock(HINSTANCE hinst, HDC hdc)
{

    *DigitTable[0]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT0));
    *DigitTable[1]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT1));
    *DigitTable[2]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT2));
    *DigitTable[3]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT3));
    *DigitTable[4]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT4));
    *DigitTable[5]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT5));
    *DigitTable[6]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT6));
    *DigitTable[7]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT7));
    *DigitTable[8]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT8));
    *DigitTable[9]  = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_DIGIT9));
    *DigitTable[10] = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_COLON));

    hdctemp = CreateCompatibleDC(hdc);      // create temporary clock digits dc
    hBitmapOld = (struct HBITMAP__*) GetCurrentObject(hdctemp, OBJ_BITMAP);    // save old bitmap handle

}



// delete the handles for the digital clock digits
void ClockCleanup(void)
{
    SelectObject(hdctemp, hBitmapOld);  // restore original dc settings
    DeleteDC(hdctemp);

    DeleteObject(*DigitTable[0]);
    DeleteObject(*DigitTable[1]);
    DeleteObject(*DigitTable[2]);
    DeleteObject(*DigitTable[3]);
    DeleteObject(*DigitTable[4]);
    DeleteObject(*DigitTable[5]);
    DeleteObject(*DigitTable[6]);
    DeleteObject(*DigitTable[7]);
    DeleteObject(*DigitTable[8]);
    DeleteObject(*DigitTable[9]);
    DeleteObject(*DigitTable[10]);

}



// Initialize a clock structure and return a new handle
HANDLE CreateClock(HWND hwnd, RECT *rect, int Digits) 
{
    HGLOBAL hGlbClock;   
    CLOCK *clock;                       // temp place to store clock ptr

// Used to compute digit and colon rectangles
    int DesiredTotalWidth;
    int DigitWidth;
    int DigitHeight;
    int DigitWidthTotal;
    int ColonWidth;
    int ColonHeight;
    int ColonWidthTotal;
    int TotalWidth;
    int Digit;
    float Ratio;
    float NewDigitWidthTotal;
    float NewColonWidthTotal;
    float NewDigitWidth;
    float NewColonWidth;
    int i;
    BITMAP bm;                          // provides access to dimensions
        
// Allocate storage for this clock structure
    if ((hGlbClock = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (CLOCK) )) == NULL)
        FatalAppExit(0, "Out of global storage during CreateClock");
    if ((clock = (CLOCK *) GlobalLock(hGlbClock)) == NULL)
        FatalAppExit(0, "Can't lock storage during CreateClock");
    
    clock->hglobal = hGlbClock;                         // save handle for later displosal
    CopyRect(&(clock->rect), rect);                     // save client rectangle of clock area
    clock->handle = GetClockHandle(clock);              // clock addr = unique handle for this clock
    clock->hwnd = hwnd;                                 // save window handle

// Calculate the rectangles for the digits
    GetObject(*DigitTable[0], sizeof bm, &bm);          // Get dimensions of a digit
    DigitWidth = bm.bmWidth;
    DigitHeight = bm.bmHeight;
    GetObject(*DigitTable[10], sizeof bm, &bm);         // Get dimensions of the colon
    ColonWidth = bm.bmWidth;
    ColonHeight = bm.bmHeight;

    clock->digits       = Digits;                       // Set number of digits in clock
    clock->colons       = FmtTable[Digits-1].Colons;    // Get the number of colons
    clock->pattern      = FmtTable[Digits-1].Pattern;   // Save address of R/O pattern string

    DesiredTotalWidth   = rect->right - rect->left;     // Compute width of input rectangle
    DigitWidthTotal     = clock->digits * DigitWidth;          // Total width of all digit bitmaps
    ColonWidthTotal     = clock->colons * ColonWidth;   // Total width of colon bitmaps
    TotalWidth          = DigitWidthTotal + ColonWidthTotal;    // Total width of all bitmaps
    Ratio	            = (float) DesiredTotalWidth / (float) TotalWidth;   // ratio of desired to actual widths
    NewDigitWidthTotal	= DigitWidthTotal * Ratio;      // calc new width for total digits
    NewColonWidthTotal	= ColonWidthTotal * Ratio;      // and colons
    NewDigitWidth	    = NewDigitWidthTotal / clock->digits;  // New width to use for each digit
    NewColonWidth	    = NewColonWidthTotal / clock->colons;  // New width to use for each colon

// Loop to compute the list of digit entries and rectangles based on the pattern
    for (i = 0, Digit = 0; i < (clock->digits+clock->colons); i++)
    {

// Set top, bottom, left & right of each digit
        clock->rects[i].top = rect->top;                        // set top
        clock->rects[i].bottom = rect->bottom;                  // set bottom
        if (i == 0) clock->rects[i].left = rect->left;          // set left (first digit only)
        else clock->rects[i].left = clock->rects[i-1].right;    // set left (all other digits)

// set right                                                    // set right
        if ((char) *(clock->pattern+i) == '0')                  // if pattern = digit
        {
            clock->values[i] = 0;                               // initial value of this digit
            clock->rects[i].right = (clock->rects[i].left + (int) floor(NewDigitWidth + .5));
        }
        else
        if ((char) *(clock->pattern+i) == ':')                  // if pattern = colon
        {
            clock->values[i] = COLONBITMAP;                     // index of colon bitmap into bitmap table
            clock->rects[i].right = (clock->rects[i].left + (int) floor(NewColonWidth + .5));
        }
    }   // for each clock character position

    Clocks.Enq((QUEUEITEM *) clock); // save this clock structure
    return clock->handle;
}





// Analyze the incoming digits to see if each digit needs to be Blt'd to the screen. The force flag
// causes all digit rectangles to be invalidated.
void SetClock(HANDLE handle, const TM *time_input, BOOL force)
{
    int i, digit;
    TM time;
    CLOCK *clock;
    
    if (!IsValidClockHandle(handle)) FatalAppExit(0, "SetClock(): Bad Clock Handle");
    clock = GetClockAddress(handle);           // Access the clock's data area
    time = *time_input;                 // make copy of input time parameter
    
// Scan the input digits to see if the current digit differs from the new digit. If it does, invalidate the digit
// rectangle and store the new digit in the values[] list.
    time.dw<<=(FmtTable[clock->digits-1].shift * 4);        // Adjust the TM so that the leftmost nibble = leftmost digit of display
    for (i = 0, digit = 0; i < clock->digits; i++, digit++, time.dw<<=4)
    {
        if (clock->values[digit] == COLONBITMAP) digit++;                      // skip this digit if it's a colon
        if ((LONIBBLE(time.dw >> (4*7)) != clock->values[digit]) || force)  // if high nibble of new TM != current value of this digit
        {
            InvalidateRect(clock->hwnd, &(clock->rects[digit]), TRUE);
            clock->values[digit] = LONIBBLE(time.dw >> (4*7));
        }
    }
}





// Call this routine from WM_PAINT to update the window clock bitmap.
void RefreshClock(HANDLE handle)
{
    HDC hdc;                // DC of physical screen
    CLOCK *clock;           // clock data area
    RECT u, x;              // update and intersected rects
    int i;
    HBRUSH hbrsave;         // saves current DC brush
    HBRUSH DigitBrush;      // Brush used to draw digits

    if (!IsValidClockHandle(handle)) FatalAppExit(0, "RefreshClock(): Bad Clock Handle");
    clock = GetClockAddress(handle);        
    hdc = GetDC(clock->hwnd);
    DigitBrush = CreateSolidBrush(RGB(0,32,64));    // Set digit color
    hbrsave = (struct HBRUSH__*) SelectObject(hdc, DigitBrush);

// Check each digit rect and blt to screen if necessary
    GetUpdateRect(clock->hwnd, &u, FALSE);
    clock = GetClockAddress(handle);    
    for(i = 0; i < (clock->digits + clock->colons); i++)
    {
        IntersectRect(&x, &(clock->rects[i]), &u);      // Get the intersection of the current digit & update rect
        if (!IsRectEmpty(&x))
        {   // Draw this rect onto the physical screen
            NewDrawDigit(clock->values[i], hdc, &(clock->rects[i]));    // draw this digit onto the phys screen
        }   
    }

// Clean up
    SelectObject(hdc, hbrsave);     // restore brush
    DeleteObject(DigitBrush);
    ReleaseDC(clock->hwnd, hdc);
}




void NewDrawDigit(int value, HDC hdc, RECT *rect)
{
    BITMAP bm;              // used to get dimensions of digit bitmap

// Get the bitmap dimensions
    GetObject(*DigitTable[value], sizeof bm, &bm);

// Draw the digit
    SelectObject(hdctemp, *DigitTable[value]);   // select the correct digit into the memory hdc
    StretchBlt(hdc, rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top),
        hdctemp, 0, 0, bm.bmWidth, bm.bmHeight ,0x00E20746L); // copy brush with source mask

}



void DeleteClock(HANDLE handle)
{
    HGLOBAL htemp;
    CLOCK *temp;

    if (!IsValidClockHandle(handle))
    {
        CString str;
        str.Format("DeleteClock(): Bad Clock Handle '%08X'.\ncount = '%d'",
        handle, Clocks.Count());
        FatalAppExit(0, str);
    }

    temp = GetClockAddress(handle);     // Get address of clock
    Clocks.Deq((QUEUEITEM*) handle);
    htemp = temp->hglobal;              // get memory handle
    GlobalUnlock(temp);
    GlobalFree(htemp);
}



BOOL IsValidClockHandle(HANDLE handle)
{
    if (Clocks.Count() == 0) return FALSE;     // Invalid handle, queue is empty

    if (Clocks.IsHere((QUEUEITEM*) handle)) return TRUE;
    else return FALSE;

}





CLOCK *GetClockAddress(HANDLE handle)
{
    if (!IsValidClockHandle(handle)) FatalAppExit(0, "GetClockAddress(): Bad Clock Handle");
    return (CLOCK *) handle;
}



HANDLE GetClockHandle(CLOCK *clock)
{
    return (HANDLE) clock;
}



TM *GetSystemTime6(TM *t)
{
    SYSTEMTIME st;

    TM u = {0};

    GetLocalTime(&st);  // Get the local time
    
    u.hmsf.c2 = (char) BIN2BCD(st.wHour);
    u.hmsf.c1 = (char) BIN2BCD(st.wMinute);
    u.hmsf.c0 = (char) BIN2BCD(st.wSecond);

    t->dw = u.dw;         // pass new value back to caller
    return t;       // allows function call to replace (TM *) parms to SetClock
}



// Convert samples to 6 digit hh:mm:ss clock
TM *ConvertSamplesToTime6(unsigned long samples, int SampleRate, TM *t)
{
	_ASSERT(SampleRate);



    TM u = {0};

	if (samples == 0)
	{
		u.dw = 0;
	}
	else
	{
		u.hmsf.c2 = (char) BIN2BCD(samples / SampleRate / 3600 % 60);
		u.hmsf.c1 = (char) BIN2BCD(samples / SampleRate / 60 % 60);
		u.hmsf.c0 = (char) BIN2BCD(samples / SampleRate % 60);
	}

    t->dw = u.dw;   // pass new value back to caller
    return t;       // allows function call to replace (TM *) parms to SetClock
}



// Convert samples to 8 digit hh:mm:ss:ff clock
TM *ConvertSamplesToTime8(unsigned long samples, int SampleRate, TM *t)
{
    TM u = {0};

    u.hmsf.c3 = (char) BIN2BCD(samples / SampleRate / 3600 % 60);
    u.hmsf.c2 = (char) BIN2BCD(samples / SampleRate / 60 % 60);
    u.hmsf.c1 = (char) BIN2BCD(samples / SampleRate % 60);
    u.hmsf.c0 = (char) BIN2BCD(samples % SampleRate);

    t->dw = u.dw;  // pass new value back to caller
    return t;       // allows function call to replace (TM *) parms to SetClock
}



TM *GetMsgTime(TM *t, MSG *msg)
{
    TM u = {0};

    u.hmsf.c2 = (char) BIN2BCD(msg->time / 3600000);
    u.hmsf.c1 = (char) BIN2BCD(msg->time / 60000 % 60);
    u.hmsf.c0 = (char) BIN2BCD(msg->time / 1000 % 60);

    t->dw = u.dw;         // pass new value back to caller
    return t;    
}
