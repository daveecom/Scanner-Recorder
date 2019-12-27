/*********************************************************************
* 04/21/95															 *
* All-Day Audio Recorder squelch detection and data storage.		 *
*																	 *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*																	 *
*********************************************************************/

#include "AdarPch.h"

#define SKIPBACK_TIME 30				// amount of time to back up to avoid chopping (in ms)
#define SQUELCH_DELAY 1 				// amount of quiet time before squelch activated (in seconds)

long SampleCount;						// Counts number of captured samples (maintained in ProcessSquelch).
										// Used for elapsed time indicator.
RECBUFF *BufferB;						// address of secondary audio data input buffer
long SampleStoredCount; 				// number of samples stored in file after squelch (maintained by WriteSample)
DWORD SquelchCount;						// counts from end of sound to beginning of squelch
int  SquelchState = SQUELCHED;			// indicates whether squelching is in effect
extern int	SquelchThresh;				// Squelch Threshold (0 - 100 %)
DWORD SquelchDelayTimeMs;				// Milliseconds to wait before closing squelch.
DWORD SquelchDelayCount;				// number of samples below threshold before state changes to SQUELCHED

// Output buffer for writing squelched data
#define OUTBUFFSIZE 32768				// sample's output buffer size
UCHAR Outbuff[OUTBUFFSIZE]; 			// output buffer
UCHAR *OutbuffPtr8 = (UCHAR *) &Outbuff;// buffer pointer
ULONG OutbuffCount = 0; 				// output buffer count

DWORD Duration = 0; 					// Duration of squelch event.
DWORD MarkIn   = 0; 					// Set to sample # at beginning of duration
SYSTEMTIME SquelchOnTime = {0};			// Time stamp when squelch opened.

// Call this function before sending any buffers to the Squelch Processor
void SquelchInit(void)
{
	trace("SquelchInit Entered\n");
	SquelchCount = 0;
	SquelchTransition(SQUELCHED);	//
	SquelchDelayTimeMs = GetPrivateProfileInt(IniSectionName, "SquelchDelayTimeMs", 1000, IniName);
//	SquelchDelayCount = (DWORD) ((double) ((SquelchDelayTimeMs / 1000)) * (double) SampleRate);
	OutbuffPtr8  = (UCHAR *) &Outbuff;
	OutbuffCount = 0;
	BufferB = NULL;
}


// Handle the stop button while recording
void SquelchStop(void)
{
	trace("SquelchStop Entered\n");
	SquelchCount = 0;
	SquelchTransition(SQUELCHED);			// squelch
	if (Duration > 0)
	{
		WriteIndex();
		Duration = 0;
	}
}


// Called prior to closing output disk file
void SquelchReset(Queue &ReadyQueue)
{
	trace("SquelchReset Entered\n");
	SquelchFlush(); 	// flush leftover data to file

// recycle the BufferB if present
	if (BufferB != NULL)
	{
		RecycleRecbuff(BufferB, ReadyQueue);
		BufferB = NULL;
	}
	SampleCount 	  = 0;				// counts number of samples recorded (maintained in ProcessSquelch)
	SampleStoredCount = 0;				// number of samples stored in file after squelch (maintained by WriteSample)
	SquelchCount = 0;
	SquelchTransition(SQUELCHED);		//
}




// centralizes control of the SquelchState variable and invalidates the light area
_inline void SquelchTransition(int state)
{
	SquelchState = state;
	InvalidateRect(hwndd, &dlgsqllite, FALSE); // flag light to be updated
	trace("SquelchState changed to %d\n", state);
	if (state == UNSQUELCHED) GetLocalTime(&SquelchOnTime);
}




void ProcessSquelch(RECBUFF *BufferA, Queue &ReadyQueue)
{
	ProcessSquelchNew(BufferA);
}

// Given the address of a RECBUFF, determine if data should be squelched or not and output to WriteSample function.
void ProcessSquelchNew(RECBUFF* BufferA)
{
	double	thresh;								// used to compute threshold values
	double	leftThresh;
	double	rightThresh;
	DWORD	numSamples;							// # samples in buffer. derived from dwBytesRecorded
	DWORD	loopCount;							// Used to count loops (copy of numSamples)
	PBYTE	pSample;							// pointer to current sample
	PBYTE	pSample8;							// Access to 8 bit sample data.
	PSHORT	pSample16;							// Access to 16 bit sample data.
	int		sampleSize;							// # bytes per sample. (1 - 4)
	BOOL	isAboveL;							// Indicates the sample exceeds the squelch.
	BOOL	isAboveR;							// Indicates the sample exceeds the squelch.
	BOOL	antiClipEnabled;					// Anti clipping is turned on.
	BOOL	pipEnabled;							// Pip tone activate.
	static CString squelchChannel;				// Used to add symbol to log for channel that broke the squelch.

NANOBEGIN

	antiClipEnabled	= GetPrivateProfileInt(IniSectionName, "AntiClip", 1, IniName);
	pipEnabled		= GetPrivateProfileInt(IniSectionName, "pip", 0, IniName);

	SquelchDelayCount = (DWORD) (((double) (((double) SquelchDelayTimeMs / 1000)) * (double) SampleRate)) & (0xffffffff - pcm.wf.nBlockAlign);
	_ASSERT(SquelchDelayCount);

	// Compute the squelch threshold 16 bit absolute value
	thresh = ((double) SquelchThresh / 100) * 32768;
	leftThresh = rightThresh = thresh;					// Stub for future split channel operation.

	// compute the number of samples in the buffer
	sampleSize	= pcm.wf.nBlockAlign;								// # of samples
	numSamples	= BufferA->wavehdr.dwBytesRecorded / sampleSize;	// get number of samples

	// Prepare for scanning loop
	pSample = (PBYTE) BufferA->data; 						// Get address of start of data

	// Scan entire buffer and write unsquelched data to output file (high speed loop, efficiency is critical)
	//for(pSample = BufferA->wavehdr.lpData,
	//	loopCount = numSamples;
	//	loopCount > 0;
	//	pSample = (PVOID) ((DWORD) sampleSize + (DWORD) pSample),
	//	SampleCount++, loopCount--)

	for(pSample = (PBYTE) BufferA->wavehdr.lpData,
		loopCount = numSamples;
		loopCount > 0;
		pSample = ( sampleSize +  pSample),
		SampleCount++, loopCount--)
	{
		pSample8	= (PBYTE)	pSample;
		pSample16	= (PSHORT)	pSample;

		// Check if above the squelch threshold here
		isAboveL = isAboveR = FALSE;	// Reset trigger flag
		if (IsStereo())
		{	// Check stereo values
			SHORT valueLeft		= (Is8Bit() ? Samp8To16(*pSample8)		: *pSample16);
			SHORT valueRight	= (Is8Bit() ? Samp8To16(*(pSample8+1))	: *(pSample16+1));

			isAboveL	= abs(valueLeft)		>= leftThresh;
			isAboveR	= abs(valueRight)		>= rightThresh;

			if (isAboveL)
				squelchChannel = "   Left";
			else if (isAboveR)
				squelchChannel = "   Right";
		}
		else
		{	// Check mono values
			isAboveL = (Is8Bit() ? Samp8To16(*pSample8):*pSample16) >= thresh;
		}

		if (isAboveL || isAboveR) // if sample above squelch threshold
		{
			SquelchCount = 0;						// reset squelch counter
			if (SquelchState == SQUELCHED)
			{
				SquelchTransition(UNSQUELCHED); 	// unsquelch
				MarkIn = SampleStoredCount; 		// Used for log display

				if (pipEnabled)
				{
					// Insert a pip tone

					DWORD  i;
					PVOID  pBuffer;
					PBYTE  pBuffer8;
					PSHORT pBuffer16;
					DWORD  countBytes;	// # size of returned buffer
	
					CreateSinewave(2500, .05, .5, &wfSrc, &pBuffer, &countBytes);
					ASSERT(pBuffer);

					if (Is8Bit())
					{
						for(i = 0, pBuffer8 = (PBYTE) pBuffer; i < countBytes;i++, pBuffer8++)
						{
							WriteSample8(*pBuffer8);
						}
					}
					else
					{	// 16 bit
						for(i = 0, pBuffer16 = (PSHORT) pBuffer; i < countBytes / 2;i++, pBuffer16++)
						{
							WriteSample16M(*pBuffer16);
						}
					}
	
					delete pBuffer;

					SampleStoredCount += (countBytes / pcm.wf.nBlockAlign);

				}

// write previous few milliseconds' samples up to the current sample (exclusive)
				if (antiClipEnabled) WritePreviousAudio(BufferA, BufferB, pSample);

			} // End (SquelchState == SQUELCHED)
			
			// Write the current sample to the file.
			if (IsStereo())
			{	// Stereo
				if (Is8Bit())
				{
					WriteSample8(*pSample8);
					WriteSample8(*(pSample8+1));
				}
				else
				{
					WriteSample16M(*pSample16);
					WriteSample16M(*(pSample16+1));
				}
			}
			else
			{	// Mono
				if (Is8Bit())
					WriteSample8(*pSample8);
				else
					WriteSample16M(*pSample16);
			}
			SampleStoredCount++;											// statistics
			Duration++;

		}		 
		else	// sample below squelch threshold
		{
			if (SquelchState == UNSQUELCHED)
			{

				// Write the current sample to the file.
				if (IsStereo())
				{	// Stereo
					if (Is8Bit())
					{
						WriteSample8(*pSample8);
						WriteSample8(*(pSample8+1));
					}
					else
					{
						WriteSample16M(*pSample16);
						WriteSample16M(*(pSample16+1));
					}
				}
				else
				{	// Mono
					if (Is8Bit())
						WriteSample8(*pSample8);
					else
						WriteSample16M(*pSample16);
				}
				SampleStoredCount++;											// statistics
				Duration++;

				if (SquelchCount++ >= SquelchDelayCount)	// if terminal count reached on delay time
				{
					SquelchTransition(SQUELCHED);			// squelch

					WriteIndex(squelchChannel);
					squelchChannel.Empty();

					Duration = 0;
				}
			}
			else	// Squelched
			{
				;	// currently nothing needs to be done here while we're squelched and below the threshold
			}
		}
	} // End of scanning loop

// If not first time here, recycle last buffer B
	if (BufferB != NULL) RecycleRecbuff(BufferB, ReadyQueue);

// save current buffer for possible skip-back during unsquelching
	BufferB = BufferA;						

NANOEND
}


// Write the samples from a few milliseconds ago up to the current buffer position
// in order to avoid cutting off the beginning of the sound that broke the squelch
// This function handles 8 & 16 bits, mono and stereo.
void WritePreviousAudio(RECBUFF *BufferA, RECBUFF *BufferB, PBYTE Sample)
{
	PBYTE StartA, StartB, EndA, EndB;				// pointers to mark skipback boundries
	PBYTE Bptr; 									// used in loops to point to current sample
	DWORD count;									// Total # of skipback bytes to write
	DWORD CountA, CountB;							// # bytes to write from each buffer
	DWORD i;
	DWORD Total;									// Used to compute # Samples written.
	BOOL OKB = (BufferB != NULL);					// prevents crash if no previous buffer

// For debug, verify the Sample pointer is within BufferA's valid data range.
	if
	(!
		(Sample >= BufferA->data) &&
		(Sample < (PBYTE)BufferA->data + BufferA->wavehdr.dwBytesRecorded)
	)
		ASSERT(0);

// compute how many samples to skip back
	count = (SKIPBACK_TIME * (SampleRate / 1000)) * pcm.wf.nBlockAlign;	// # of bytes to skip back

// compute boundry pointers

	StartA = (PBYTE) BufferA->data;						// mark start of current buffer
	EndA = (PBYTE) Sample;								// set end of skipback boundry = current pos
	CountA = (DWORD) (EndA - StartA); 					// compute # bytes to write from BufferA

	if (count <= CountA)
	{	// no need to cross buffer boundries
		StartA = EndA - count;							// re-mark start of skipback data
		CountA = (DWORD) (EndA - StartA);				// re-set count
		CountB = 0; 									// prevent processing previous buffer
	}
	else
	{	// if we have to back up to previous buffer
		if (OKB)
		{
			count -= CountA;								// account for distance backed up so far
			EndB = ((PBYTE) BufferB->data + BufferB->wavehdr.dwBytesRecorded);
			StartB = EndB - count;						// mark start of previous buffer
			CountB = (DWORD) (EndB - StartB);				// set counter for 1st loop
		}
		else	// no previous buffer present.
		{
			StartB = 0;
			EndB   = 0;
			CountB = 0;
		}
	}

// Now that we've calculated everything needed, it's just a matter of looping to write previous audio.

	trace("Sample Ptr: %0X\n", Sample);
	trace
	(
		"BufferA: %0X to %0X, StartA: %0X, EndA: %0X, CountA: %0X\n",
		BufferA->data,
		((PBYTE) BufferA->data + BufferA->wavehdr.dwBytesRecorded - 1),
		StartA,
		EndA,
		CountA
	);
	
	// CountA and CountB represent the # of BYTES to write to the output file.

	if (CountB)
	{
		trace
		(
			"BufferB: %0X to %0X, StartB: %0X, EndB: %0X, CountB: %0X\n",
			BufferB->data,
			((PBYTE) BufferB->data) + BufferB->wavehdr.dwBytesRecorded - 1,
			StartB,
			EndB,
			CountB
		);
	}

// If necessary, write previous buffer's data (1 byte at a time)
	for (Bptr = (PBYTE) StartB, i = 0; i < CountB; i++, Bptr++)
	{
		WriteAudioByte(*Bptr); 							// write a byte
	}

// write current buffer's data up to the current sample position
	for (Bptr = (PBYTE) StartA, i = 0; i < CountA; i++, Bptr++)
	{
		WriteAudioByte(*Bptr); 							// write a sample
	}

	// Update stat counters
	Total = CountA + CountB;
	Total /= pcm.wf.nBlockAlign;	// Convert total # bytes to total # of samples.

	SampleStoredCount	+= Total;
	Duration			+= Total;
}

// Write one byte of sample data to the output file via a buffer
_inline void WriteAudioByte(BYTE Sample)
{

	if (OutbuffCount >= OUTBUFFSIZE)
	{	// Buffer full
		WriteToRecordFile((const char *) &Outbuff, OutbuffCount);	// write the full buffer
		OutbuffPtr8 = (BYTE *) &Outbuff;							// reset buffer pointer
		OutbuffCount = 0;											// make buffer empty
	}
	*OutbuffPtr8++ = Sample;										// store a sample in the buffer
	OutbuffCount++; 												// bump count

}


// Write a sample to the output file via a buffer
_inline void WriteSample8(UCHAR Sample)
{
	WriteAudioByte(Sample);
}




// Write a sample to the output file via a buffer
_inline void WriteSample16M(SHORT Sample)
{
	WriteAudioByte(LOBYTE(Sample));
	WriteAudioByte(HIBYTE(Sample));
}




// Flush the remaining samples from the output buffer (called by SquelchReset)
void SquelchFlush(void)
{
	WriteToRecordFile((const char *) &Outbuff, OutbuffCount);	// write the full buffer
	OutbuffPtr8 = (UCHAR *) &Outbuff;					// reset buffer pointer
	OutbuffCount = 0;											// make buffer empty
}



// Write an index to the index file (If ExtraString is !NULL, append it to the message.
void WriteIndex(LPCTSTR ExtraString)
{
	TM t, t1;
	SYSTEMTIME time = {0};
	CString str, msg;
	CFileException e;
	double durSeconds;

	time = SquelchOnTime;
	ConvertSamplesToTime6(Duration, SampleRate, &t);
	ConvertSamplesToTime6(MarkIn, SampleRate, &t1);
	durSeconds = (double) Duration / (double) SampleRate;

	str.Format
	(
		"%04d/%02d/%02d,   %02d:%02d:%02d,    %08.1f,    %02X:%02X:%02X%s\r\n",
//		"%04d/%02d/%02d,   %02d:%02d:%02d,   %02X:%02X:%02X,   %02X:%02X:%02X\r\n",
		time.wYear,
		time.wMonth,
		time.wDay,
		time.wHour,
		time.wMinute,
		time.wSecond,
		
		durSeconds,

//		t.hmsf.c2,		// hh
//		t.hmsf.c1,		// mm
//		t.hmsf.c0,		// ss
		
		t1.hmsf.c2, 	// hh
		t1.hmsf.c1, 	// mm
		t1.hmsf.c0,		// ss

		(strlen(ExtraString) > 0) ? ExtraString : ""
	);

retry:

	TRY
	{
		IndexFile.Write(str, str.GetLength());
		if (hindex) SendMessage(hindex, ADD_RECORD, 0, (LPARAM) (LPCTSTR) str);
	}
	CATCH(CFileException, e)
	{
		msg.Format("I/O Error %d during log file write.", e->m_cause);
		int rc = MessageBox(hwndd, msg, NULL, MB_ABORTRETRYIGNORE);
		switch(rc)
		{
			case IDRETRY:
				goto retry;

			case IDABORT:
				PostMessage(hwndd, WM_COMMAND, IDC_STOP, 0);			// Press Stop button
				PostMessage(hwndd, WM_COMMAND, ID_DEVICE_CLOSEWAVE, 0);	// Press Close button
				break;

			case IDIGNORE:
				break;
		}
	}
	END_CATCH

}


// Move the squelch slider to its correct position
void SetSquelchSlider(HWND h, int Value)
{
	HWND hS;	// slider handle
	LRESULT rc;

	hS = GetDlgItem(h, IDC_SLIDER_SQUELCH);
	_ASSERT(hS);
	rc = SendMessage(hS, TBM_SETPOS, TRUE, Value);	// Set the slider
}



// Return the current squelch slider value
int GetSquelchSlider(HWND h)
{
	LRESULT Value;
	HWND hS;	// slider handle

	hS = GetDlgItem(h, IDC_SLIDER_SQUELCH);
	_ASSERT(hS);
	Value = SendMessage(hS, TBM_GETPOS, 0, 0);	// Get the slider
	return (int) Value;
}
