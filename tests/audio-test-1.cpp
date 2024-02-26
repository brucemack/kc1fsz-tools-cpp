#include <cmath>
#include <iostream>

#include "kc1fsz-tools/AudioAnalyzer.h"

using namespace std;
using namespace kc1fsz;

int main(int, const char**) {

    const uint32_t historySize = 512;
    int16_t history[historySize];
    AudioAnalyzer analyzer(history, historySize, 8000);

    // Make a tone
    const uint32_t toneSize = 512;
    int16_t tone[toneSize];
    float w = 2.0 * 3.1415926 * 500.0 / 8000.0;
    float phi = 0;
    // NOTICE: We are shifting down by 6 here
    // 32767, 16384, 8192, 4096, 2048, 1024, 512
    for (uint32_t i = 0; i < toneSize; i++) {
        tone[i] = std::cos(phi) * 256.0;
        phi += w;
    }

    analyzer.play(tone, toneSize);

    cout << analyzer.getTonePower(200) << endl;
    cout << analyzer.getTonePower(500) << endl;
    cout << analyzer.getTonePower(510) << endl;
    cout << "Max " << std::pow(512.0 * (toneSize / 2.0), 2) << endl;

}
