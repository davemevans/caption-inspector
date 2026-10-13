//
//  utest__mcc_decode.c
//  Caption Inspector
//
//  Unit tests for decodeMccLine()'s bounds handling. The ANC/CDP packet is
//  walked using field widths (data count, cc_count, service count, cdp length)
//  taken straight from the input, so a truncated or malformed line must not be
//  allowed to drive reads past the end of the decoded buffer.
//

#include "test_engine.h"
#include "../src/xform/mcc_decode.c"

/*----------------------------------------------------------------------------*/
/*--                            Test Helpers                                --*/
/*----------------------------------------------------------------------------*/

// Build an input Buffer holding the given raw bytes (decodeMccLine consumes and
// frees whatever it is handed, so each call needs a fresh one).
static Buffer* makeMccBuffer( const uint8* bytes, uint16 count ) {
    Buffer* buffPtr = NewBuffer(BUFFER_TYPE_BYTES, count);
    buffPtr->numElements = count;
    memcpy(buffPtr->dataPtr, bytes, count);
    buffPtr->captionTime.frameRatePerSecTimesOneHundred = 0;   // let decodeFrameRate set it
    return buffPtr;
}

// A minimal, well-formed ANC/CDP packet carrying 2 CC constructs.
//  DID  SDID DC | cdpId  len  fr  flg seq   | ccId cc | <-- 2 constructs -->
//  61   01   13 | 96 69  13   50  43  00 00 | 72   E2 | FF 02 21 FE 03 04
//  ...footer(74 00 00 6C) ANC-cs(75)
static const uint8 wellFormedPacket[] = {
    0x61, 0x01, 0x13, 0x96, 0x69, 0x13, 0x50, 0x43, 0x00, 0x00, 0x72, 0xE2,
    0xFF, 0x02, 0x21, 0xFE, 0x03, 0x04,
    0x74, 0x00, 0x00, 0x6C, 0x75
};

// The same packet with a CDP time code section (01:00:00:00) before the CC data.
//  DID  SDID DC | cdpId  len  fr  flg seq   | tcId <-- time code --> | ccId cc | ...
//  61   01   18 | 96 69  18   50  C3  00 00 | 71   C1  80  80  00    | 72   E2 | ...
//  ...footer(74 00 00 B5) ANC-cs(7A)
static const uint8 timeCodePacket[] = {
    0x61, 0x01, 0x18, 0x96, 0x69, 0x18, 0x50, 0xC3, 0x00, 0x00,
    0x71, 0xC1, 0x80, 0x80, 0x00,
    0x72, 0xE2, 0xFF, 0x02, 0x21, 0xFE, 0x03, 0x04,
    0x74, 0x00, 0x00, 0xB5, 0x7A
};

/*----------------------------------------------------------------------------*/
/*--                             Test Cases                                 --*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decodeMccLine()
 |
 | TEST CASES:
 |     1) A well-formed packet decodes to the expected CC construct bytes.
 |     2) A packet whose cc_count exceeds the bytes present is rejected (this is
 |        the heap over-read: cc_count*3 bytes were memcpy'd from the buffer).
 |     3) A line too short to even hold the ANC packet header is rejected.
 |     4) A line too short to hold the CDP header is rejected.
 |     5) A packet with a time code section decodes the CC constructs after it
 |        (the section is 5 bytes including its ID).
 -------------------------------------------------------------------------------*/
void utest__decodeMccLineBounds( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    MccDecodeCtx ctx;
    ctx.numCcCountMismatches = 0;
    ctx.numFrameRateMismatches = 0;

    TEST_START("Test Case: decodeMccLine() - Well-formed packet decodes its CC constructs.");
    ctx.numCcCountMismatches = 0;
    Buffer* good = decodeMccLine(&ctx, makeMccBuffer(wellFormedPacket, sizeof(wellFormedPacket)));
    ASSERT_NEQ((uint64)(uintptr_t)NULL, (uint64)(uintptr_t)good);
    if( good != NULL ) {
        ASSERT_EQ(2 * sizeof(cc_construct), good->numElements);
        FreeBuffer(good);
    }
    TEST_END

    TEST_START("Test Case: decodeMccLine() - cc_count larger than the buffer is rejected.");
    // Same header, but cc_count is set to 10 (0xEA) while only 2 constructs follow.
    // Unguarded, this memcpy'd 30 bytes out of an 18-byte buffer.
    uint8 truncated[18];
    memcpy(truncated, wellFormedPacket, 18);
    truncated[11] = 0xEA;   // 0xE0 | cc_count(10)
    ctx.numCcCountMismatches = 0;
    ERROR_EXPECTED
    Buffer* bad = decodeMccLine(&ctx, makeMccBuffer(truncated, sizeof(truncated)));
    ASSERT_PTREQ(NULL, bad);
    TEST_END

    TEST_START("Test Case: decodeMccLine() - Line too short for the ANC header is rejected.");
    uint8 tiny[2] = { 0x61, 0x01 };
    ctx.numCcCountMismatches = 0;
    ERROR_EXPECTED
    Buffer* tooTiny = decodeMccLine(&ctx, makeMccBuffer(tiny, sizeof(tiny)));
    ASSERT_PTREQ(NULL, tooTiny);
    TEST_END

    TEST_START("Test Case: decodeMccLine() - Line too short for the CDP header is rejected.");
    uint8 shortHdr[6] = { 0x61, 0x01, 0x10, 0x96, 0x69, 0x10 };
    ctx.numCcCountMismatches = 0;
    ERROR_EXPECTED
    Buffer* tooShort = decodeMccLine(&ctx, makeMccBuffer(shortHdr, sizeof(shortHdr)));
    ASSERT_PTREQ(NULL, tooShort);
    TEST_END

    TEST_START("Test Case: decodeMccLine() - The time code section is skipped in full.");
    ctx.numCcCountMismatches = 0;
    Buffer* withTc = decodeMccLine(&ctx, makeMccBuffer(timeCodePacket, sizeof(timeCodePacket)));
    ASSERT_NEQ((uint64)(uintptr_t)NULL, (uint64)(uintptr_t)withTc);
    if( withTc != NULL ) {
        ASSERT_EQ(2 * sizeof(cc_construct), withTc->numElements);
        ASSERT_EQ(0xFF, withTc->dataPtr[0]);
        ASSERT_EQ(0x02, withTc->dataPtr[1]);
        ASSERT_EQ(0x04, withTc->dataPtr[5]);
        FreeBuffer(withTc);
    }
    TEST_END
}  // utest__decodeMccLineBounds()

/*------------------------------------------------------------------------------
 | UNTESTED FUNCTIONS:
 |    expandMccLine(), decodeFrameRate(), etc. are exercised by the
 |    integration/Python pipeline tests; this suite covers the bounds handling.
 -------------------------------------------------------------------------------*/
int main( int argc, char* argv[] ) {
    INIT_TEST_FRAMEWORK( argc, argv )

    TEST_SUITE_START("Test Suite: mcc_decode.c -- decodeMccLine() bounds");
    utest__decodeMccLineBounds( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    SHUTDOWN_TEST_FRAMEWORK
}  // main()
