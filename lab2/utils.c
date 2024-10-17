#include <stdlib.h>

#include "utils.h"

int dec_atoi(char *s) {
    return atoi(s);
}

int oct_atoi(char *s) {
    return strtol(s, NULL, 8);
}

int hex_atoi(char *s) {
    return strtol(s, NULL, 16);
}

float fl_atof(char *s) {
    return atof(s);
}
