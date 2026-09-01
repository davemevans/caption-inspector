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
//  61   01   10 | 96 69  10   50  43  00 00 | 72   E2 | FF 02 21 FE 03 04
//  ...footer(74 00 00 00) ANC-cs(00)
static const uint8 wellFormedPacket[] = {
    0x61, 0x01, 0x10, 0x96, 0x69, 0x10, 0x50, 0x43, 0x00, 0x00, 0x72, 0xE2,
    0xFF, 0x02, 0x21, 0xFE, 0x03, 0x04,
    0x74, 0x00, 0x00, 0x00, 0x00
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
 -------------------------------------------------------------------------------*/
void utest__decodeMccLineBounds( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    MccDecodeCtx ctx;
    ctx.numCcCountMismatches = 0;

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
