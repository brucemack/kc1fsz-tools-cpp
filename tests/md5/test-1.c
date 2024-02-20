#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "kc1fsz-tools/md5/md5.h"

int main(int argc, const char** argv) {

    UINT2 t2;
    assert(sizeof(t2) == 2);
    UINT4 t4;
    assert(sizeof(t4) == 4);

    MD5_CTX context;
    unsigned char buffer[1024], digest[16];
    buffer[0] = 'a';
    int len = 1;

    MD5Init (&context);
    MD5Update (&context, buffer, len);
    MD5Final (digest, &context);

    char computed[33];
    MD5DigestToText(digest, computed);
    char* expected = "0cc175b9c0f1b6a831c399e269772661";
    assert(strcmp(computed, expected) == 0);

    return 0;
}
