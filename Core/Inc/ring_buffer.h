/*
 * ring_buffer.h
 *
 *  Created on: Feb 15, 2024
 *      Author: User
 */

#ifndef INC_RING_BUFFER_H_
#define INC_RING_BUFFER_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "usbd_audio.h"
#include "debug_defs.h"



//#define RESAMPLE_BUFFER_SIZE (AUDIO_IN_PACKET*_SAMEPLE_SIZE/2)
//#define RING_BUFFER_SIZE (AUDIO_IN_PACKET*_PACK_SIZE/2)

typedef struct {
    int16_t* buffer;
    int head;
    int tail;
//    int size;
} RingBuffer;

//void RingBuffer_Init(RingBuffer* rb);
void RingBuffer_Init(RingBuffer* rb, int16_t* buffer);
bool RingBuffer_Write(RingBuffer* rb, const int16_t* data, int length);
bool RingBuffer_Read(RingBuffer* rb, int16_t* data, int length);
void RingBuffer_Free(RingBuffer* rb);


#endif /* INC_RING_BUFFER_H_ */
