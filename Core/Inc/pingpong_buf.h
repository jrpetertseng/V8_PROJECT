/*
 * pingpong_buf.h
 *
 *  Created on: 2022年11月24日
 *      Author: User
 */
#include <stdbool.h>
#include <stdint.h> //for uint8_t
#ifndef INC_PINGPONG_BUF_H_
#define INC_PINGPONG_BUF_H_


typedef struct
{
    void *buffer[2];
    volatile uint8_t writeIndex;
    volatile uint8_t readIndex;
    volatile uint8_t readAvaliable[2];
} PingPongBuffer_t;

void PingPongBuffer_Init(PingPongBuffer_t *ppbuf, void *buf0, void *buf1);
bool PingPongBuffer_GetReadBuf(PingPongBuffer_t *ppbuf, void **pReadBuf);
void PingPongBuffer_SetReadDone(PingPongBuffer_t *ppbuf);
void PingPongBuffer_GetWriteBuf(PingPongBuffer_t *ppbuf, void **pWriteBuf);
void PingPongBuffer_SetWriteDone(PingPongBuffer_t *ppbuf);

#endif /* INC_PINGPONG_BUF_H_ */
