/*********************************************************************
*                                                                    *
* ADARUTL.C                                                          *
*                                                                    *
* All-Day Audio Recorder Utility Functions                           *
*                                                                    *
* Copyright (c) 1995-2005 Dave Jacobs. All Rights Reserved. 		 *
*                                                                    *
*********************************************************************/

#include "AdarPch.h"



Queue::Queue()
{

    q.count = 0;
    q.anchor = NULL;    
    InitializeCriticalSection(&cs);
}


Queue::~Queue()
{
    DeleteCriticalSection(&cs);
    if (q.count != 0)
    {
        MessageBox(0, "Queue being deleted before queue empty", "", MB_OK);
    }
}


void Queue::Enq(QUEUEITEM *qi)
{
    EnterCriticalSection(&cs);

// enchain this block

    if (q.anchor == NULL)
    {   // empty queue
        q.anchor   = qi;                    // anchor it
        qi->prev    = qi;                   // prev = us
        qi->next    = qi;                   // next = us
    }
    else
    {   // non empty chain
        qi->next        = q.anchor;         // chain next to us
        qi->prev        = q.anchor->prev;   // chain prev to us
        qi->next->prev  = qi;               // chain us to next
        qi->prev->next  = qi;               // chain us to prev
    }
    q.count++;                              // increment queue item count
    LeaveCriticalSection(&cs);
}




// general purpose fifo dequeueing routine
QUEUEITEM *Queue::Deq()
{
    QUEUEITEM *qi;                          // gives us access to chaining info

    EnterCriticalSection(&cs);

    if (q.anchor == NULL)
    {   // fatal error
        DebugBreak();
        q.count = 0;                       // in case resumed anyway, this should protect us
        return NULL;
    }

    qi = q.anchor;                         // select oldest item to be dequeued

// cut this item's earthly bonds    

    if (qi->next == qi || qi->prev == qi)
    {   // if we are alone in the chain
        q.anchor = NULL;                   // nullify the anchor (deletes the chain)
        qi->next = NULL;                    // indicate we are not chained
        qi->prev = NULL;
    }
    else
    {   // very carefully, unhook us
        q.anchor       = q.anchor->next;  // this should select the next oldest item
        qi->next->prev  = qi->prev;
        qi->prev->next  = qi->next;
        qi->next        = NULL;
        qi->prev        = NULL;
    }
    q.count--;                             // decrement queue item count
    LeaveCriticalSection(&cs);

    return qi;
}

// Dequeue a specific entry and return the address
QUEUEITEM *Queue::Deq(QUEUEITEM *qi)
{

    if (!IsHere(qi))
    {
        FatalAppExit(0, "Deq(QUEUEITEM *qi): Trying to dequeue an item that is not on the queue.");
    }

    EnterCriticalSection(&cs);

// cut this item's earthly bonds    

    if (qi->next == qi || qi->prev == qi)
    {   // if we are alone in the chain
        q.anchor = NULL;                   // nullify the anchor (deletes the chain)
        qi->next = NULL;                    // indicate we are not chained
        qi->prev = NULL;
    }
    else
    {   // very carefully, unhook us
        q.anchor       = qi->next;          // Make sure the anchor != us
        qi->next->prev  = qi->prev;
        qi->prev->next  = qi->next;
        qi->next        = NULL;
        qi->prev        = NULL;
    }
    q.count--;                             // decrement queue item count
    LeaveCriticalSection(&cs);

    return qi;



}

// Return the count of items in this queue
int Queue::Count()
{
    return q.count;
}


  // Inform caller if item is in this queue
BOOL Queue::IsHere(QUEUEITEM *qi)
{
    QUEUEITEM *temp;
    int i;

    for (i = q.count, temp = q.anchor; i > 0; i--, temp = temp->next)
    {
        if (temp == qi) return TRUE;
    }

    return FALSE;       // Not found

}







// general purpose fifo enqueueing routine
void Enqueue(QUEUEITEM *qi, QUEUE *q)
{
    EnterCriticalSection(&q->cs);

// enchain this block

    if (q->anchor == NULL)
    {   // empty queue
        q->anchor   = qi;                   // anchor it
        qi->prev    = qi;                   // prev = us
        qi->next    = qi;                   // next = us
    }
    else
    {   // non empty chain
        qi->next        = q->anchor;        // chain next to us
        qi->prev        = q->anchor->prev;  // chain prev to us
        qi->next->prev  = qi;               // chain us to next
        qi->prev->next  = qi;               // chain us to prev
    }
    q->count++;                             // increment queue item count
    LeaveCriticalSection(&q->cs);
}




// general purpose fifo dequeueing routine
QUEUEITEM *Dequeue(QUEUE *q)
{
    QUEUEITEM *qi;                          // gives us access to chaining info

    EnterCriticalSection(&q->cs);

    if (q->anchor == NULL)
    {   // fatal error
        DebugBreak();
        q->count = 0;                       // in case resumed anyway, this should protect us
        return NULL;
    }

    qi = q->anchor;                         // select oldest item to be dequeued

// cut this item's earthly bonds    

    if (qi->next == qi || qi->prev == qi)
    {   // if we are alone in the chain
        q->anchor = NULL;                   // nullify the anchor (deletes the chain)
        qi->next = NULL;                    // indicate we are not chained
        qi->prev = NULL;
    }
    else
    {   // very carefully, unhook us
        q->anchor       = q->anchor->next;  // this should select the next oldest item
        qi->next->prev  = qi->prev;
        qi->prev->next  = qi->next;
        qi->next        = NULL;
        qi->prev        = NULL;
    }
    q->count--;                             // decrement queue item count
    LeaveCriticalSection(&q->cs);

    return qi;
}

SHORT Samp8To16(BYTE In)
{
	return (((SHORT) In) - 128) << 8;
}

BYTE Samp16To8(SHORT In)
{
	return (In >> 8) + 128;
}

// Allocate a local buffer and create a sine wave of the proper format in the buffer.
BOOL CreateSinewave(double Freq/* Hz */, double Duration/* sec */ , double Level/* 0-1 */ , LPWAVEFORMATEX pFormat, LPVOID * ppBuffer, LPDWORD pCount)
{
	double	theta;			// Wave cycler (in radians).
	double	val;			// Used to compute sample value (output of sine function).
	double	stepRad;		// Cycler step per sample in rads.
	double  samplesPerWave;
	double  wavesTotal;
	DWORD	samplesTotal;
	DWORD	bytesTotal;
	int		bytesPerSample;
	double	pi = 3.1415926535898;
	PVOID	pBuffer;	// Store samples here
	PSHORT	p16;		// scan pointer 16 bit samples
	PBYTE	p8;			//               8
	DWORD	i,j;		// Loop controls

	samplesPerWave	= (double) pFormat->nSamplesPerSec / Freq;
	wavesTotal		= floor(Duration * Freq);				// By truncating this value, the waves will be multiples of 360%
	samplesTotal	= (DWORD) (wavesTotal * samplesPerWave);
	bytesPerSample	= pFormat->nBlockAlign;
	bytesTotal		= (DWORD) samplesTotal * bytesPerSample;
	stepRad			= (360 / samplesPerWave) * (pi / 180);	// Find # radians of a wave cycle per sample

	pBuffer = (PVOID)	new BYTE[(int) bytesTotal];
	p16		= (PSHORT)	pBuffer;
	p8		= (PBYTE)	pBuffer;

	// Loop to generate the waveform.
	theta = 0;
	for(i = 0, j = (DWORD) samplesTotal; i < j; i++, theta += stepRad, p16++, p8++)
	{
		if (theta >= (2*pi)) theta -= (2*pi);	// prevent wrapping of theta value.
		val = sin(theta) * Level;		// Get a wave sample.
		if (pFormat->wBitsPerSample == 16)
		{
			*p16 = (SHORT) (val * ((double) 32768));
			if (pFormat->nChannels == 2)
			{
				*(++p16) = (SHORT) (val * (double) 32768);
			}
		}
		else
		{	// 8 bits
			*p8 = (BYTE) (val * (double) 128 + 128);
			if (pFormat->nChannels == 2)
			{
				*(++p8) = (BYTE) (val * (double) 128 + 128);
			}
		}
	}
	
	// Return this new buffer and count to the caller. The caller has to deallocate the memory.
	*ppBuffer = pBuffer;
	*pCount = (DWORD) bytesTotal;

	return TRUE;
}
