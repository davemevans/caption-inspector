//
//  utest__cc_data_output.c
//  Caption Inspector
//
//  Unit tests for the CEA-708 Service Block handling in cc_data_output.c,
//  focused on the Service Number bounds check.
//
//  A Service Number decoded from the stream is used as (currentService - 1),
//  an index into txtStr708[NUM_708_SERVICES]. An Extended Service Number of 0
//  would index txtStr708[-1] (an out-of-bounds write into the caller's stack).
//  decodeServiceBlockHeaderExtension() must reject Service 0 and keep every
//  value it hands on as currentService within 1..NUM_708_SERVICES.
//

#include "test_engine.h"
#include "../src/sink/cc_data_output.c"

/*----------------------------------------------------------------------------*/
/*--                            Test Helpers                                --*/
/*----------------------------------------------------------------------------*/

static void clearTextString( TextString* txtStr ) {
    for( int loop = 0; loop < NUM_608_CHANNELS; loop++ ) {
        txtStr->txtStr608[loop][0] = '\0';
    }
    for( int loop = 0; loop < NUM_708_SERVICES; loop++ ) {
        txtStr->txtStr708[loop][0] = '\0';
    }
}

static boolean allServiceTextEmpty( TextString* txtStr ) {
    for( int loop = 0; loop < NUM_708_SERVICES; loop++ ) {
        if( strlen(txtStr->txtStr708[loop]) != 0 ) return FALSE;
    }
    return TRUE;
}

/*----------------------------------------------------------------------------*/
/*--                             Test Cases                                 --*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decodeServiceBlockHeaderExtension()
 |
 | TEST CASES:
 |     1) Extended Service Number 0 is rejected (would index txtStr708[-1]).
 |     2) Extended Service Numbers 1..6 are irregular but usable.
 |     3) Extended Service Number 7 (the minimum valid) is used as-is.
 |     4) Extended Service Number 63 (the maximum) is used as-is.
 -------------------------------------------------------------------------------*/
void utest__decodeServiceBlockHeaderExtension( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    CcDataOutputCtx ctx;
    char tagStr[CC_DATA_ELEMENT_HALF_TAG_STR_SIZE];
    char decStr[CC_DATA_ELEMENT_HALF_DEC_STR_SIZE];
    char errStr[CEA708_ERROR_STR_SIZE + 1];

    TEST_START("Test Case: decodeServiceBlockHeaderExtension() - Service 0 is rejected as invalid.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = 42;                       // some prior valid Service
    ctx.cea708State = CEA708_STATE_EXTENDED_SRV_NUM;
    errStr[0] = '\0';
    ERROR_EXPECTED
    decodeServiceBlockHeaderExtension(&ctx, 0x00, tagStr, decStr, errStr);
    // Service 0 must NOT become the current Service (that would index [-1]); the
    // rest of the block is skipped via CEA708_STATE_UNKNOWN.
    ASSERT_EQ(UNKNOWN_SERVICE, ctx.currentService);
    ASSERT_EQ(CEA708_STATE_UNKNOWN, ctx.cea708State);
    TEST_END

    TEST_START("Test Case: decodeServiceBlockHeaderExtension() - Service 3 (irregular) is used.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = UNKNOWN_SERVICE;
    ctx.cea708State = CEA708_STATE_EXTENDED_SRV_NUM;
    errStr[0] = '\0';
    decodeServiceBlockHeaderExtension(&ctx, 0x03, tagStr, decStr, errStr);
    ASSERT_EQ(3, ctx.currentService);
    ASSERT_EQ(CEA708_STATE_DATA_WAIT, ctx.cea708State);
    TEST_END

    TEST_START("Test Case: decodeServiceBlockHeaderExtension() - Service 7 (minimum valid) is used.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = UNKNOWN_SERVICE;
    ctx.cea708State = CEA708_STATE_EXTENDED_SRV_NUM;
    errStr[0] = '\0';
    decodeServiceBlockHeaderExtension(&ctx, 0x07, tagStr, decStr, errStr);
    ASSERT_EQ(7, ctx.currentService);
    ASSERT_EQ(CEA708_STATE_DATA_WAIT, ctx.cea708State);
    TEST_END

    TEST_START("Test Case: decodeServiceBlockHeaderExtension() - Service 63 (maximum) is used.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = UNKNOWN_SERVICE;
    ctx.cea708State = CEA708_STATE_EXTENDED_SRV_NUM;
    errStr[0] = '\0';
    // 0x3F == 63 == NUM_708_SERVICES, the largest in-range index (txtStr708[62]).
    decodeServiceBlockHeaderExtension(&ctx, 0x3F, tagStr, decStr, errStr);
    ASSERT_EQ(63, ctx.currentService);
    ASSERT_EQ(CEA708_STATE_DATA_WAIT, ctx.cea708State);
    TEST_END
}  // utest__decodeServiceBlockHeaderExtension()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decodePacketData() (Extended Service Block sequence)
 |
 | TEST CASES:
 |     1) Data following an invalid (Service 0) extended header is discarded,
 |        not written into any Service buffer, and the state machine recovers.
 |     2) Data following a valid extended header IS decoded into that Service
 |        (proving the bounds check did not break normal decoding).
 -------------------------------------------------------------------------------*/
void utest__extendedServiceBlockData( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    CcDataOutputCtx ctx;
    TextString txtStr;
    char tagStr[CC_DATA_ELEMENT_HALF_TAG_STR_SIZE];
    char decStr[CC_DATA_ELEMENT_HALF_DEC_STR_SIZE];
    char errStr[CEA708_ERROR_STR_SIZE + 1];

    TEST_START("Test Case: decodePacketData() - Block data after a Service 0 header is discarded.");
    // Arrange: as though a Service-7 extended block header (2 data bytes) was just
    // parsed and we are waiting on the extended Service Number byte.
    ctx.cea708ErrNum = 0;
    ctx.currentService = UNKNOWN_SERVICE;
    ctx.cea708State = CEA708_STATE_EXTENDED_SRV_NUM;
    ctx.cea708PacketBytesRemaining = 4;
    ctx.cea708BlockBytesRemaining = 2;
    clearTextString(&txtStr);

    // Extended Service Number 0x00 -> invalid; block is dropped.
    errStr[0] = '\0';
    ERROR_EXPECTED
    decodePacketData(&ctx, 0x00, tagStr, decStr, &txtStr, errStr);
    ASSERT_EQ(UNKNOWN_SERVICE, ctx.currentService);
    ASSERT_EQ(CEA708_STATE_UNKNOWN, ctx.cea708State);

    // A following G0 character byte ('A') must be ignored, not written anywhere.
    errStr[0] = '\0';
    decodePacketData(&ctx, 0x41, tagStr, decStr, &txtStr, errStr);
    ASSERT_EQ(TRUE, allServiceTextEmpty(&txtStr));

    // Final block byte returns us to waiting for the next Service Block Header.
    errStr[0] = '\0';
    decodePacketData(&ctx, 0x42, tagStr, decStr, &txtStr, errStr);
    ASSERT_EQ(TRUE, allServiceTextEmpty(&txtStr));
    ASSERT_EQ(CEA708_STATE_BLOCK_HEADER_WAIT, ctx.cea708State);
    TEST_END

    TEST_START("Test Case: decodePacketData() - Block data after a valid extended header decodes.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = UNKNOWN_SERVICE;
    ctx.cea708State = CEA708_STATE_EXTENDED_SRV_NUM;
    ctx.cea708PacketBytesRemaining = 4;
    ctx.cea708BlockBytesRemaining = 2;
    clearTextString(&txtStr);

    // Extended Service Number 40 -> valid; subsequent G0 text lands in Service 40.
    errStr[0] = '\0';
    decodePacketData(&ctx, 0x28, tagStr, decStr, &txtStr, errStr);
    ASSERT_EQ(40, ctx.currentService);
    ASSERT_EQ(CEA708_STATE_DATA_WAIT, ctx.cea708State);

    errStr[0] = '\0';
    decodePacketData(&ctx, 0x41, tagStr, decStr, &txtStr, errStr);
    // Something was written for Service 40 (index 39), and only there.
    ASSERT_NEQ(0, strlen(txtStr.txtStr708[39]));
    ASSERT_EQ(0, strlen(txtStr.txtStr708[38]));
    ASSERT_EQ(0, strlen(txtStr.txtStr708[40]));
    TEST_END
}  // utest__extendedServiceBlockData()

/*------------------------------------------------------------------------------
 | UNTESTED FUNCTIONS:
 |    Most of cc_data_output.c drives file output and is exercised by the
 |    integration/Python pipeline tests; this suite covers the Service Number
 |    bounds check specifically.
 -------------------------------------------------------------------------------*/
int main( int argc, char* argv[] ) {
    INIT_TEST_FRAMEWORK( argc, argv )

    TEST_SUITE_START("Test Suite: cc_data_output.c -- decodeServiceBlockHeaderExtension()");
    utest__decodeServiceBlockHeaderExtension( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: cc_data_output.c -- Extended Service Block Data");
    utest__extendedServiceBlockData( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    SHUTDOWN_TEST_FRAMEWORK
}  // main()
