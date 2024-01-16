#ifndef __GESTURE_H
#define __GESTURE_H

enum {
	DIRECTION_HORIZONTAL =0,
	DIRECTION_VERTIVAL
};

enum {
    DATA_TYPE_PEAK_RATE = 0,
    DATA_TYPE_MEDIAN_RANGE
};

enum {
	GESTURE_NONE = 0,
	GESTURE_UP,
	GESTURE_DOWN,
	GESTURE_LEFT,
	GESTURE_RIGHT,
	GESTURE_PULL,
	GESTURE_PUSH,
	GESTURE_HALT,
	GESTURE_UNKNOWN = 0xFF
};

#define MIN_SIGNAL_RATE 250000

struct VoteResult {
    int pos;
    int neg;
};

void initGesture(int dataCount);
void resetGesture();
void putToGestureBuffer(
		int *peakRate,
		int *medianRange
		);
int presence(int *frame, int size);
uint8_t determinePushPull(int *frame, int size);
uint8_t determineGesture();
void calculateEdges(int type);
struct VoteResult *getCurrentVoteResult(int dir);

#endif // __GESTURE_H
