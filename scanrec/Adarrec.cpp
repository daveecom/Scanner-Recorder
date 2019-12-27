/*********************************************************************
*                                                                    *
* ADARREC.C Multimedia Recorder interface module.                    *
*                                                                    *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*                                                                    *
*********************************************************************/

#include "AdarPch.h"

HANDLE hwavein = 0;                     // waveform recording device handle
HANDLE hwaveout = 0;                    // waveform playback device handle
WAVEFORMAT waveformat;                  // describes physical device characteristics
PCMWAVEFORMAT pcm;                      // pcm style waveformat header
WAVEINCAPS waveincaps;                  // recording device capabilities
UINT DevState = DEV_RESET;              // Sound device state
UINT ButtonState = IDLE;                // State of recorder buttons
UINT DevQCnt = 0;                       // keeps track of how deep the device queue is
UINT RBAllocCount = 0;                  // # RECBUFFs allocated
char errortext[256];

CAL_SETTINGS		calSettings			= {0};			// Global DC calibration system.

// The recording device driver has given us a completed data buffer
void DataInMessage(HWND hwndFilter, HANDLE hwave, WAVEHDR *wavehdr, Queue &ReadyQueue)
{
    RECBUFF *rb = (RECBUFF *) wavehdr->dwUser;   // get RECBUFF address for this buffer

    DevQCnt--;                          // account for device queue depth
//  trace("DataInMessage: RB received, RB= %08lX, DevQCnt= %d.\n", rb, DevQCnt);

// Give the device driver another buffer if it's still open
    if (DevState == DEV_REC || DevState == DEV_RECRDY) AddRecbuff(ReadyQueue);    // only replenish if device open for recording

    if (wavehdr->dwBytesRecorded > 0)
    {   // if something got recorded
		// Scan the incoming data for the average value in order to compute the DC calibration.

		if (calSettings.calMode == IDC_RADIO_CAL_TRACK)
		{
			// Average this buffer contents into the current running avg.
			ScanAvg(rb, &calSettings.DCAvg16L[calSettings.DCAvgIdx], &calSettings.DCAvg16R[calSettings.DCAvgIdx]);		// Store the new average into the hist bfr
			calSettings.DCAvgIdx = (++calSettings.DCAvgIdx % DC_AVG_CNT);	// Circular index maintenance
			double valL = 0, valR = 0;
			for (int i = 0; i < DC_AVG_CNT; i++)
			{	// Sum the values in the history buffer
				valL += calSettings.DCAvg16L[i];
				valR += calSettings.DCAvg16R[i];
			}
			valL /= DC_AVG_CNT;
			valR /= DC_AVG_CNT;

			// Store the new average for L and R
			calSettings.DCCal16L = (SHORT) valL;
			calSettings.DCCal16R = (SHORT) valR;
		}

		if (calSettings.calMode == IDC_RADIO_CAL_LOCK || calSettings.calMode == IDC_RADIO_CAL_TRACK)
		{
			// Adjust the DC offset of the data now.
			ApplyDCOffset(rb);
		}

		// Display the corrected avg
		SHORT l, r;
		ScanAvg(rb, &l, &r);
		SetDlgItemInt(hwndd, IDC_STATIC_OFFSETMEAS, l, TRUE);
		SetDlgItemInt(hwndd, IDC_STATIC_OFFSETMEAS2, r, TRUE);

		ScanForLevels(rb);	// Get the display level values to update the current sweep sliver;


		// Pass this new buffer into the squelch processor/writer
        ProcessSquelch(rb, ReadyQueue);
		
    }
    else
    {   // nothing got recorded, just recycle it
        RecycleRecbuff(rb, ReadyQueue);
    }
}



// Open recorder then prepare initial buffer chain.
void RecordInit(Queue &ReadyQueue)
{
	static char b[100];
    MMRESULT rc;
    
	trace("Calling waveInGetDevCaps()\n");
    rc = waveInGetDevCaps(WAVE_MAPPER, &waveincaps, sizeof waveincaps);
	trace("waveInGetDevCaps result = %d\n", rc);
	if (rc != MMSYSERR_NOERROR)
	{
        MessageBox(NULL, "Audio card seems to be missing or not working.", "",
            MB_OK|MB_ICONWARNING|MB_TASKMODAL);
		return;
	}
    
	// set up WAVEFORMAT prior to waveInOpen    
    pcm.wf.wFormatTag           = WAVE_FORMAT_PCM;
    pcm.wf.nChannels            = AudioChan ? 2 : 1;
    pcm.wf.nSamplesPerSec       = SampleRate;
    pcm.wBitsPerSample          = (AudioRes ? 16 : 8);
    pcm.wf.nBlockAlign          = (pcm.wBitsPerSample / 8) * pcm.wf.nChannels;
    pcm.wf.nAvgBytesPerSec      = pcm.wf.nSamplesPerSec * pcm.wf.nBlockAlign;

    // Open the device driver for recording
    rc = OpenRecordDevice();
	if (!rc) return;

	// Try to open output data file
    static char Buffer[256];
    rc = GetOutputFileName(Buffer, sizeof Buffer, "", "", "Waveform Files (*.WAV)\0*.WAV\0");
    if (!rc)
    {
        CloseRecordDevice();
        return;  // Error No file selected
    }

	// Save filename for window title, etc.
    FileName = Buffer;
    IndexFileName = FileName.Left(FileName.GetLength()-4) + ".LOG";

	// Open the output file
    rc = OpenRecordFile(Buffer);
    if (rc)
	{	// Error, could not open file
        CloseRecordDevice();
		return;
	}

    if (hindex) SendMessage(hindex, CLEAR_LOG_DISPLAY, 0, 0);

    CString str;
    str.Format("Ready - %s", FileName);
    SetWindowText(hwndd, str);

	// Prepare some buffers and give them to the device driver
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);
    AddRecbuff(ReadyQueue);

    PrepareRecbuff(ReadyQueue);   // 5 spares
    PrepareRecbuff(ReadyQueue);
    PrepareRecbuff(ReadyQueue);
    PrepareRecbuff(ReadyQueue);
    PrepareRecbuff(ReadyQueue);

    DevState = DEV_RECRDY;  // indicate recorder ready to use

    // Handle button states after Open button
    EnableWindow(GetDlgItem(hwndd, (int) ID_FILE_EXIT),         FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_RECORD),           TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_STOP),             FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_OPENWAVE),   FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_CLOSEWAVE),  TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_COMBO_SMP_RATE),   FALSE);
    EnableMenuItem(hmenu, ID_DEVICE_OPENWAVE, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hmenu, ID_DEVICE_CLOSEWAVE, MF_ENABLED);
    ButtonState = IDLE;

    return;
}





int OpenRecordDevice(void)
{
    UINT rc;

	trace("Calling waveInOpen()\n");

    rc = waveInOpen
    (
	    (LPHWAVEIN) &hwavein,
	    WAVE_MAPPER,
	    (LPCWAVEFORMATEX) &pcm.wf,
	    (DWORD_PTR) &WaveInProc,
	    0,
	    CALLBACK_FUNCTION
	);

	trace("waveInOpen result = %d\n", rc);
    if (rc != MMSYSERR_NOERROR)
	{   // if error from query
        waveInGetErrorText(rc, errortext, sizeof errortext);
        trace("Message Text = '%s'\n", errortext);
        MessageBox(hwndd, errortext, "MM System Error", MB_APPLMODAL | MB_OK);
        return FALSE;       // return w/error result
    }
    return TRUE;
}

void CloseRecordDevice()
{
    MMRESULT rc;
	trace("Calling waveInReset()\n");
    rc = waveInReset((HWAVEIN) hwavein);
	trace("waveInReset result = %d\n", rc);
    if (rc != MMSYSERR_NOERROR)
	{
        waveInGetErrorText(rc, errortext, sizeof errortext);
        trace("Message Text = '%s'\n", errortext);
        MessageBox(hwndd, errortext, "MM System Error", MB_APPLMODAL | MB_OK);
	}

	trace("Calling waveInClose()\n");
    rc = waveInClose((HWAVEIN) hwavein);
	trace("waveInClose result = %d\n", rc);
    if (rc != MMSYSERR_NOERROR)
	{
        waveInGetErrorText(rc, errortext, sizeof errortext);
        trace("Message Text = '%s'\n", errortext);
        MessageBox(hwndd, errortext, "MM System Error", MB_APPLMODAL | MB_OK);
	}
    
    hwavein = 0;                // prevent accidental use
}



void CALLBACK WaveInProc(HWAVE  hWave, UINT  uMsg, DWORD  dwInstance, DWORD  dwParam1, DWORD  dwParam2)
{
    SYSTEMTIME time;
    RECBUFF *rb;
    LPWAVEHDR pWH;

    switch(uMsg)
    {
        case WIM_OPEN:
        {
			trace("WaveInProc: WIM_OPEN\n");
            PostMessage(hwnd, MM_WIM_OPEN, (WPARAM) hWave, dwParam1); // pass to main wnd for processing
            break;
        }

        case WIM_CLOSE:
        {
			trace("WaveInProc: WIM_CLOSE\n");
            PostMessage(hwnd, MM_WIM_CLOSE, (WPARAM) hWave, dwParam1); // pass to main wnd for processing
            break;
        }

        case WIM_DATA:
        {
			trace("WaveInProc: WIM_DATA\n");

// time stamp the RB's time field
            GetLocalTime(&time);
            pWH = (LPWAVEHDR) (DWORD_PTR) dwParam1;      // access the WAVEHDR
            rb = (RECBUFF *) pWH->dwUser;    // access the RECBUFF
            rb->time = time;                // remember what time the buffer arrived

            PostMessage(hwnd, MM_WIM_DATA, (WPARAM) hWave, dwParam1); // pass to main wnd for processing
            break;
        }

        default:
        {
            DebugBreak();
            break;
        }
    }
}




int RecordStart(void)
{
    UINT rc;
    char errortext[256];

    if (DevState != DEV_RECRDY) return 2;

    SquelchInit();                      // Set squelch subsystem to a known state

	trace("Calling waveInStart()\n");
    if ((rc = waveInStart((HWAVEIN) hwavein)) != 0)
    {
        waveInGetErrorText(rc, errortext, sizeof errortext);
        trace("Message Text = '%s'\n", errortext);
        MessageBox(hwndd, errortext, "MM System Error", MB_APPLMODAL | MB_OK);
        return 1;           // return w/error result
    }
    DevState = DEV_REC;     // inidicate busy recording

    // Handle button states after Start button
    EnableWindow(GetDlgItem(hwndd, (int) ID_FILE_EXIT),         FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_RECORD),           FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_STOP),             TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_OPENWAVE),   FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_CLOSEWAVE),  FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_COMBO_SMP_RATE),   FALSE);
    EnableMenuItem(hmenu, ID_DEVICE_OPENWAVE, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hmenu, ID_DEVICE_CLOSEWAVE, MF_DISABLED | MF_GRAYED);
    ButtonState = RECORDING;

    CString str;
    str.Format("Recording - %s", FileName);
    SetWindowText(hwndd, str);

    return 0;           // successful
}



int RecordStop(void)
{
    UINT rc;
    char errortext[256];

    if (DevState != DEV_REC) return 2;
	trace("Calling waveInStop()\n");
    rc = waveInStop((HWAVEIN) hwavein);
    if (rc != 0)
    {
        waveInGetErrorText(rc, errortext, sizeof errortext);
        trace("Message Text = '%s'\n", errortext);
        MessageBox(hwndd, errortext, "MM System Error", MB_APPLMODAL | MB_OK);
        return 1;       // return w/error result
    }
    DevState = DEV_RECRDY;  // indicate ready (paused)

    // Handle button states after Stop button
    EnableWindow(GetDlgItem(hwndd, (int) ID_FILE_EXIT),         FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_RECORD),           TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_STOP),             FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_OPENWAVE),   FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_CLOSEWAVE),  TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_COMBO_SMP_RATE),   FALSE);
    EnableMenuItem(hmenu, ID_DEVICE_OPENWAVE, MF_DISABLED | MF_GRAYED);
    EnableMenuItem(hmenu, ID_DEVICE_CLOSEWAVE, MF_ENABLED);
    ButtonState = PAUSED;

    CString str;
    str.Format("Paused - %s", FileName);
    SetWindowText(hwndd, str);

//  EnableWindow(GetDlgItem(hwndd, (int) IDC_RECORD), TRUE);
//  EnableWindow(GetDlgItem(hwndd, (int) IDC_PLAY), TRUE);
//  EnableWindow(GetDlgItem(hwndd, (int) IDC_STOP), FALSE);
//  EnableMenuItem(hmenu, ID_DEVICE_OPENWAVE, MF_DISABLED | MF_GRAYED);
//  EnableMenuItem(hmenu, ID_DEVICE_CLOSEWAVE, MF_ENABLED);

    SquelchStop();

    return 0;           // successful
}



// return all storage for recording (make sure none are active or else unpredictable)
int RecordReset(Queue &ReadyQueue)
{
    SquelchReset(ReadyQueue);     // Flushes output buffer to file and returns any buffers held by the squelch subsystem
    
    DevState = DEV_RESET;

    CloseRecordDevice();

// loop to return all recycled RECBUFFs
    for (;ReadyQueue.Count() != 0;)
    {   // unprepare all headers

        FreeRecbuff((RECBUFF *) ReadyQueue.Deq());  // return storage
    }

//  DeleteCriticalSection(&ReadyQueue.cs);    

    // Handle button states after Close button
    EnableWindow(GetDlgItem(hwndd, (int) ID_FILE_EXIT),         TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_RECORD),           FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_STOP),             FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_OPENWAVE),   TRUE);
    EnableWindow(GetDlgItem(hwndd, (int) ID_DEVICE_CLOSEWAVE),  FALSE);
    EnableWindow(GetDlgItem(hwndd, (int) IDC_COMBO_SMP_RATE),   TRUE);
    EnableMenuItem(hmenu, ID_DEVICE_OPENWAVE,  MF_ENABLED );
    EnableMenuItem(hmenu, ID_DEVICE_CLOSEWAVE,  MF_DISABLED | MF_GRAYED);
    ButtonState = IDLE;

    CloseRecordFile();  // close the .WAV output file
    
    SetWindowText(hwndd, "Recorder Idle");

    return 0;   // success
}



// recycles a used RECBUFF back onto the ready queue.
void RecycleRecbuff(RECBUFF *rb, Queue &ReadyQueue)
{
    if (DevState == DEV_REC || DevState == DEV_RECRDY)  // if recorder open
    {

        ReadyQueue.Enq((QUEUEITEM *) rb);   // place the buffer back into the ready queue
//      trace("RecycleRecbuff: RB Recycled, RB= %08lX, ReadyQueue.count= %d.\n", rb, ReadyQueue.Count);
    }
    else
    {   // recorder not ready, discard the RECBUFF
        FreeRecbuff(rb);        // return RECBUFF and its storage
        return;
    }

}



// Initialize a RECBUFF and all it's appendages
RECBUFF *AllocRecbuff(void) 
{
    HGLOBAL hglobaltemp;
    RECBUFF *rb;

    if ((hglobaltemp = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(RECBUFF))) == NULL)
    {
        trace("Error 1 in AllocRecbuff\n");
        DebugBreak();
    }
    if ((rb = (RECBUFF *) GlobalLock(hglobaltemp)) == NULL)
    {
        trace("Error 2 in AllocRecbuff\n");
        DebugBreak();
    }
    rb->hglobalRecbuff = hglobaltemp;    // save handle for later disposal

// Allocate the data buffer

    rb->data = rb->databuffer; // for compatibility until all referenced code uses single RECBUFF/databuffer

    InitWavehdr(rb);

    trace("AllocRecbuff: RB= %08lX Allocated, RBAllocCount= %d.\n", (DWORD_PTR) rb, ++RBAllocCount);

    return rb;
}

// Initialize a WAVEHDR
void InitWavehdr(RECBUFF *rb)
{
    memset(&rb->wavehdr, 0, sizeof (WAVEHDR));   // clear the area to zero
    rb->wavehdr.lpData = (LPSTR) rb->data;
    rb->wavehdr.dwBufferLength = BUFFERSIZE;
    rb->wavehdr.dwUser = (DWORD_PTR) rb;      // used by callback to access this RECBUFF
}



// Return a RECBUFF and all it's appendages
void FreeRecbuff(RECBUFF *rb)
{
	char errortext[256];
    UINT rc;
    HGLOBAL temp, temp1;


    if (hwavein != NULL)
	{
		if (rb->wavehdr.dwFlags & WHDR_PREPARED)
		{
			trace("Calling waveInUnprepareHeader()\n");
			rc = waveInUnprepareHeader((HWAVEIN) hwavein, &rb->wavehdr, sizeof (WAVEHDR));
			trace("waveInUnprepareHeader result = '%d'\n", rc);
			if (rc != MMSYSERR_NOERROR)
			{
				waveInGetErrorText(rc, errortext, sizeof errortext);
				trace("FreeRecbuff: waveInUnprepareHeader result: %s\n", errortext);
				CString str;
				str.Format("UnPrepare error in FreeRecbuff. \nResult = '%s'", errortext);
				::MessageBox(NULL, str, "ScanRec", MB_OK);
				FatalExit(999);
			}
		}
	}

    temp    = rb->hglobalRecbuff;  // pull our handle out of buffer to be axed
//  rc      = GlobalUnlock(rb->hglobaldata);
//  temp1   = GlobalFree(rb->hglobaldata);
    rc      = GlobalUnlock(temp);
    temp1   = GlobalFree(temp);

    trace("FreeRecbuff: RB= %08lX Returned, RBAllocCount= %d.\n", (DWORD_PTR) rb, --RBAllocCount);
}






// prepare a RECBUFF and store it in the record ready queue
void PrepareRecbuff(Queue &ReadyQueue)
{
    RECBUFF *rb;
	MMRESULT rc;
	char errortext[256];
    
    rb = AllocRecbuff();    // get a new RECBUFF

	if (!(rb->wavehdr.dwFlags & WHDR_PREPARED))
	{
		trace("Calling waveInPrepareHeader()\n");
		rc = waveInPrepareHeader((HWAVEIN) hwavein, &rb->wavehdr, sizeof (WAVEHDR));
		trace("PrepareRecbuff: waveInPrepareHeader result = '%d'.\n", rc);
		if (rc != MMSYSERR_NOERROR)
		{
			waveInGetErrorText(rc, errortext, sizeof errortext);
			trace("PrepareRecbuff: waveInPrepareHeader result: %s\n", errortext);
			CString str;
			str.Format("Prepare error in PrepareRecbuff. \nResult = '%s'", errortext);
			::MessageBox(NULL, str, "ScanRec", MB_OK);
			FatalExit(999);
		}
	}

    ReadyQueue.Enq((QUEUEITEM *) rb);   // place the buffer into the ready queue
}


// sends a prepared WAVEHDR to the device driver
void AddRecbuff(Queue &ReadyQueue)
{
    RECBUFF *rb;
    MMRESULT rc;
    char errortext[256];
    
// see if any available recbuffs in the ready list

    if (ReadyQueue.Count() == 0)
    {
        trace("AddRecbuff: No ready buffers, Allocating our own.\n");
        PrepareRecbuff(ReadyQueue);  // if none ready, make one.
    }

    rb = (RECBUFF *) ReadyQueue.Deq();
//  trace("AddRecbuff: RB Dequeued, RB= %08lX, ReadyQueue.count= %d.\n", rb, ReadyQueue.Count());

//  trace("AddRecbuff: Passing RB to device, RB= %08lX, DevQCnt= %d.\n", rb, ++DevQCnt);

// Pass a recycled WAVEHDR on down to the device driver

	if (!(rb->wavehdr.dwFlags & WHDR_PREPARED))
	{
		trace("Calling waveInPrepareHeader()\n");
		rc = waveInPrepareHeader((HWAVEIN) hwavein, &rb->wavehdr, sizeof (WAVEHDR));
		trace("AddRecbuff: waveInPrepareHeader result = '%d'.\n", rc);
		if (rc != MMSYSERR_NOERROR)
		{
			waveInGetErrorText(rc, errortext, sizeof errortext);
			trace("Message Text = '%s'\n", errortext);
			CString str;
			str.Format("Prepare error in AddRecbuff. \nResult = '%s'", errortext);
			::MessageBox(NULL, str, "ScanRec", MB_OK);
			FatalExit(999);
		}
	}

	//trace("Calling waveInAddBuffer()\n");
    if ((rc = waveInAddBuffer((HWAVEIN) hwavein, &rb->wavehdr, sizeof (WAVEHDR))) != 0)
    {
        waveInGetErrorText(rc, errortext, sizeof errortext);
        trace("Message Text = '%s'\n", errortext);
        MessageBox(hwndd, errortext, "MM System Error", MB_APPLMODAL | MB_OK);
    }
}

// I believe the 8 bit calibration bug is in here.
void ScanAvg(RECBUFF* Rb, PSHORT Left, PSHORT Right)
{
	double avgL = 0.0;
	double avgR = 0.0;
	int i, j;
	DWORD samples;	// # of samples in the buffer

	_ASSERT(Rb->wavehdr.dwBytesRecorded > 0);

	samples = (Rb->wavehdr.dwBytesRecorded / pcm.wf.nBlockAlign);

	if (GetBitsPerSample() == 16)
	{	// 16 bits / channel
		PSHORT ptr = (PSHORT) Rb->wavehdr.lpData;

		// Get the average for one buffer
		for (i = 0, j = Rb->wavehdr.dwBytesRecorded / 2; i < j; i++, ptr++)
		{
			avgL += *ptr;	// Total the Left channel values

			if (IsStereo())
			{
				i++;
				ptr++;
				avgR += *ptr;	// Total the Right channel values
			}
		}
	}
	else
	{	// 8 bits
		PBYTE ptr = (PBYTE) Rb->wavehdr.lpData;

		// Get the average for one buffer
		for (i = 0, j = Rb->wavehdr.dwBytesRecorded; i < j; i++, ptr++)
		{
			avgL += Samp8To16(*ptr);	// Total the Left channel values

			if (IsStereo())
			{
				i++;
				ptr++;
				avgR += Samp8To16(*ptr);	// Total the Right channel values
			}
		}
	}

	// Get the running average DC bias for each channel

	avgL /= samples;
	avgR /= samples;


	// Store the new avg biases.
	*Left = (SHORT) avgL;
	*Right = (SHORT) avgR;
}

void ApplyDCOffset(RECBUFF* Rb)
{
	int i, j;
	long val;

	ASSERT(Rb->wavehdr.dwBytesRecorded > 0);
	

	if (GetBitsPerSample() == 16)
	{	// 16 bits
		PSHORT ptr = (PSHORT) Rb->wavehdr.lpData;
		for (i = 0, j = Rb->wavehdr.dwBytesRecorded / 2; i < j; i++, ptr++)
		{
			// Adjust offset and apply limit if overflow.
			// Left (mono)
			val = *ptr - calSettings.DCCal16L;
			val = min(val,32767);
			val = max(val,-32768);
			*ptr = (SHORT) val;

			if (IsStereo())
			{	// Right
				i++;
				ptr++;
				val = *ptr - calSettings.DCCal16R;
				val = min(val,32767);
				val = max(val,-32768);
				*ptr = (SHORT) val;
			}
		}
	}
	else
	{	// 8 bits
		PBYTE ptr = (PBYTE) Rb->wavehdr.lpData;

		for (i = 0, j = Rb->wavehdr.dwBytesRecorded; i < j; i++, ptr++)
		{
			// Adjust offset and apply limit if overflow.
			// Left (mono)
			val = Samp8To16(*ptr);
			val -= calSettings.DCCal16L;
			val = min(val,32767);
			val = max(val,-32768);

			*ptr = (BYTE) Samp16To8((SHORT)val);

			if (IsStereo())
			{	// Right
				i++;
				ptr++;
				val = Samp8To16(*ptr);
				val -= calSettings.DCCal16R;
				val = min(val,32767);
				val = max(val,-32768);
				*ptr = (BYTE) Samp16To8((SHORT)val);
			}
		}
	}
}

void ScanForLevels(RECBUFF* rb)
{
	// Update the envelope values (8 bit sample size)

	if (pcm.wBitsPerSample == 8)
	{
		EnvlPos = 0;		// reset values
		EnvlNeg = 255;
		BYTE *ptr;          // current buffer character pointer
		UINT i;

		// scan buffer for highest and lowest data values
		for(i = 0, ptr = (PBYTE) rb->data; i < rb->wavehdr.dwBytesRecorded; i++, ptr++)
		{
			EnvlNeg = (min(*ptr, EnvlNeg));
			EnvlPos = (max(*ptr, EnvlPos));
		}
	}
	else
	{	// 16 bits / channel
		SHORT posVal = 0, negVal = 0;
		SHORT *ptr;
		DWORD i;

		// scan buffer for highest (positive) and lowest (neg) data values.
		for (i = 0, ptr = (PSHORT) rb->data; i < (rb->wavehdr.dwBytesRecorded)/2; i++, ptr++)
		{
			posVal = (max(posVal, *ptr));
			negVal = (min(negVal, *ptr));
		}
		
		EnvlPos = Samp16To8(posVal);
		EnvlNeg = Samp16To8(negVal);
	}
}

// Clears all calibration data to zero.
void DCCalReset()
{
	calSettings.DCCal16L = 0;
	calSettings.DCCal16R = 0;
	calSettings.DCAvgIdx = 0;

	// Clear the average buffers.
	for (int i = 0; i < DC_AVG_CNT; i++)
	{
		calSettings.DCAvg16L[i] = 0;
		calSettings.DCAvg16R[i] = 0;
	}
}

int GetBitsPerSample()
{
	return ((pcm.wf.nBlockAlign / pcm.wf.nChannels) == 1) ? 8 : 16;
}

BOOL IsStereo()
{
	return pcm.wf.nChannels == 2;
}

BOOL Is8Bit()
{
	return (GetBitsPerSample() == 8);
}
