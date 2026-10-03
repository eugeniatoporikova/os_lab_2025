#include "revert_string.h"
#include <string.h>


void RevertString(char *str)
{
	int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

