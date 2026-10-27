CEA-608 Decode File (.608)
==========================

Caption Inspector will output decoded CEA-608 as text with inline tags from the 608 [spec](https://en.wikipedia.org/wiki/EIA-608).
Each Channel of 608 captions that are found are written to their own file, which contains the channel number appended
to the end of the filename. This happens as a result of finding 'any' captions for a specific channel, and does not
necessarily indicate that a specific channel is a valid channel with captions. Sometimes it is just random garbage
that is in the captioning and decodes as data for a specific channel which was not intended to have captions.

Caption channels CC1-CC4 are written to `-C1.608` to `-C4.608`. Text Mode services T1-T4 share those data channels:
after a Text Restart `{TR}` or Resume Text Display `{RTD}`, a channel's data is written to `-T1.608` to `-T4.608`
until a caption code (`{RCL}`, `{RDC}`, `{RU2}`-`{RU4}` or `{EOC}`) returns it to captions. `{EDM}` and `{ENM}` always
act on caption memory, so they stay in the caption file. On field 2 an XDS packet also ends Text Mode.

Field 2 caption control codes should use the first bytes 0x15/0x1D, but some encoders send the field 1 bytes 0x14/0x1C
there too. The field decides the channel, so these are decoded as CC3/CC4, with a warning in the debug log.

XDS packets on field 2 are not captions, so they are left out of the `.608` files; the `.ccd` file shows them.

Here is an example from a Decoded CEA-608 file that was produced by Caption Inspector (CC1 of
`test/media/BigBuckBunny_256x144-24fps.ts`), followed by a description of the different elements in the file.

```
00:00:00,291 - {RCL}
00:00:00,333 - {RCL} {R14:C12} {R14:C12} "- 20." {R15:C4} {R15:C4} {TO2} "- THAT'S STRETCH"
00:00:01,042 - {RCL}
00:00:01,084 - {RCL}
00:00:01,126 - {EDM}
00:00:01,167 - {EDM}
00:00:01,209 - {EOC}
```

Timestamp
---------

On the leftmost portion of the document is the timestamp. Each Closed Caption Data (CCD) packet has a timestamp, which
corresponds with the frame of the asset containing the CCD packet. For readability sake, the timestamp is only printed
out when specific Global Control Codes are encountered. There are three types of timestamps: Frame Number, Drop Frame
Number, and Millisecond. This refers to the last number, and the symbol that separates it from the second to last number.

Drop Frame Number Timestamps format --- `<Hour>:<Minute>:<Second>;<Frame>`

Frame Number Timestamps format --- `<Hour>:<Minute>:<Second>:<Frame>`

Millisecond Timestamps format --- `<Hour>:<Minute>:<Second>,<Millisecond>`

The Frame Number vs. Drop Frame Number is differentiated by using either a `:` for Frame or a `;` for Drop Frame. For
a description of what Drop Frame is, and why we have it, please look [here](http://www.bodenzord.com/archives/79).

Text
----

The text is merely wrapped in quotes, with all whitespaces preserved inside of the quotes. If there are quotes in
the text, they will be printed out directly and will not be escaped out. The file is UTF-8.

Special Characters (e.g. `♪`, `®`, and the transparent space shown as `☐`) and Extended Characters (e.g. `Á`, `ß`, `—`)
are printed inside the quotes with the rest of the text. An Extended Character is sent after a fallback character
that a decoder without the extended set would show, and replaces it, so `"AÁ"` is displayed as `Á`. Like control
codes, these characters are usually sent twice in a row; the repeated copy is ignored by a decoder, so it is left out
of the text (a third copy is a new character).


Inline Representation of Global Control Codes
---------------------------------------------

The Global Control are represented inline by wrapping the mnemonic code with braces: `{Code}` The codes can be
interpreted using the following table.

![Global Control Code Decode](./html/assets/content-images/608GlobalCtrlCodes.png)


Inline Representation of Preamble Access Codes (PAC)
----------------------------------------------------

There are two types of PACs, Address and Style. Address PACs relate specifically to positioning, and Style PACs
describe the color of the captions.

![Address and Style PACs](./html/assets/content-images/608PACs.png)

Inline Representation of Mid Row Style Change Code
--------------------------------------------------

The Mid Row Style Change codes affect the current row that they are encountered in. The new style remains in effect
until either next change or end of row signaled by a Global Control Code or Preamble Access Code. The Mid Row Style
Change will either change the Foreground Color or the Background Color. Additionally, it will signal whether the
text is either Underlined, Partially Transparent, or both.

![Midrow](./html/assets/content-images/608MidRow.png)

In practice a background code only carries the Partial Transparency flag and a foreground code only carries the
Underline flag, so the output is e.g. `{BG-Red:PT}` or `{FG-White:UL}`. The attribute codes sent with the Tab Offset
prefix are shown the same way: Background Transparent as `{BG-Transparent}`, and Foreground Black as `{FG-Black}` or
`{FG-Black:UL}`.

Inline Representation of Tab Control Codes
------------------------------------------

Tab Control codes are used for positioning the captions. Specifically, they tell how far to move to the right from
the current position. Used in conjunction with the Column Number from the Address PAC, it allows the captions to
be placed anywhere in the text field.

![Tab Offset](./html/assets/content-images/608TabOffset.png)

The same first byte also carries the optional character set selection codes (CEA-608-E Table 4), which are shown as
`{CS-Std}`, `{CS-Dbl}`, `{CS-Priv1}`, `{CS-Priv2}`, `{CS-GB}`, `{CS-KSC}` and `{CS-Reg1}`. They are not meant for
North American captioning, so seeing one usually means the stream was authored for another market.

