/* PC test for the CAN decoder (src/can_rx.c), no STM32 needed.
 *
 * Feeds the bus bit stream from python/make_can_vectors.py through the
 * decoder, the way the bit interrupt does (a dominant bit while idle is the
 * start of frame edge), and compares every frame with the expected list.
 *
 *   python python/make_can_vectors.py --frames 5000 --out <dir>
 *   cl /O2 /I src test\host_test.c src\can_rx.c      (or gcc)
 *   host_test <dir>
 *
 * Exit code 0 when every good frame is decoded unchanged and every broken one
 * is counted as the right error.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can_rx.h"

int main(int argc, char **argv)
{
    char path[1024];
    FILE *fb, *fe;
    const char *dir = argc > 1 ? argv[1] : "hdl/vectors";
    unsigned long bits = 0, want_good = 0, want_crc = 0, want_stuff = 0, got = 0, bad = 0;
    char line[128];
    int c;

    snprintf(path, sizeof path, "%s/bus_bits.mem", dir);
    fb = fopen(path, "r");
    snprintf(path, sizeof path, "%s/expected.mem", dir);
    fe = fopen(path, "r");
    if (!fb || !fe) {
        fprintf(stderr, "cannot open bus_bits.mem / expected.mem in %s\n", dir);
        return 2;
    }

    can_rx_reset();
    while ((c = fgetc(fb)) != EOF) {
        can_frame_t f;
        uint8_t level;
        if (c != '0' && c != '1')
            continue;
        level = (uint8_t)(c - '0');
        bits++;
        if (can_rx_idle()) {
            if (level)
                continue;              /* idle bus, no sampling */
            can_rx_sof();              /* falling edge: start of frame */
        }
        if (!can_rx_bit(level, &f))
            continue;

        /* Next good frame in the expected list (skip the broken ones). */
        for (;;) {
            unsigned flags, id, dlc;
            char hex[17];
            uint8_t data[8];
            int k;
            if (!fgets(line, sizeof line, fe)) {
                printf("extra frame id %03X decoded after the expected list ended\n", f.id);
                bad++;
                break;
            }
            if (sscanf(line, "%2x%3x%1x%16s", &flags, &id, &dlc, hex) != 4)
                continue;
            if (flags == 1) { want_crc++; continue; }
            if (flags == 2) { want_stuff++; continue; }
            want_good++;
            for (k = 0; k < 8; k++) {
                unsigned b;
                sscanf(hex + 2 * k, "%2x", &b);
                data[k] = (uint8_t)b;
            }
            got++;
            if (f.id != id || f.dlc != dlc || f.ide || f.rtr || memcmp(f.data, data, 8) != 0) {
                if (bad < 10)
                    printf("frame %lu differs: got %03X/%u, want %03X/%u\n", got, f.id, f.dlc, id, dlc);
                bad++;
            }
            break;
        }
    }
    while (fgets(line, sizeof line, fe)) {     /* expected frames never decoded */
        unsigned flags, id, dlc;
        char hex[17];
        if (sscanf(line, "%2x%3x%1x%16s", &flags, &id, &dlc, hex) != 4)
            continue;
        if (flags == 1) want_crc++;
        else if (flags == 2) want_stuff++;
        else { want_good++; bad++; printf("expected frame id %03X never decoded\n", id); }
    }

    printf("%lu bus bits, %lu good frames expected, %lu decoded, %lu wrong or missing\n",
           bits, want_good, got, bad);
    printf("errors: crc %lu (expected %lu), stuff %lu (expected %lu), form %lu\n",
           (unsigned long)can_rx_stats.crc, want_crc, (unsigned long)can_rx_stats.stuff,
           want_stuff, (unsigned long)can_rx_stats.form);
    if (bad || can_rx_stats.crc != want_crc || can_rx_stats.stuff != want_stuff || can_rx_stats.form) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
