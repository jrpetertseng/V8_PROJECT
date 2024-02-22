/*
 * ring_buffer.c
 *
 *  Created on: Feb 15, 2024
 *      Author: User
 */
#include "ring_buffer.h"
#include <stdlib.h>


//void RingBuffer_Init(RingBuffer* rb) {
//    rb->head = 0;
//    rb->tail = 0;
//    memset(rb->buffer, 0, sizeof(rb->buffer));
//}

void RingBuffer_Init(RingBuffer* rb, int16_t* buffer)
{
//    rb->buffer = (int16_t*)malloc(bufferSize * sizeof(int16_t)); // 动态分配缓冲区
//    memset(rb->buffer, 0, bufferSize * sizeof(int16_t));
    rb->buffer = buffer;
    if (rb->buffer == NULL) {
        // 处理动态分配失败的情况
    }
    rb->head = 0;
    rb->tail = 0;
//    rb->size = bufferSize; // 设置缓冲区大小
}

bool RingBuffer_Write(RingBuffer* rb, const int16_t* data, int length)
{
    bool result = true;

    for (int i = 0; i < length; ++i) {
        int nextTail = (rb->tail + 1) % RING_BUFFER_SIZE;
        if (nextTail != rb->head) {
            rb->buffer[rb->tail] = data[i];
            rb->tail = nextTail;
        } else {
            result = false;
            break;
        }
    }
    return result;
}

bool RingBuffer_Read(RingBuffer* rb, int16_t* data, int length)
{
    bool result = true;

    for (int i = 0; i < length; ++i) {
        if (rb->head != rb->tail) {
            data[i] = rb->buffer[rb->head];
            rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
        } else {
            result = false;
            break;
        }
    }
    return result;
}

void RingBuffer_Free(RingBuffer* rb)
{
    free(rb->buffer); // 释放动态分配的缓冲区
    rb->buffer = NULL; // 避免野指针
    rb->head = 0;
    rb->tail = 0;
//    rb->size = 0;
}

