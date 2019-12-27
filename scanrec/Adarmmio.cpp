/*********************************************************************
*                                                                    *
* All-Day Audio Recorder Multimedia File I/O Routines                *
*                                                                    *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*                                                                    *
*********************************************************************/

#include "AdarPch.h"

HMMIO hmmio_out;							// handle of output .WAV file
HMMIO hmmio_in;								// handle of input .WAV file

MMCKINFO InfoParent;						// info for parent chunk
MMCKINFO InfoFmt;							// info for fmt chunk
MMCKINFO InfoData;							// info for data chunk

CFile IndexFile;							// Index output file object
CFileException e;
CString msg;

// used for compression
HACMSTREAM			hStream = 0;
ACMSTREAMHEADER		StreamHdr;
WAVEFORMATEX		wfSrc = {0};
LPWAVEFORMATEX		wfDst = NULL;
DWORD				wfSizeMax = 0;			// Size of largest WAVEFORMATEX

// Open the .WAV output file and write the initial info into the file.
int OpenRecordFile(const char *fn)
{
    MMRESULT rc;
    char ErrorMessage[256];
    
	// Open the output .WAV file
    if ((hmmio_out = mmioOpen((char *) fn, NULL, MMIO_WRITE | MMIO_CREATE)) == NULL)
    {
        sprintf(ErrorMessage, "Can't open output file: '%s'", fn);
        MessageBox(hwndd, ErrorMessage, "File Open Error", MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }

// Set up an output compression stream

	rc = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &wfSizeMax);
	if (rc != 0)
	{
		CString str;
		str.Format("Error %d from acmMetrics(ACM_METRIC_MAX_SIZE_FORMAT). Tell author.", rc);
		MessageBox(hwndd, str, NULL, MB_OK);
		return 1;	// Fatal error exit
	}

	wfDst = (LPWAVEFORMATEX) new char[wfSizeMax];
	memset(wfDst, 0, wfSizeMax);		// clear the dest wf

	// Prepare the source WAVEFORMATEX
	wfSrc.wFormatTag		= pcm.wf.wFormatTag;
	wfSrc.nChannels			= pcm.wf.nChannels;
	wfSrc.nSamplesPerSec	= pcm.wf.nSamplesPerSec;
	wfSrc.nAvgBytesPerSec	= pcm.wf.nAvgBytesPerSec;
	wfSrc.nBlockAlign		= pcm.wf.nBlockAlign;
	wfSrc.wBitsPerSample	= pcm.wBitsPerSample;
	wfSrc.cbSize			= 0;

	// If compression enabled
	if (GetPrivateProfileInt(IniSectionName, "Compression", 1, IniName))
	{	// Compression enabled
		// Prepare for compression query dialog
		ACMFORMATCHOOSE	afc = {0};
		afc.cbStruct		= sizeof(ACMFORMATCHOOSE);
		afc.hwndOwner		= hwndd;
		afc.fdwEnum			= ACM_FORMATENUMF_CONVERT|ACM_FORMATENUMF_NCHANNELS/*|ACM_FORMATENUMF_SUGGEST*/;
		afc.pwfxEnum		= &wfSrc;					// This is our input format
		afc.pszTitle		= "Select compression format or PCM for no compression.";
		afc.pwfx			= wfDst;
		afc.cbwfx			= wfSizeMax;
		
		rc = acmFormatChoose(&afc);
		if (rc != MMSYSERR_NOERROR)
		{
			switch (rc)
			{
				case ACMERR_CANCELED:
					// "The user chose the Cancel button or the Close command on the System menu to close the dialog box.";
					break;

				case ACMERR_NOTPOSSIBLE:
					MessageBox(hwndd, "The buffer identified by the pwfx member of the ACMFORMATCHOOSE structure is too small to contain the selected format.", "", MB_OK);
					break;

				case MMSYSERR_INVALFLAG:
					MessageBox(hwndd, "At least one flag is invalid.", "", MB_OK);
					break;

				case MMSYSERR_INVALHANDLE:
					MessageBox(hwndd, "The specified handle is invalid.", "", MB_OK);
					break;

				case MMSYSERR_INVALPARAM:
					MessageBox(hwndd, "At least one parameter is invalid.", "", MB_OK);
					break;

				case MMSYSERR_NODRIVER:
					MessageBox(hwndd, "A suitable driver is not available to provide valid format selections.", "", MB_OK);
					break;

				case MMSYSERR_NOMEM:
					MessageBox(hwndd, "The system is unable to allocate resources.", "", MB_OK);
					break;
			}

			// Cleanup
			mmioClose(hmmio_out, 0);
			if (wfDst)
			{
				delete [] wfDst ;
				wfDst = NULL;
			}

			return 1;	// error
		}
	}
	else
	{	// Compression disabled
		*wfDst = wfSrc;	// Use source WF to prevent compression
	}

	// Open a compression stream if needed.
	if ((memcmp(wfDst, &wfSrc, sizeof (WAVEFORMATEX)) != 0) && (GetPrivateProfileInt(IniSectionName, "Compression", 1, IniName)) == 1)
	{	// Use converter if the WAVEFORMATEX of the source and dest are different.
		rc = acmStreamOpen(&hStream, NULL, &wfSrc,	wfDst, NULL, NULL, 0, ACM_STREAMOPENF_NONREALTIME);
		if (rc != MMSYSERR_NOERROR)
		{
			CString str;
			switch (rc)
			{
				case ACMERR_NOTPOSSIBLE:
					str = "The requested operation cannot be performed.";
					break;

				case MMSYSERR_INVALFLAG:
					str = "At least one flag is invalid.";
					break;

				case MMSYSERR_INVALHANDLE:
					str = "The specified handle is invalid.";
					break;

				case MMSYSERR_INVALPARAM:
					str = "At least one parameter is invalid.";
					break;

				case MMSYSERR_NODRIVER:
					str = "A suitable driver is not available to provide valid format selections.";
					break;

				case MMSYSERR_NOMEM:
					str = "The system is unable to allocate resources.";
					break;
			}
			str += "Compression not enabled.";
			MessageBox(hwndd, str, "", MB_OK);
		}
	}

	// Create the main WAVE chunk
    InfoParent.fccType = mmioStringToFOURCC("WAVE", 0);
    if ((rc = mmioCreateChunk(hmmio_out, &InfoParent, MMIO_CREATERIFF)) != 0) DebugBreak();

	// Create the fmt chunk

    InfoFmt.ckid = mmioStringToFOURCC("fmt", 0);
    rc = mmioCreateChunk(hmmio_out, &InfoFmt, 0);
    _ASSERT(rc == MMSYSERR_NOERROR);

	// Use the destination WAVEFORMATEX in the output file.
	rc = mmioWrite(hmmio_out, (const char*) wfDst, sizeof(WAVEFORMATEX)+wfDst->cbSize);
	_ASSERT(rc);

    if ((rc = mmioAscend(hmmio_out, &InfoFmt, 0)) != 0) DebugBreak();

	// Create the data chunk (just the header, not the data itself).
    InfoData.ckid = mmioStringToFOURCC("data", 0);
    if ((rc = mmioCreateChunk(hmmio_out, &InfoData, 0)) != 0) DebugBreak();

retry:

	TRY
	{
		// Prepare index file for output
		IndexFile.Open(IndexFileName, CFile::modeCreate|CFile::modeWrite|CFile::shareDenyWrite);
		CString str
		(
//           2000/04/27,   17:34:02,    00000.16,    00:00:00
			"Date          Time         Duration     Relative Time\r\n"\
			"------------------------------------------------\r\n"
		   //1997/05/03,   16:30:42,   00:00:03,   00:00:00
		);

		IndexFile.Write(str, str.GetLength());  // Write header
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

    return 0;   // Success
    SampleCount         = 0;
    SampleStoredCount   = 0;
}



// Close the output .WAV file
void CloseRecordFile(void)
{
    int rc;

	// Cleanup compression resources
	if (wfDst)
	{
		delete wfDst;
		wfDst = NULL;
	}
	if (hStream) acmStreamClose(hStream, 0);
	hStream = 0;

// Ascend out of the data subchunk
    if ((rc = mmioAscend(hmmio_out, &InfoData, 0)) != 0) DebugBreak();

// Ascend out of the RIFF parent chunk
    if ((rc = mmioAscend(hmmio_out, &InfoParent, 0)) != 0) DebugBreak();

// And finally, close the file
    if ((rc = mmioClose(hmmio_out, 0)) != 0) DebugBreak();

    IndexFile.Close();
}



// Write waveform audio data to the open .WAV file
void WriteToRecordFile(const char *data, LONG count)
{
    MMRESULT rc;
	ACMSTREAMHEADER hdr = {0};
	PVOID buff = NULL;
	DWORD outputBufferSize = count * 2;		// Use a larger output buffer in case the data expands instead of compresses.

	if (count)
	{
		if (hStream)
		{	// If compression active
			// Create compressed output buffer
			buff				= new char[outputBufferSize];
			hdr.cbStruct		= sizeof ACMSTREAMHEADER;
			hdr.dwUser			= 0;
			hdr.pbSrc			= (LPBYTE) data;
			hdr.cbSrcLength		= count;
			hdr.dwSrcUser		= 0;
			hdr.pbDst			= (LPBYTE) buff;
			hdr.cbDstLength		= outputBufferSize;
			hdr.dwDstUser		= 0;

			rc = acmStreamPrepareHeader(hStream, &hdr, 0);
			if (rc != MMSYSERR_NOERROR)
			{
				switch (rc)
				{
					case MMSYSERR_INVALFLAG:
						MessageBox(NULL, "Error in WriteToRecordFile: MMSYSERR_INVALFLAG", "", MB_OK);
						break;
					case MMSYSERR_INVALHANDLE:
						MessageBox(NULL, "Error in WriteToRecordFile: MMSYSERR_INVALHANDLE", "", MB_OK);
						break;
					case MMSYSERR_INVALPARAM:
						MessageBox(NULL, "Error in WriteToRecordFile: MMSYSERR_INVALPARAM", "", MB_OK);
						break;
					case MMSYSERR_NOMEM:
						MessageBox(NULL, "Error in WriteToRecordFile: MMSYSERR_NOMEM", "", MB_OK);
						break;
					case ACMERR_NOTPOSSIBLE:
						MessageBox(NULL, "Error in WriteToRecordFile: ACMERR_NOTPOSSIBLE", "", MB_OK);
						break;
					case ACMERR_BUSY:
						MessageBox(NULL, "Error in WriteToRecordFile: ACMERR_BUSY", "", MB_OK);
						break;
					case ACMERR_UNPREPARED:
						MessageBox(NULL, "Error in WriteToRecordFile: ACMERR_UNPREPARED", "", MB_OK);
						break;
					case ACMERR_CANCELED:
						MessageBox(NULL, "Error in WriteToRecordFile: ACMERR_CANCELED", "", MB_OK);
						break;
					default:
						MessageBox(NULL, "Error in WriteToRecordFile: Unknown", "", MB_OK);
						break;
				}
			}
			
			// Look into block alignment settings for performance
			rc = acmStreamConvert(hStream, &hdr, ACM_STREAMCONVERTF_BLOCKALIGN);
			_ASSERT(rc == MMSYSERR_NOERROR);

			rc = acmStreamUnprepareHeader(hStream, &hdr, 0);
			_ASSERT(rc == MMSYSERR_NOERROR);

			// Check hdr.cbDstLengthUsed;
			_ASSERT(hdr.cbDstLengthUsed);

			rc = mmioWrite(hmmio_out, (const char *) buff, hdr.cbDstLengthUsed);
			_ASSERT(rc > 0);

			delete buff;
		}
		else
		{	// not compressed
			rc = mmioWrite(hmmio_out, (LPSTR) data, count);
			_ASSERT(rc > 0);
		}
	}
}

// Use the COMDLG32.DLL's File|SaveAs dialog to get the name and directory of the output file

BOOL GetOutputFileName(char *Output, int OutputLength, char *lpszDir, char *lpszFilename, char *lpszFilter)
{
	OPENFILENAME ofn;
    static char szFile[256];       // Local szFile
    static char szFileTitle[256];
    
    int i;

    strcpy( szFile, "");            // Clear result areas
    strcpy( szFileTitle, "");


//  memset(&ofn, 0, sizeof(OPENFILENAME));
    ofn.lStructSize 		= sizeof(OPENFILENAME);
    ofn.hwndOwner 			= hwndd;
    ofn.hInstance 			= hinst;
    ofn.lpstrFilter 		= lpszFilter;
    ofn.lpstrCustomFilter 	= NULL;
    ofn.nMaxCustFilter 		= 0;
    ofn.nFilterIndex		= 1;
    ofn.lpstrFile			= szFile;
    ofn.nMaxFile			= sizeof szFile;
    ofn.lpstrFileTitle		= szFileTitle;
    ofn.nMaxFileTitle		= sizeof szFileTitle;
    ofn.lpstrInitialDir		= lpszDir;
    ofn.lpstrTitle			= "Save Waveform File As";
    ofn.Flags				= OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
    ofn.nFileOffset			= 0;
    ofn.nFileExtension      = 0;
    ofn.lpstrDefExt         = "WAV";
    ofn.lCustData			= 0;
    ofn.lpfnHook			= NULL;
    ofn.lpTemplateName		= NULL;

    i = GetSaveFileName(&ofn);                              // Invoke the DLL routine
    if (i)  // OK button pressed
    {
        // Copy the file name into the caller's buffer
        strncpy(Output, szFile, OutputLength);
        return TRUE;
    }
    else return FALSE;
}

