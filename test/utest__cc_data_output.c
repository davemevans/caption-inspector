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
    for( int loop = 0; loop < NUM_608_SERVICES; loop++ ) {
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

static void init608Ctx( CcDataOutputCtx* ctxPtr ) {
    ctxPtr->currentChannel[CC_DATA_TYPE__FIELD_1] = UNKOWN_CHANNEL;
    ctxPtr->currentChannel[CC_DATA_TYPE__FIELD_2] = UNKOWN_CHANNEL;
    ctxPtr->inXds[CC_DATA_TYPE__FIELD_1] = FALSE;
    ctxPtr->inXds[CC_DATA_TYPE__FIELD_2] = FALSE;
    ctxPtr->last608Pair[CC_DATA_TYPE__FIELD_1] = 0;
    ctxPtr->last608Pair[CC_DATA_TYPE__FIELD_2] = 0;
    for( int loop = 0; loop <= LINE21_MAX_NUM_CHANNELS; loop++ ) {
        ctxPtr->isTextMode[loop] = FALSE;
    }
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
    char errStr[CEA708_ERROR_STR_SIZE];

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
    char errStr[CEA708_ERROR_STR_SIZE];

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
 | FUNCTION UNDER TEST: add708Error()
 |
 | TEST CASES:
 |     1) Many errors on one line stay within the CEA708_ERROR_STR_SIZE buffer
 |        (the bytes after it are untouched) and the string stays terminated.
 -------------------------------------------------------------------------------*/
void utest__add708Error( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    CcDataOutputCtx ctx;
    struct {
        char errStr[CEA708_ERROR_STR_SIZE];
        char guard[8];
    } buf;

    TEST_START("Test Case: add708Error() - Many errors do not overrun the error string.");
    ctx.cea708ErrNum = 0x100;   // three hex digits per reference fills the buffer quickly
    buf.errStr[0] = '\0';
    memset(buf.guard, 0x5A, sizeof(buf.guard));
    for( int loop = 0; loop < 20; loop++ ) {
        add708Error(&ctx, buf.errStr);
    }
    ASSERT_EQ(TRUE, strlen(buf.errStr) < CEA708_ERROR_STR_SIZE);
    boolean guardIntact = TRUE;
    for( int loop = 0; loop < sizeof(buf.guard); loop++ ) {
        if( buf.guard[loop] != 0x5A ) guardIntact = FALSE;
    }
    ASSERT_EQ(TRUE, guardIntact);
    ASSERT_EQ(0x100 + 20, ctx.cea708ErrNum);
    TEST_END
}  // utest__add708Error()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decode608Pair()
 |
 | TEST CASES:
 |     1) A basic pair with the solid block (0x7F, three bytes of UTF-8) keeps
 |        whole glyphs and the column width in screen cells.
 |     2) A NULL then a character shows and collects the character.
 |     3) 0x14 on field 2 is a CC3 control code.
 |     4) A Special Character before any control code sets the channel rather
 |        than tripping an assert.
 |     5) Text after TR is collected as T1, and after EOC as CC1 again.
 -------------------------------------------------------------------------------*/
void utest__decode608Pair( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    CcDataOutputCtx ctx;
    CcdElemOut elem;
    TextString txtStr;

    TEST_START("Test Case: decode608Pair() - The solid block keeps its bytes and the column width.");
    init608Ctx(&ctx);
    clearTextString(&txtStr);
    ctx.currentChannel[CC_DATA_TYPE__FIELD_1] = 1;
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_1, 0xFE, 0x7F, &elem, &txtStr);
    ASSERT_STREQ("Ch1: \"ñ■\"", elem.tagStr);
    ASSERT_EQ(CC_DATA_ELEMENT_TAG_STR_SIZE - 1, displayCells(elem.tagStr));
    ASSERT_STREQ("Chan-1:  \"ñ\"  \"■\"", elem.decStr);
    ASSERT_EQ(CC_DATA_ELEMENT_DEC_STR_SIZE - 1, displayCells(elem.decStr));
    ASSERT_STREQ("ñ■", txtStr.txtStr608[0]);
    TEST_END

    TEST_START("Test Case: decode608Pair() - A NULL then a character is that character.");
    init608Ctx(&ctx);
    clearTextString(&txtStr);
    ctx.currentChannel[CC_DATA_TYPE__FIELD_1] = 1;
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_1, 0x80, 0xC1, &elem, &txtStr);
    ASSERT_STREQ("Ch1 - \"A\"", elem.tagStr);
    ASSERT_STREQ("A", txtStr.txtStr608[0]);
    TEST_END

    TEST_START("Test Case: decode608Pair() - 0x14 on field 2 is a CC3 control code.");
    init608Ctx(&ctx);
    clearTextString(&txtStr);
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_2, 0x94, 0x20, &elem, &txtStr);
    ASSERT_STREQ("Ch3 {RCL}", elem.tagStr);
    ASSERT_EQ(3, ctx.currentChannel[CC_DATA_TYPE__FIELD_2]);
    TEST_END

    TEST_START("Test Case: decode608Pair() - A Special Character sets the channel.");
    init608Ctx(&ctx);
    clearTextString(&txtStr);
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_2, 0x19, 0x37, &elem, &txtStr);
    ASSERT_EQ(4, ctx.currentChannel[CC_DATA_TYPE__FIELD_2]);
    ASSERT_STREQ("♪", txtStr.txtStr608[3]);
    TEST_END

    TEST_START("Test Case: decode608Pair() - Text Mode text is collected as T1.");
    init608Ctx(&ctx);
    clearTextString(&txtStr);
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_1, 0x94, 0x2A, &elem, &txtStr);   // TR
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_1, 0xC1, 0xC2, &elem, &txtStr);   // "AB"
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_1, 0x94, 0x2F, &elem, &txtStr);   // EOC
    decode608Pair(&ctx, CC_DATA_TYPE__FIELD_1, 0x43, 0xC4, &elem, &txtStr);   // "CD"
    ASSERT_STREQ("AB", txtStr.txtStr608[NUM_608_CHANNELS]);
    ASSERT_STREQ("CD", txtStr.txtStr608[0]);
    TEST_END
}  // utest__decode608Pair()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decodePacketData() - Variable-length C3 commands
 |
 | TEST CASES:
 |     1) EXT1 0x90 is followed by a header byte whose low 5 bits count the data
 |        bytes; those are skipped and the next byte is text again.
 |     2) A header with a length of 0 ends the command at the header.
 -------------------------------------------------------------------------------*/
void utest__variableLengthC3( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    CcDataOutputCtx ctx;
    TextString txtStr;
    char tagStr[CC_DATA_ELEMENT_HALF_TAG_STR_SIZE];
    char decStr[CC_DATA_ELEMENT_HALF_DEC_STR_SIZE];
    char errStr[CEA708_ERROR_STR_SIZE];
    uint8 command[] = { 0x10, 0x90, 0xC2, 0x41, 0x42, 0x43 };   // EXT1, 0x90, OSC with 2 bytes, 2 bytes, 'C'
    uint8 empty[] = { 0x10, 0x9F, 0xC0, 0x44 };                 // EXT1, 0x9F, OSC with 0 bytes, 'D'

    TEST_START("Test Case: decodePacketData() - A variable-length C3 command is skipped by its length.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = 1;
    ctx.cea708State = CEA708_STATE_DATA_WAIT;
    ctx.cea708PacketBytesRemaining = 20;
    ctx.cea708BlockBytesRemaining = 20;
    clearTextString(&txtStr);
    for( int loop = 0; loop < sizeof(command); loop++ ) {
        errStr[0] = '\0';
        decodePacketData(&ctx, command[loop], tagStr, decStr, &txtStr, errStr);
    }
    ASSERT_STREQ("C", txtStr.txtStr708[0]);
    ASSERT_EQ(CEA708_STATE_DATA_WAIT, ctx.cea708State);
    TEST_END

    TEST_START("Test Case: decodePacketData() - A variable-length C3 command can have no data.");
    ctx.cea708ErrNum = 0;
    ctx.currentService = 1;
    ctx.cea708State = CEA708_STATE_DATA_WAIT;
    ctx.cea708PacketBytesRemaining = 20;
    ctx.cea708BlockBytesRemaining = 20;
    clearTextString(&txtStr);
    for( int loop = 0; loop < sizeof(empty); loop++ ) {
        errStr[0] = '\0';
        decodePacketData(&ctx, empty[loop], tagStr, decStr, &txtStr, errStr);
    }
    ASSERT_STREQ("D", txtStr.txtStr708[0]);
    TEST_END
}  // utest__variableLengthC3()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: appendText()
 |
 | TEST CASES:
 |     1) A multi-byte glyph that does not fit is dropped whole, and a shorter
 |        one that fits is still added.
 -------------------------------------------------------------------------------*/
void utest__appendText( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    char text[MAX_CHARS_PER_LINE_PER_CHANNEL];

    TEST_START("Test Case: appendText() - A glyph that does not fit is not cut.");
    memset(text, 'x', MAX_CHARS_PER_LINE_PER_CHANNEL - 3);
    text[MAX_CHARS_PER_LINE_PER_CHANNEL - 3] = '\0';
    appendText(text, "■");
    ASSERT_EQ(MAX_CHARS_PER_LINE_PER_CHANNEL - 3, strlen(text));
    appendText(text, "é");
    ASSERT_EQ(MAX_CHARS_PER_LINE_PER_CHANNEL - 1, strlen(text));
    appendText(text, "a");
    ASSERT_EQ(MAX_CHARS_PER_LINE_PER_CHANNEL - 1, strlen(text));
    TEST_END
}  // utest__appendText()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: displayCells() and layoutGlyph()
 |
 | TEST CASES:
 |     1) Accented, symbol and solid-block glyphs take one cell, CJK two.
 |     2) A glyph is right-justified after its prefix to fill the column in
 |        cells, and the prefix is shortened when a wide glyph needs the room.
 -------------------------------------------------------------------------------*/
void utest__glyphLayout( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    char half[CC_DATA_ELEMENT_HALF_DEC_BUF_SIZE];

    TEST_START("Test Case: displayCells() - Cells, not bytes.");
    ASSERT_EQ(1, displayCells("A"));
    ASSERT_EQ(1, displayCells("é"));
    ASSERT_EQ(1, displayCells("■"));
    ASSERT_EQ(1, displayCells("™"));
    ASSERT_EQ(2, displayCells("左"));
    ASSERT_EQ(2, displayCells("ア"));
    ASSERT_EQ(5, displayCells("A左é■"));
    TEST_END

    TEST_START("Test Case: layoutGlyph() - Glyphs fill the column in cells.");
    layoutGlyph(half, sizeof(half), CC_DATA_ELEMENT_HALF_TAG_STR_SIZE - 1, "G1:", "é");
    ASSERT_STREQ("G1:é", half);
    layoutGlyph(half, sizeof(half), CC_DATA_ELEMENT_HALF_DEC_STR_SIZE - 1, "G1:", "\"é\"");
    ASSERT_STREQ("G1:  \"é\"", half);
    layoutGlyph(half, sizeof(half), CC_DATA_ELEMENT_HALF_TAG_STR_SIZE - 1, "G1:", "￥");
    ASSERT_STREQ("G1￥", half);
    ASSERT_EQ(CC_DATA_ELEMENT_HALF_TAG_STR_SIZE - 1, displayCells(half));
    layoutGlyph(half, sizeof(half), CC_DATA_ELEMENT_HALF_DEC_STR_SIZE - 1, "G2:", "'™'");
    ASSERT_STREQ("G2:  '™'", half);
    TEST_END
}  // utest__glyphLayout()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: DtvccDecodeG1CharSet()
 |
 | TEST CASES:
 |     1) G1 is ISO 8859-1: the currency signs are the Latin-1 characters, not
 |        their fullwidth forms, and 0xF6-0xFA are not shifted by one.
 -------------------------------------------------------------------------------*/
void utest__g1CharSet( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE

    TEST_START("Test Case: DtvccDecodeG1CharSet() - G1 matches Latin-1.");
    ASSERT_STREQ("¢", (char*)DtvccDecodeG1CharSet(0xA2));
    ASSERT_STREQ("£", (char*)DtvccDecodeG1CharSet(0xA3));
    ASSERT_STREQ("¥", (char*)DtvccDecodeG1CharSet(0xA5));
    ASSERT_STREQ("¬", (char*)DtvccDecodeG1CharSet(0xAC));
    ASSERT_STREQ("¯", (char*)DtvccDecodeG1CharSet(0xAF));
    ASSERT_STREQ("ö", (char*)DtvccDecodeG1CharSet(0xF6));
    ASSERT_STREQ("÷", (char*)DtvccDecodeG1CharSet(0xF7));
    ASSERT_STREQ("ú", (char*)DtvccDecodeG1CharSet(0xFA));
    ASSERT_STREQ("ÿ", (char*)DtvccDecodeG1CharSet(0xFF));
    TEST_END
}  // utest__g1CharSet()

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

    TEST_SUITE_START("Test Suite: cc_data_output.c -- add708Error()");
    utest__add708Error( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: cc_data_output.c -- decode608Pair()");
    utest__decode608Pair( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: cc_data_output.c -- Variable-length C3 Commands");
    utest__variableLengthC3( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: cc_data_output.c -- appendText()");
    utest__appendText( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: cc_data_output.c -- Glyph Layout");
    utest__glyphLayout( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: cc_data_output.c -- G1 Characters");
    utest__g1CharSet( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    SHUTDOWN_TEST_FRAMEWORK
}  // main()
