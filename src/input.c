#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "input.h"

void readLine(char *buffer, size_t size)
{
    size_t length;

    if (fgets(buffer, (int)size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }

    length = strcspn(buffer, "\n");
    buffer[length] = '\0';

    /* If input was longer than the buffer, discard the remainder. */
    if (length == size - 1) {
        int character;
        while ((character = getchar()) != '\n' && character != EOF) {
            /* discard */
        }
    }
}

int readIntInRange(const char *prompt, int minimum, int maximum)
{
    char buffer[100];
    int value;
    char extra;

    while (1) {
        printf("%s", prompt);
        readLine(buffer, sizeof(buffer));

        if (sscanf(buffer, "%d %c", &value, &extra) == 1 &&
            value >= minimum && value <= maximum) {
            return value;
        }

        printf(
            "Invalid input. Enter a number between %d and %d.\n",
            minimum,
            maximum
        );
    }
}

void normalizeVehicleNumber(char *text)
{
    char result[100];
    size_t i, start, end, j = 0;

    /* Remove leading spaces. */
    start = 0;
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        start++;
    }

    /* Find end after removing trailing spaces. */
    end = strlen(text);
    while (end > start &&
           isspace((unsigned char)text[end - 1])) {
        end--;
    }

    /* Convert to uppercase and remove internal spaces. */
    for (i = start; i < end && j < sizeof(result) - 1; i++) {
        if (!isspace((unsigned char)text[i])) {
            result[j++] = (char)toupper((unsigned char)text[i]);
        }
    }

    result[j] = '\0';
    strcpy(text, result);
}
