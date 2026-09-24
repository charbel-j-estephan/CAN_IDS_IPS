/* Run the generated C model on your PC against the same test vectors the
 * Verilog testbench uses. A PASS here plus a PASS in simulation proves the
 * FPGA and the STM32 compute the identical classifier.
 *
 * From the repo root:
 *   gcc -O2 -I mcu/generated -o build/host_test mcu/host_test.c
 *   ./build/host_test hdl/generated/vectors.hex
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "can_ids_model.h"

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "hdl/generated/vectors.hex";
    FILE *f = fopen(path, "r");
    if (!f) {
        perror(path);
        return 2;
    }

    char line[64];
    long n = 0, errors = 0;
    while (fgets(line, sizeof line, f)) {
        if (strlen(line) < 22)
            continue;
        /* Line layout: can_id(3 hex) dlc(1) data(16) expected(2). */
        char tmp[4] = {0};
        float feat[10];
        memcpy(tmp, line, 3);
        feat[0] = (float)strtol(tmp, NULL, 16);
        memset(tmp, 0, sizeof tmp);
        memcpy(tmp, line + 3, 1);
        feat[1] = (float)strtol(tmp, NULL, 16);
        for (int i = 0; i < 8; i++) {
            memset(tmp, 0, sizeof tmp);
            memcpy(tmp, line + 4 + 2 * i, 2);
            feat[2 + i] = (float)strtol(tmp, NULL, 16);
        }
        memset(tmp, 0, sizeof tmp);
        memcpy(tmp, line + 20, 2);
        int32_t expected = (int32_t)strtol(tmp, NULL, 16);

        int32_t got = can_ids_predict(feat, 10);
        if (got != expected) {
            if (errors < 10)
                printf("mismatch frame %ld: got %d expected %d\n", n, got, expected);
            errors++;
        }
        n++;
    }
    fclose(f);

    if (errors == 0 && n > 0)
        printf("PASS: %ld frames, C model matches Python\n", n);
    else
        printf("FAIL: %ld mismatches out of %ld frames\n", errors, n);
    return errors == 0 && n > 0 ? 0 : 1;
}
