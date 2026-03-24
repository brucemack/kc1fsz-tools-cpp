#include <time.h>
#include <cstdlib>
#include <cstring>

#include "kc1fsz-tools/GPSUtils.h"

namespace kc1fsz {

int tokenizeNMEASentence(const char* str, char parsedTokens[][16], unsigned maxTokens, 
    unsigned maxTokenLen) {
    unsigned tokens = 0;
    unsigned tokenPtr = 0;
    for (unsigned i = 0; i < strlen(str) && tokens < maxTokens; i++) {
        if (str[i] == ',') {
            tokens++;
            tokenPtr = 0;
        } else if (tokenPtr < maxTokenLen - 1) {
            parsedTokens[tokens][tokenPtr] = str[i];
            parsedTokens[tokens][tokenPtr + 1] = 0;
            tokenPtr++;
        }
    }
    return tokens;
}

int parseTimeFromGPRMC(const char tokens[][16], unsigned tokenCount, struct tm* t) {

    memset(t, 0, sizeof(struct tm));
    char temp[3];
    
    if (strlen(tokens[1]) < 6)
        return -1;
    // Seconds
    temp[0] = tokens[1][4];
    temp[1] = tokens[1][5];
    temp[2] = 0;
    t->tm_sec = atoi(temp);
    // Minutes
    temp[0] = tokens[1][2];
    temp[1] = tokens[1][3];
    temp[2] = 0;
    t->tm_min = atoi(temp);
    // Hours
    temp[0] = tokens[1][0];
    temp[1] = tokens[1][1];
    temp[2] = 0;
    t->tm_hour = atoi(temp);

    if (strlen(tokens[9]) < 6)
        return -1;
    // Day
    temp[0] = tokens[9][0];
    temp[1] = tokens[9][1];
    temp[2] = 0;
    t->tm_mday = atoi(temp);
    // Month
    temp[0] = tokens[9][2];
    temp[1] = tokens[9][3];
    temp[2] = 0;
    // Month is zero-based
    t->tm_mon = atoi(temp) - 1;
    // MYear
    temp[0] = tokens[9][4];
    temp[1] = tokens[9][5];
    temp[2] = 0;
    // Year is 1900-based
    t->tm_year = (atoi(temp) + 2000) - 1900;

    return 0;
}

}
