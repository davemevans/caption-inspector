//
//  utest__types.c
//  Caption Inspector
//
//  Unit tests for the fixed-width integer typedefs in types.h.
//
//  These guard against a platform (e.g. any LP64 target: macOS, 64-bit Linux)
//  silently making the "32-bit" types 64-bit wide, or making int8 unsigned.
//  Both mistakes existed historically: int8 was 'char' (unsigned on ARM) and
//  uint32/int32 were 'long' (8 bytes on LP64).
//

#include "test_engine.h"
#include "../include/types.h"

/*----------------------------------------------------------------------------*/
/*--                             Test Cases                                 --*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: types.h -- Integer Widths
 |
 | TEST CASES:
 |     1) Each type has exactly the width its name advertises.
 -------------------------------------------------------------------------------*/
void utest__typeWidths( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE

    TEST_START("Test Case: types.h - Integer type widths match their names.");
    ASSERT_EQ(1, sizeof(uint8));
    ASSERT_EQ(1, sizeof(int8));
    ASSERT_EQ(2, sizeof(uint16));
    ASSERT_EQ(2, sizeof(int16));
    ASSERT_EQ(4, sizeof(uint32));
    ASSERT_EQ(4, sizeof(int32));
    ASSERT_EQ(8, sizeof(uint64));
    ASSERT_EQ(8, sizeof(int64));
    ASSERT_EQ(1, sizeof(boolean));
    TEST_END
}  // utest__typeWidths()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: types.h -- Signedness
 |
 | TEST CASES:
 |     1) Signed types are signed; unsigned types are unsigned.
 -------------------------------------------------------------------------------*/
void utest__typeSignedness( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE

    TEST_START("Test Case: types.h - Signed types hold negative values.");
    int8 s8 = -1;
    int16 s16 = -1;
    int32 s32 = -1;
    int64 s64 = -1;
    ASSERT_EQ(TRUE, (s8 < 0));
    ASSERT_EQ(TRUE, (s16 < 0));
    ASSERT_EQ(TRUE, (s32 < 0));
    ASSERT_EQ(TRUE, (s64 < 0));
    TEST_END

    TEST_START("Test Case: types.h - Unsigned types wrap instead of going negative.");
    // (uint8)-1 == 255, (uint16)-1 == 65535, etc. A signed 'char' int8 would
    // have made these behave differently in the callers that treat bytes as 0..255.
    uint8 u8 = (uint8)(-1);
    uint16 u16 = (uint16)(-1);
    ASSERT_EQ(255, u8);
    ASSERT_EQ(65535, u16);
    TEST_END
}  // utest__typeSignedness()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: types.h -- Modular Arithmetic
 |
 | TEST CASES:
 |     1) uint32 arithmetic wraps at 2^32, not 2^64.
 -------------------------------------------------------------------------------*/
void utest__typeWraparound( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE

    TEST_START("Test Case: types.h - uint32 wraps at 2^32.");
    // If uint32 were 64-bit (the historical LP64 bug), 0xFFFFFFFF + 1 would be
    // 0x100000000 rather than 0, and this assertion would fail.
    uint32 u32max = 0xFFFFFFFFu;
    ASSERT_EQ(0, (uint32)(u32max + 1));
    ASSERT_EQ(0xFFFFFFFFu, u32max);
    TEST_END

    TEST_START("Test Case: types.h - uint16 wraps at 2^16.");
    uint16 u16max = 0xFFFF;
    ASSERT_EQ(0, (uint16)(u16max + 1));
    TEST_END
}  // utest__typeWraparound()

/*------------------------------------------------------------------------------
 | UNTESTED FUNCTIONS:
 |    (types.h declares no functions.)
 -------------------------------------------------------------------------------*/
int main( int argc, char* argv[] ) {
    INIT_TEST_FRAMEWORK( argc, argv )

    TEST_SUITE_START("Test Suite: types.h -- Integer Widths");
    utest__typeWidths( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: types.h -- Signedness");
    utest__typeSignedness( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: types.h -- Modular Arithmetic");
    utest__typeWraparound( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    SHUTDOWN_TEST_FRAMEWORK
}  // main()
