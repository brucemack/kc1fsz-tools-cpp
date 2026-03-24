#pragma once

namespace kc1fsz {

/** 
 * Splits a comma-separated string into tokens.
 */
int tokenizeNMEASentence(const char* str, char parsedTokens[][16], unsigned maxTokens, 
    unsigned maxTokenLen);

/**
 * Parses the date/time of a tokenized GPRMC sentence into a UNIX tm structure.
 *
 * Example NMEA GPS String
 * $GPRMC,194013,A,4032.94888,N,10511.83890,W,0.005,,020121,,,D*62
 *        hhmmss                                     ddmmyy
 */
int parseTimeFromGPRMC(const char parsedTokens[][16], unsigned tokenCount, struct tm* t);

}

