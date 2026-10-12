//
//  utest__line21_decode.c
//  Caption Inspector
//
//  Unit tests for the CEA-608 byte pair classification in line21_decode.c.
//
//  Many 608 codes share the first bytes 0x10-0x1F and are told apart only by
//  the second byte: Mid-Row Codes (0x20-0x2F), Special Characters (0x30-0x3F),
//  Extended Characters (0x20-0x3F on 0x12/0x13/0x1A/0x1B) and PACs (0x40-0x7F).
//  decodeCaptionData() must route each pair to the right decoder; a loose PAC
//  test used to swallow every Special and Extended Character.
//

#include "test_engine.h"
#include "../src/xform/line21_decode.c"

/*----------------------------------------------------------------------------*/
/*--                            Test Helpers                                --*/
/*----------------------------------------------------------------------------*/

static void initCtx( Line21DecodeCtx* ctxPtr ) {
    ctxPtr->processOnly = TRUE;
    ctxPtr->foundText = TEXT_REPORTED;
    ctxPtr->currentChannel[CEA608E_LINE21_FIELD_1_CC] = LINE21_CHANNEL_1;
    ctxPtr->currentChannel[CEA608E_LINE21_FIELD_2_CC] = LINE21_CHANNEL_3;
    ctxPtr->inXds[CEA608E_LINE21_FIELD_1_CC] = FALSE;
    ctxPtr->inXds[CEA608E_LINE21_FIELD_2_CC] = FALSE;
    ctxPtr->lastPair[CEA608E_LINE21_FIELD_1_CC] = 0;
    ctxPtr->lastPair[CEA608E_LINE21_FIELD_2_CC] = 0;
    for( int loop = 0; loop <= LINE21_MAX_NUM_CHANNELS; loop++ ) {
        ctxPtr->dataFound[loop] = FALSE;
        ctxPtr->isPopOnCaptioning[loop] = FALSE;
        ctxPtr->isRollUpCaptioning[loop] = FALSE;
        ctxPtr->isPaintOnCaptioning[loop] = FALSE;
        ctxPtr->captioningChange[loop] = FALSE;
        ctxPtr->captioningStart[loop] = FALSE;
    }
}

// Decodes one pair (parity already stripped) on the given field.
static boolean decodePair( Line21DecodeCtx* ctxPtr, uint8 field, uint8 byte1, uint8 byte2, Line21Code* codePtr ) {
    CaptionTime captionTime = { 0 };
    codePtr->fieldNum = field;
    codePtr->codeType = LINE21_CODE_TYPE_UNKNOWN;
    return decodeCaptionData(ctxPtr, byte1, byte2, codePtr, &captionTime);
}

/*----------------------------------------------------------------------------*/
/*--                             Test Cases                                 --*/
/*----------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decodeCaptionData() - Special and Extended Characters
 |
 | TEST CASES:
 |     1) 0x11 0x37 is the Special Character '♪', not a PAC.
 |     2) 0x19 0x39 on field 2 is a Special Character on channel 4.
 |     3) 0x12 0x2A is the Extended Character '—' from the Spanish/Misc/French set.
 |     4) 0x13 0x3F is the Extended Character '┘' from the Portuguese/German/Danish set.
 |     5) 0x1A 0x20 (channel 2) uses the Spanish/Misc/French set, chosen by the
 |        first byte rather than by the channel.
 -------------------------------------------------------------------------------*/
void utest__specialAndExtendedChars( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    Line21DecodeCtx ctx;
    Line21Code code;

    TEST_START("Test Case: decodeCaptionData() - 0x11 0x37 is a Special Character.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x37, &code));
    ASSERT_EQ(LINE21_SPECIAL_CHAR, code.codeType);
    ASSERT_EQ(LINE21_CHANNEL_1, code.channelNum);
    ASSERT_STREQ("♪", Line21DecodeSpecialCharacter(code.code.specialChar.spChar));
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x19 0x39 on field 2 is a Special Character on channel 4.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x19, 0x39, &code));
    ASSERT_EQ(LINE21_SPECIAL_CHAR, code.codeType);
    ASSERT_EQ(LINE21_CHANNEL_4, code.channelNum);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x12 0x2A is the Extended Character em dash.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x12, 0x2A, &code));
    ASSERT_EQ(LINE21_EXTENDED_CHAR, code.codeType);
    ASSERT_EQ(EXT_W_EURO_CHAR_SET_SPANISH_FRENCH, code.code.extendedChar.charSet);
    ASSERT_STREQ("—", Line21DecodeExtendedCharacter(code.code.extendedChar.charSet, code.code.extendedChar.exChar));
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x13 0x3F is the Extended Character lower right corner.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x13, 0x3F, &code));
    ASSERT_EQ(LINE21_EXTENDED_CHAR, code.codeType);
    ASSERT_EQ(EXT_W_EURO_CHAR_SET_DUTCH_GERMAN, code.code.extendedChar.charSet);
    ASSERT_STREQ("┘", Line21DecodeExtendedCharacter(code.code.extendedChar.charSet, code.code.extendedChar.exChar));
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x1A 0x20 on channel 2 uses the Spanish/Misc/French set.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x1A, 0x20, &code));
    ASSERT_EQ(LINE21_EXTENDED_CHAR, code.codeType);
    ASSERT_EQ(LINE21_CHANNEL_2, code.channelNum);
    ASSERT_STREQ("Á", Line21DecodeExtendedCharacter(code.code.extendedChar.charSet, code.code.extendedChar.exChar));
    TEST_END
}  // utest__specialAndExtendedChars()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: decodeCaptionData() - PACs, Mid-Row and Tab codes
 |
 | TEST CASES:
 |     1) 0x11 0x40 is a PAC (row 1), 0x10 0x40 is a PAC (row 11).
 |     2) Mid-Row foreground 0x11 0x21 is underlined, not partially transparent.
 |     3) Mid-Row background 0x10 0x21 is partially transparent, not underlined.
 |     4) 0x17 0x2D / 0x2E / 0x2F are Background Transparent, Foreground Black
 |        and Foreground Black Underline, not PACs.
 |     5) 0x17 0x21 is Tab Offset 1.
 |     6) Unassigned pairs (0x14 0x05, 0x17 0x2B) are not decoded at all.
 |     7) 0x10 0x60-0x7F is not a PAC: row 11 only uses 0x40-0x5F.
 |     8) 0x17 0x24-0x2A are the Table 4 character set selection codes.
 -------------------------------------------------------------------------------*/
void utest__controlCodes( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    Line21DecodeCtx ctx;
    Line21Code code;

    TEST_START("Test Case: decodeCaptionData() - 0x11 0x40 and 0x10 0x40 are PACs.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x40, &code));
    ASSERT_EQ(LINE21_PREAMBLE_ACCESS_CODE, code.codeType);
    ASSERT_EQ(1, code.code.pac.rowNumber);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x10, 0x40, &code));
    ASSERT_EQ(LINE21_PREAMBLE_ACCESS_CODE, code.codeType);
    ASSERT_EQ(11, code.code.pac.rowNumber);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - Mid-Row foreground bit 0 is underline.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x21, &code));
    ASSERT_EQ(LINE21_MID_ROW_CONTROL_CODE, code.codeType);
    ASSERT_EQ(MIDROW_FOREGROUND_STYLE, code.code.midRow.backgroundOrForeground);
    ASSERT_EQ(TRUE, code.code.midRow.isUnderlined);
    ASSERT_EQ(FALSE, code.code.midRow.isPartiallyTransparent);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - Mid-Row background bit 0 is partial transparency.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x10, 0x29, &code));
    ASSERT_EQ(LINE21_MID_ROW_CONTROL_CODE, code.codeType);
    ASSERT_EQ(MIDROW_BACKGROUND_COLOR, code.code.midRow.backgroundOrForeground);
    ASSERT_EQ(MIDROW_BG_COLOR_RED, code.code.midRow.backgroundForgroundData.color);
    ASSERT_EQ(TRUE, code.code.midRow.isPartiallyTransparent);
    ASSERT_EQ(FALSE, code.code.midRow.isUnderlined);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x17 0x2D-0x2F are attribute codes, not PACs.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x17, 0x2D, &code));
    ASSERT_EQ(LINE21_MID_ROW_CONTROL_CODE, code.codeType);
    ASSERT_EQ(MIDROW_BACKGROUND_COLOR, code.code.midRow.backgroundOrForeground);
    ASSERT_EQ(MIDROW_BG_COLOR_TRANSPARENT, code.code.midRow.backgroundForgroundData.color);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x17, 0x2E, &code));
    ASSERT_EQ(LINE21_MID_ROW_CONTROL_CODE, code.codeType);
    ASSERT_EQ(MIDROW_FG_STYLE_BLACK, code.code.midRow.backgroundForgroundData.style);
    ASSERT_EQ(FALSE, code.code.midRow.isUnderlined);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x1F, 0x2F, &code));
    ASSERT_EQ(LINE21_MID_ROW_CONTROL_CODE, code.codeType);
    ASSERT_EQ(LINE21_CHANNEL_4, code.channelNum);
    ASSERT_EQ(MIDROW_FG_STYLE_BLACK, code.code.midRow.backgroundForgroundData.style);
    ASSERT_EQ(TRUE, code.code.midRow.isUnderlined);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x17 0x21 is Tab Offset 1.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x17, 0x21, &code));
    ASSERT_EQ(LINE21_TAB_CONTROL_CODE, code.codeType);
    ASSERT_EQ(TAB_CMD_TO1, code.code.tabControl.tabOffset);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - Unassigned pairs are not decoded.");
    initCtx(&ctx);
    ASSERT_EQ(FALSE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x14, 0x05, &code));
    ASSERT_EQ(FALSE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x17, 0x2B, &code));
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x10 0x60 and 0x18 0x7F are not PACs.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x10, 0x5F, &code));
    ASSERT_EQ(LINE21_PREAMBLE_ACCESS_CODE, code.codeType);
    ASSERT_EQ(FALSE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x10, 0x60, &code));
    ASSERT_EQ(FALSE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x18, 0x7F, &code));
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x60, &code));
    ASSERT_EQ(LINE21_PREAMBLE_ACCESS_CODE, code.codeType);
    TEST_END

    TEST_START("Test Case: decodeCaptionData() - 0x17 0x24-0x2A select a character set.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x17, 0x24, &code));
    ASSERT_EQ(LINE21_TAB_CONTROL_CODE, code.codeType);
    ASSERT_EQ(0x24, code.code.tabControl.tabOffset);
    ASSERT_EQ(TRUE, decodePair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x1F, 0x2A, &code));
    ASSERT_EQ(LINE21_CHANNEL_4, code.channelNum);
    TEST_END
}  // utest__controlCodes()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: isXdsPair()
 |
 | TEST CASES:
 |     1) A complete field 2 XDS packet (start, data, end) is all XDS, and the
 |        caption text after it is not.
 |     2) A caption control code interrupts an XDS packet.
 |     3) 0x01-0x0F first bytes on field 1 are not XDS.
 -------------------------------------------------------------------------------*/
void utest__xds( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    Line21DecodeCtx ctx;

    TEST_START("Test Case: isXdsPair() - A complete XDS packet is skipped, captions after it are not.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x01));   // Current Class Start
    ASSERT_EQ(TRUE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x41));   // data
    ASSERT_EQ(TRUE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x0F));   // End / checksum
    ASSERT_EQ(FALSE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x41));  // caption text again
    TEST_END

    TEST_START("Test Case: isXdsPair() - A caption control code interrupts an XDS packet.");
    initCtx(&ctx);
    ASSERT_EQ(TRUE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x05));   // Channel Class Start
    ASSERT_EQ(TRUE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x20));   // data
    ASSERT_EQ(FALSE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x15));  // CC3 control code
    ASSERT_EQ(FALSE, ctx.inXds[CEA608E_LINE21_FIELD_2_CC]);
    ASSERT_EQ(FALSE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x41));
    TEST_END

    TEST_START("Test Case: isXdsPair() - Field 1 never carries XDS.");
    initCtx(&ctx);
    ASSERT_EQ(FALSE, isXdsPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x01));
    TEST_END
}  // utest__xds()

/*------------------------------------------------------------------------------
 | FUNCTION UNDER TEST: isRedundantPair()
 |
 | TEST CASES:
 |     1) The second of two identical pairs is redundant, a third is not.
 |     2) A NULL pair between two identical pairs makes the second one new.
 |     3) Identical pairs on different fields are not redundant, and parity
 |        bits are ignored.
 -------------------------------------------------------------------------------*/
void utest__redundantPairs( TEST_SUITE_RECEIVED_ARGUMENTS ) {
    TEST_INITIALIZE
    Line21DecodeCtx ctx;

    TEST_START("Test Case: isRedundantPair() - The second copy is redundant, a third is new.");
    initCtx(&ctx);
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x39));
    ASSERT_EQ(TRUE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x39));
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x39));
    TEST_END

    TEST_START("Test Case: isRedundantPair() - A NULL pair in between makes the next copy new.");
    initCtx(&ctx);
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x12, 0x2A));
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x80, 0x80));
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x12, 0x2A));
    TEST_END

    TEST_START("Test Case: isRedundantPair() - Fields are tracked separately and parity is ignored.");
    initCtx(&ctx);
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x91, 0xB9));
    ASSERT_EQ(FALSE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_2_CC, 0x91, 0xB9));
    ASSERT_EQ(TRUE, isRedundantPair(&ctx, CEA608E_LINE21_FIELD_1_CC, 0x11, 0x39));
    TEST_END
}  // utest__redundantPairs()

/*------------------------------------------------------------------------------
 | UNTESTED FUNCTIONS:
 |    Line21DecodeProcNextBuffer() and the captioning-style bookkeeping are
 |    exercised by the integration/Python pipeline tests.
 -------------------------------------------------------------------------------*/
int main( int argc, char* argv[] ) {
    INIT_TEST_FRAMEWORK( argc, argv )

    TEST_SUITE_START("Test Suite: line21_decode.c -- Special and Extended Characters");
    utest__specialAndExtendedChars( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: line21_decode.c -- Control Codes");
    utest__controlCodes( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: line21_decode.c -- Extended Data Services");
    utest__xds( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    TEST_SUITE_START("Test Suite: line21_decode.c -- Redundant Pairs");
    utest__redundantPairs( &tmpNumSuccessfulTests, &tmpNumFailedTests );
    TEST_SUITE_END

    SHUTDOWN_TEST_FRAMEWORK
}  // main()
