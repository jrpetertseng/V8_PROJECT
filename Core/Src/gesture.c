#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "gesture.h"

#define TOF_8x8_SIZE 64
#define DATA_BUFFER_SIZE 2
#define KERNEL_WIDTH 3
#define KERNEL_HEIGHT 3
#define KERNEL_SIZE KERNEL_WIDTH*KERNEL_HEIGHT
#define MIN_PRESENCE_DISTANCE 200
#define MAX_PRESENCE_DISTANCE 2000
#define MIN_POINT_FOR_PRESENT 3
#define PULL_THRESHOLD 500
#define PUSH_THRESHOLD 500
#define HALT_CNT_THRESHOLD 23 // (23 frames / 15 fps) = 1.5 sec

int horKernel[] = {
    -1, -1, 1,
    -1, -1, 1,
    -1, -1, 1
};

int verKernel[] = {
    -1, -1, -1,
    -1, -1, -1,
    1, 1, 1
};

static struct {
    int prBuffer0[TOF_8x8_SIZE];
    int prBuffer1[TOF_8x8_SIZE];
    int mrBuffer0[TOF_8x8_SIZE];
    int mrBuffer1[TOF_8x8_SIZE];
    struct VoteResult horResult;
    struct VoteResult verResult;
    int tofSize;
    int width;
    int height;
    int count;
    int pos;
    int firstFrameDistanceAvg;
    int cnt;
} tofData;

//Temp Buffer
static int normalizeBuf[TOF_8x8_SIZE];
static int convBuf[TOF_8x8_SIZE];

static int inRange(int value) {
    return (value >= MIN_PRESENCE_DISTANCE) &&
           (value <= MAX_PRESENCE_DISTANCE);
}



/*static void rangeFilterWithSignalRate(int *pr, int *mr, int size, int threshold) {
    for (int i=0; i<size; i++) {
        if (pr[i] < threshold) {
            mr[i] = -1;
        }
    }
}*/

static void getLastAndCurrentBuffer(int **curr, int **last, int type) {
    switch (type)
    {
    case DATA_TYPE_PEAK_RATE:
        if (!tofData.pos) {
            *curr = tofData.prBuffer1;
            *last = tofData.prBuffer0;
        }
        else {
            *curr = tofData.prBuffer0;
            *last = tofData.prBuffer1;
        }
        break;
    case DATA_TYPE_MEDIAN_RANGE:
        if (!tofData.pos) {
            *curr = tofData.mrBuffer1;
            *last = tofData.mrBuffer0;
        }
        else {
            *curr = tofData.mrBuffer0;
            *last = tofData.mrBuffer1;
        }
        break;
    default:
        *curr = NULL;
        *last = NULL;
        return;
    }
}

static void diffAndNormalize(int *result, int size, int type) {
    int *curr = NULL;
    int *last = NULL;
    int diff = 0;
    int threshold = 100000;

    if (result == NULL) return;

    getLastAndCurrentBuffer(&curr, &last, type);

    if ((curr == NULL) || (last == NULL)) return;

    if (type == DATA_TYPE_MEDIAN_RANGE) threshold = 200;

    for (int i=0;i<size;i++) {
        diff = curr[i] - last[i];
            if (diff < 0) {
                if (-diff > threshold) {
                    diff = -1;
                }
                else {
                    diff = 0;
                }
            }
            else if (diff > 0) {
                if (diff > threshold) {
                    diff = 1;
                }
                else {
                    diff = 0;
                }
            }
            result[i] = diff;
    }
}

static void convolution(
    int *src, int *result, int width, int height,
    int *kernel, int kernelWidth, int kernelHeight) {
    int kernelMid = kernelWidth >> 1;
    int sum, posI, posJ;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            sum = 0;
            for (int ki = 0; ki < kernelHeight; ki++) {
                for (int kj = 0; kj < kernelWidth; kj++) {
                    posI = i+ki-kernelMid;
                    posJ = j+kj-kernelMid;
                    if ((posI>=0) && (posI<height) &&
                        (posJ>=0) && (posJ<width)) {
                        sum += src[posI*width+posJ] * kernel[ki*kernelWidth+kj];
                    }
                }
            }
            result[i*width+j] = sum;
        }
    }
}

static void findHorEdgesAndVote(
    int *src, int *diff,
    struct VoteResult *result,
    int width, int height) {
    int max, min, pIndex, nIndex, val;
    int singleSign, sign, pVote, nVote;
    for (int i = 0; i < height; i++) {
        // find edges
        for (int j = 0; j < width; j++) {
            val = src[i*width+j];
            if (j == 0) {
                pIndex = nIndex = 0;
                max = val;
                min = val;
            }
            else {
                if (val > max) {
                    max = val;
                    pIndex = j;
                }

                if (val < min) {
                    min = val;
                    nIndex = j;
                }
            }
        }

        // vote
        singleSign = 1;
        sign = 0;
        pVote = nVote = 0;
        for (int j = 0; j < width; j++) {
            val = diff[i*width+j];
            if (j <= pIndex) {
                pVote -= val;
            }
            else {
                pVote += val;
            }

            if (j <= nIndex) {
                nVote -= val;
            }
            else {
                nVote += val;
            }

            if (sign == 0) {
                sign = val;
            }
            else if (((sign > 0) && (val < 0)) ||
                     ((sign < 0) && (val > 0))) {
                singleSign = 0;
            }
        }

        if (singleSign) {
            continue;
        }

        result->pos += pVote;
        result->neg += nVote;
    }
}

static void findVerEdgesAndVote(
    int *src, int *diff,
    struct VoteResult *result,
    int width, int height) {
    int max, min, pIndex, nIndex, val;
    int singleSign, sign, pVote, nVote;
    for (int j = 0; j < width; j++) {
        for (int i = 0; i < height; i++) {
            val = src[i*width+j];
            if (i == 0) {
                pIndex = nIndex = 0;
                max = val;
                min = val;
            }
            else {
                if (val > max) {
                    max = val;
                    pIndex = i;
                }

                if (val < min) {
                    min = val;
                    nIndex = i;
                }
            }
        }

        singleSign = 1;
        sign = 0;
        pVote = nVote = 0;
        for (int i = 0; i < height; i++) {
            val = diff[i*width+j];
            if (i <= pIndex) {
                pVote -= val;
            }
            else {
                pVote += val;
            }

            if (i <= nIndex) {
                nVote -= val;
            }
            else {
                nVote += val;
            }

            if (sign == 0) {
                sign = val;
            }
            else if (((sign > 0) && (val < 0)) ||
                     ((sign < 0) && (val > 0))) {
                singleSign = 0;
            }
        }

        if (singleSign) {
            continue;
        }

        result->pos += pVote;
        result->neg += nVote;
    }
}

static int distanceAvg(int *frame, int size) {
	int cnt = 0, sum = 0;
	int value;

	for (int i=0;i<size;i++) {
		value = frame[i];
		if (inRange(value)) {
			sum += value;
			cnt++;
		}
	}

	return cnt ? (sum/cnt) : -1;
}

void initGesture(int dataCount) {
	if (dataCount == TOF_8x8_SIZE) {
		tofData.width = 8;
		tofData.height = 8;
		tofData.tofSize = 64;
	}
	else {
		tofData.width = 4;
		tofData.height = 4;
		tofData.tofSize = 16;
	}

	resetGesture();
}

void resetGesture() {
	tofData.pos = 0;
	tofData.count = 0;
	tofData.firstFrameDistanceAvg = 0;

	tofData.horResult.pos = 0;
	tofData.horResult.neg = 0;
	tofData.verResult.pos = 0;
	tofData.verResult.neg = 0;

	tofData.cnt = 0;
}

void putToGestureBuffer(
		int *peakRate,
		int *medianRange
		) {
    int *prDst = tofData.prBuffer0;
    int *mrDst = tofData.mrBuffer0;

    if (tofData.tofSize <=0) return;

    if (tofData.pos == 1) {
            prDst = tofData.prBuffer1;
            mrDst = tofData.mrBuffer1;
    }

    tofData.cnt++;

    if (peakRate != NULL) {
    	memcpy(prDst, peakRate, tofData.tofSize*sizeof(int));
    }

    if (medianRange != NULL) {
    	memcpy(mrDst, medianRange, tofData.tofSize*sizeof(int));
    }

    if (++tofData.pos > DATA_BUFFER_SIZE - 1) {
        tofData.pos = 0;
    }

    if (++tofData.count > DATA_BUFFER_SIZE) {
        tofData.count = DATA_BUFFER_SIZE;
    }
}

int presence(int *frame, int size) {
    int count = 0;
    for (int i=0; i<size; i++) {
        count += inRange(frame[i]);
    }

    return count >= MIN_POINT_FOR_PRESENT;
}

uint8_t determinePushPull(int *frame, int size) {
	if (tofData.firstFrameDistanceAvg == 0) {
		tofData.firstFrameDistanceAvg = distanceAvg(frame, size);
	} else if (tofData.cnt > HALT_CNT_THRESHOLD) {
		return GESTURE_HALT;
    } else {
		int avg = distanceAvg(frame, size);
		if ((tofData.firstFrameDistanceAvg - avg) > PULL_THRESHOLD) {
			return GESTURE_PULL;
		}
		else if ((avg - tofData.firstFrameDistanceAvg) > PUSH_THRESHOLD) {
			return GESTURE_PUSH;
		}
	}

	return GESTURE_NONE;
}

uint8_t determineGesture() {
	if (tofData.count < 2) return GESTURE_NONE;

	struct VoteResult *horResult = &tofData.horResult;
	struct VoteResult *verResult = &tofData.verResult;
    int hor = abs(horResult->pos) > abs(horResult->neg) ? horResult->pos : horResult->neg;
    int ver = abs(verResult->pos) > abs(verResult->neg) ? verResult->pos : verResult->neg;
    char result = GESTURE_NONE;

    if (abs(hor) > abs(ver)) {
        if (hor > 0) {
            result = GESTURE_RIGHT;
        }
        else if (hor < 0) {
            result = GESTURE_LEFT;
        }
    }
    else if (abs(hor) < abs(ver)) {
        if (ver > 0) {
            result = GESTURE_DOWN;
        }
        else if (ver < 0) {
            result = GESTURE_UP;
        }
    }

    return result;
}

void calculateEdges(int type) {
    if (tofData.count < 2) return;

    diffAndNormalize(normalizeBuf, tofData.tofSize, type);

    // Horizontal
    convolution(normalizeBuf, convBuf, tofData.width, tofData.height,
                horKernel, KERNEL_WIDTH, KERNEL_HEIGHT);

    findHorEdgesAndVote(
        convBuf, normalizeBuf,
        &tofData.horResult,
        tofData.width, tofData.height);

    // Vertical
    convolution(normalizeBuf, convBuf, tofData.width, tofData.height,
                verKernel, KERNEL_WIDTH, KERNEL_HEIGHT);

    findVerEdgesAndVote(
        convBuf, normalizeBuf,
        &tofData.verResult,
		tofData.width, tofData.height);
}

struct VoteResult *getCurrentVoteResult(int dir) {
	if (dir == DIRECTION_HORIZONTAL) {
		return &tofData.horResult;
	}

	return &tofData.verResult;
}

