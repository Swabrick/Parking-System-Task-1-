#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>

/* Reads a whole line safely and removes the trailing newline. */
void readLine(char *buffer, size_t size);

/* Reads an integer within a specified range. */
int readIntInRange(const char *prompt, int minimum, int maximum);

/* Removes leading/trailing whitespace and converts text to uppercase. */
void normalizeVehicleNumber(char *text);

#endif
