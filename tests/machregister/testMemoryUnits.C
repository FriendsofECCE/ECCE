//  Small driver for MemoryUnits.H's GB<->MB conversion (the helper behind
//  Machine Registration's and the Launcher's "Max Memory" fields, both
//  displayed in GB while the .Q file / launch data stay MB). Header-only,
//  no wx/ecce libraries needed. Run via tests/machregister/run_tests.py.
#include <cstdio>
#include "../../src/apps/machregister/MemoryUnits.H"

static int failures = 0;

static void checkMB(unsigned int mb, unsigned int expectGB)
{
    unsigned int got = MemoryUnits::mbToGB(mb);
    printf("mbToGB(%u) = %u\n", mb, got);
    if (got != expectGB)
    {
        printf("FAIL  mbToGB(%u) expected %u, got %u\n", mb, expectGB, got);
        failures++;
    }
}

static void checkGB(unsigned int gb, unsigned int expectMB)
{
    unsigned int got = MemoryUnits::gbToMB(gb);
    printf("gbToMB(%u) = %u\n", gb, got);
    if (got != expectMB)
    {
        printf("FAIL  gbToMB(%u) expected %u, got %u\n", gb, expectMB, got);
        failures++;
    }
}

int main()
{
    // 0 means "no limit" and must stay 0 in both directions.
    checkMB(0, 0);
    checkGB(0, 0);

    // Exact multiples of 1000 MB round-trip exactly.
    checkMB(1000, 1);
    checkMB(8000, 8);
    checkGB(8, 8000);

    // A nonzero limit smaller than 1 GB must not become "no limit".
    checkMB(400, 1);

    // Normal rounding to nearest GB.
    checkMB(1499, 1);
    checkMB(1500, 2);
    checkMB(7400, 7);
    checkMB(7600, 8);

    if (failures == 0)
        printf("PASS  all MemoryUnits conversions correct\n");
    else
        printf("FAIL  %d MemoryUnits conversion(s) wrong\n", failures);

    return (failures == 0) ? 0 : 1;
}
