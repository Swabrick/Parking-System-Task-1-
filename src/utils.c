#include <stdio.h>
#include <time.h>
#include "utils.h"

void printFormattedTime(time_t value)
{
    struct tm *timeInfo;
    char buffer[64];

    timeInfo = localtime(&value);

    if (timeInfo == NULL) {
        printf("Unavailable");
        return;
    }

    strftime(
        buffer,
        sizeof(buffer),
        "%d %b %Y, %H:%M:%S",
        timeInfo
    );

    printf("%s", buffer);
}

void printSlotCode(int floor, int wing, int slot)
{
    printf(
        "%d%c%02d",
        floor + 1,
        wing == 0 ? 'A' : 'B',
        slot + 1
    );
}

void printDuration(long durationSeconds)
{
    long totalMinutes;
    long hours;
    long minutes;

    if (durationSeconds < 0) {
        durationSeconds = 0;
    }

    totalMinutes = durationSeconds / 60;
    hours = totalMinutes / 60;
    minutes = totalMinutes % 60;

    printf("%ld hour(s) %ld minute(s)", hours, minutes);
}
