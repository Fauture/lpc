#include <cstdio>
#include <cstring>
extern "C" {
#include "d3des.h"
}
int main() {
    unsigned char d[32] = {
        0x36,0x41,0x34,0x30,0x45,0x31,0x41,0x31,0x31,0x30,0x39,0x45,0x35,0x44,0x39,0x33,
        0x36,0x33,0x36,0x42,0x46,0x30,0x34,0x36,0x45,0x37,0x33,0x38,0x36,0x31,0x44,0x45
    };
    // makekey hash "uWvgwf" -> 8-byte key
    char kb[8] = {0}; strncpy(kb, "uWvgwf", 7);
    unsigned char mk[8] = {0};
    makekey(kb, mk);
    printf("makekey: ");
    for (int i = 0; i < 8; i++) printf("%02X", mk[i]);
    printf("\n");
    deskey(mk, 0);
    unsigned char out[32] = {0};
    for (int i = 0; i < 4; i++) des(d + i*8, out + i*8);
    printf("hashed enc: ");
    for (int i = 0; i < 32; i++) printf("%02X", out[i]);
    printf("\n");
    return 0;
}
