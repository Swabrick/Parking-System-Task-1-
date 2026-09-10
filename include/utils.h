#ifndef UTILS_H
#define UTILS_H

#include <time.h>

/* Prints a timestamp in a readable format. */
void printFormattedTime(time_t value);

/* Prints a slot code such as 1A05. */
void printSlotCode(int floor, int wing, int slot);

/* Prints duration as hours and minutes. */
void printDuration(long durationSeconds);

#endif
