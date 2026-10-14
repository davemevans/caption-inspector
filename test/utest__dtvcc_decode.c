//
//  utest__dtvcc_decode.c
//  Caption Inspector
//
//  Unit tests for DTVCC packet assembly and Service Block parsing in
//  dtvcc_decode.c.
//
//  cc_data triples are fed through DtvccDecodeProcNextBuffer() and the decoded
//  elements are collected by a test sink, so each test sees exactly what the
//  708 output would be given.
//

#include "test_engine.h"
#include "../src/xform/dtvcc_decode.c"

/*----------------------------------------------------------------------------*/
/*--                            Test Helpers                                --*/
/*----------------------------------------------------------------------------*/

#define PS 0xFF   // valid DTVCC Packet Start
#define PD 0xFE   // valid DTVCC Packet Data

static DtvccData captured[256];
static int numCaptured;

static uint8 captureNextBuffer( void* rootCtxPtr, Buffer* buffPtr ) {
    DtvccData* dataPtr = (DtvccData*)buffPtr->dataPtr;
    for( int loop = 0; (loop < buffPtr->numElements) && (numCaptured < 256); loop++ ) {
        captured[numCaptured++] = dataPtr[loop];
    }
    FreeBuffer(buffPtr);
    return PIPELINE_SUCCESS;
}

static uint8 captureShutdown( void* rootCtxPtr ) {
    return PIPELINE_SUCCESS;
}

static void initDecoder( Context* ctxPtr ) {
    memset(ctxPtr, 0, sizeof(Context));
    DtvccDecodeInitialize(ctxPtr, FALSE);
    LinkInfo linkInfo;
    linkInfo.linkType = DTVCC_DATA___TEXT_FILE;
    linkInfo.sourceType = DATA_TYPE_DECODED_708;
    linkInfo.sinkType = DATA_TYPE_708_TXT_FILE;
    linkInfo.NextBufferFnPtr = &captureNextBuffer;
    linkInfo.ShutdownFnPtr = &captureShutdown;
    DtvccDecodeAddSink(ctxPtr, linkInfo);
    numCaptured = 0;
}

static void freeDecoder( Context* ctxPtr ) {
    free(ctxPtr->dtvccDecodeCtxPtr);
    ctxPtr->dtvccDecodeCtxPtr = NULL;
}

// Feeds cc_data triples (type, byte 1, byte 2) to the decoder as one frame.
static void feed( Context* ctxPtr, const uint8* triples, int numBytes ) {
    Buffer* buffPtr = NewBuffer(BUFFER_TYPE_BYTES, numBytes);
    memcpy(buffPtr->dataPtr, triples, numBytes);
    buffPtr->numElements = numBytes;
    buffPtr->captionTime.source = CAPTION_TIME_PTS_NUMBERING;
    DtvccDecodeProcNextBuffer(ctxPtr, buffPtr);
}

// The G0 characters decoded so far, as a string.
static char* capturedText( void ) {
    static char text[257];
    int len = 0;
    for( int loop = 0; loop < numCaptured; loop++ ) {
        if( captured[loop].dtvccType == DTVCC_G0_CHAR ) text[len++] = captured[loop].data.g0char;
    }
    text[len] = '\0';
    return text;
}

static int countType( uint8 dtvccType ) {
    int count = 0;
    for( int loop = 0; loop < numCaptured; loop++ ) {
        if( captured[loop].dtvccType == dtvccType ) count++;
    }
    return count;
}

/*----------------------------------------------------------------------------*/
/*--                             Test Cases                                 --*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: DtvccDecodeProcNextBuffer() - Packet assembly
 |
 | TEST CASES:
 |     1) A 128-byte packet (packet_size_code 0) is assembled in full and
 |        decoded as soon as its last byte arrives.
 |     2) A packet cut short by the next Packet Start decodes only the bytes
 |        that arrived, not bytes left in the buffer by an earlier packet.
 -------------------------------------------------------------------------------*/
void utest__packetAssembly( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    Context ctx;

    TEST_START("Test Case: DtvccDecodeProcNextBuffer() - A 128-byte packet is decoded in full.");
    initDecoder(&ctx);
    // Header, three full blocks of 31 'A's, then a block of 29 'A's and a 'Z'.
    uint8 packet[128];
    int len = 0;
    packet[len++] = 0x40;                        // sequence 1, size code 0 (128 bytes)
    for( int block = 0; block < 4; block++ ) {
        int blockSize = (block < 3) ? 31 : 30;
        packet[len++] = 0x20 | blockSize;        // service 1
        for( int loop = 0; loop < blockSize; loop++ ) packet[len++] = 'A';
    }
    packet[len-1] = 'Z';
    ASSERT_EQ(128, len);
    uint8 triples[64 * 3];
    for( int loop = 0; loop < 64; loop++ ) {
        triples[loop*3] = (loop == 0) ? PS : PD;
        triples[loop*3+1] = packet[loop*2];
        triples[loop*3+2] = packet[loop*2+1];
    }
    feed(&ctx, triples, sizeof(triples));
    ASSERT_EQ(123, strlen(capturedText()));
    ASSERT_EQ('Z', capturedText()[122]);
    freeDecoder(&ctx);
    TEST_END

    TEST_START("Test Case: DtvccDecodeProcNextBuffer() - A truncated packet does not decode stale bytes.");
    initDecoder(&ctx);
    uint8 first[] = { PS, 0x43, 0x24, PD, 'a', 'b', PD, 'c', 'd' };   // 6 bytes: service 1, "abcd"
    feed(&ctx, first, sizeof(first));
    ASSERT_STREQ("abcd", capturedText());
    uint8 cut[] = { PS, 0x83, 0x24, PD, 'p', 'q' };                   // says "4 bytes", sends 2
    feed(&ctx, cut, sizeof(cut));
    uint8 next[] = { PS, 0xC1, 0x00 };                                // next packet start
    feed(&ctx, next, sizeof(next));
    ASSERT_STREQ("abcdpq", capturedText());
    freeDecoder(&ctx);
    TEST_END
}  // utest__packetAssembly()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: processServiceBlock() - Extended codes
 |
 | TEST CASES:
 |     1) A variable-length C3 command (EXT1 0x90-0x9F) is skipped using the
 |        length in its header byte; the text after it decodes.
 |     2) EXT1 as the last byte of a Service Block is dropped without reading
 |        past the block.
 -------------------------------------------------------------------------------*/
void utest__extendedCodes( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    Context ctx;

    TEST_START("Test Case: processServiceBlock() - A variable-length C3 command is skipped by its length.");
    initDecoder(&ctx);
    // 10 bytes: header, service 1 with 8 bytes: EXT1 0x90, header OSC + 2, 2 data bytes, "CDE".
    uint8 command[] = { PS, 0x05, 0x28, PD, 0x10, 0x90, PD, 0xC2, 0x41, PD, 0x42, 'C', PD, 'D', 'E' };
    feed(&ctx, command, sizeof(command));
    ASSERT_STREQ("CDE", capturedText());
    ASSERT_EQ(1, countType(DTVCC_C3_CMD));
    ASSERT_EQ(0, countType(DTVCC_G1_CHAR));
    freeDecoder(&ctx);
    TEST_END

    TEST_START("Test Case: processServiceBlock() - EXT1 at the end of a block is dropped.");
    initDecoder(&ctx);
    uint8 trailing[] = { PS, 0x02, 0x21, PD, 0x10, 0x00 };           // service 1, 1 byte: EXT1
    feed(&ctx, trailing, sizeof(trailing));
    ASSERT_EQ(0, numCaptured);
    freeDecoder(&ctx);
    TEST_END
}  // utest__extendedCodes()

/*------------------------------------------------------------------------------
 | UNTESTED FUNCTIONS:
 |    The C0/C1 command parsers and captioning-style bookkeeping are exercised
 |    by the integration/Python pipeline tests.
 -------------------------------------------------------------------------------*/
int main( int argc, char* argv[] ) {
    INIT_TEST_FRAMEWORK( argc, argv )

    TEST_SUITE_START("Test Suite: dtvcc_decode.c -- Packet Assembly");
    utest__packetAssembly( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: dtvcc_decode.c -- Extended Codes");
    utest__extendedCodes( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    SHUTDOWN_TEST_FRAMEWORK
}  // main()
