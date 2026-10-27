Closed Caption Descriptor Decode File (.ccd)
============================================

Caption Inspector will output a single Closed Caption Descriptor File for each asset that it processes. The Closed
Caption Descriptor Files are intended to be annotated hex dumps of the closed captioning descriptors located on
every frame of video or every line of an MCC file. Unlike the other decode artifacts, the CCD files are decoded inline,
all channels and services left multiplexed together, and have traceability back to the original/unmodified hex that
was extracted from the asset or caption file.

Here is an example of one frame from a Closed Caption Descriptor File that was produced by Caption Inspector (from
`test/media/BigBuckBunny_256x144-24fps.ts`), followed by a description of the different elements in the file.

```
00:00:00,542  F1:AE80  F2:D34F  PS:C76A  PD:C954  PD:4952    Ch1 - "."  Ch3: "SO"  <--3|B:10  G1:É|G0:T  G0:I|G0:R    Channel - 1:  "."  Chan-3:  "S"  "O"  <--3P014|B:10,S03  G1:  "É"|G0Svc:03  G0Svc:03|G0Svc:03
              PD:454D  PD:454E  PD:542E  PD:0000  XD:0000    G0:E|G0:M  G0:E|G0:N  G0:T|G0:.  B:00|B:00  _________    G0Svc:03|G0Svc:03  G0Svc:03|G0Svc:03  G0Svc:03|G0Svc:03  B:NULL  |B:NULL    _________________
              XD:0000  XD:0000  XD:0000  XD:0000  XD:0000    _________  _________  _________  _________  _________    _________________  _________________  _________________  _________________  _________________
              XD:0000  XD:0000  XD:0000  XD:0000  XD:0000    _________  _________  _________  _________  _________    _________________  _________________  _________________  _________________  _________________
              XD:0000  XD:0000  XD:0000  XD:0000  XD:0000    _________  _________  _________  _________  _________    _________________  _________________  _________________  _________________  _________________
TEXT: Ch1 - "." Ch3 - "SO" Svc3 - "ÉTIREMENT."
```

File Structure
--------------

The file starts with a header that describes the format, followed by the creation date and time, the frame rate and
whether the timestamps are drop frame.

The Closed Caption Descriptor File contains a decoded binary representation of Closed Captioning Data. Each grouping of
text immediately following a timestamp represents the Closed Captioning Data - `cc_data()` that would be associated with
a frame of video, or a line from an MacCaption (MCC) file. Each "line" contains the same groups of elements: Timestamp,
Hex Data, Decoded Data, Supplemental Decoded Data, and Text (if it exists in the line). These element can describe both
CEA-608 and CEA-708 data contained in the Closed Captioning Descriptors.

```
00:00:00,083  F1:8080  F2:8080  F1:8080  PS:4CD4  PD:8C01    F1 - NULL  F2 - NULL  F1 - NULL  <--1|B:20  DLW |BITM    608: Field 1 NULL  608: Field 2 NULL  608: Field 1 NULL  <--1P024|B:20,S06  C1:{DLW}|00000001
              PD:9800  PD:4137  PD:0129  PD:1197  PD:D515    DF0 |DFW1  DFW2|DFW3  DFW4|DFW5  DFW6|SWA   SWA1|SWA2    C1:{DF0}|P0 F F F  VA-065 F|HrzA-055  RN-01 UL|ColN-041  WS2  PS1|C1:{SWA}  3  1:1:1|0  1:1:1
\__________/  \__________________________________________/    \___________________________________________________/    \___________________________________________________________________________________________/
  Timestamp                     Hex Data                                           Decoded Data                                                         Supplemental Decoded Data
```

Each `cc_data()` construct is one element, and each element takes the same position in the Hex Data, Decoded Data and
Supplemental Decoded Data columns, five to a row. The number of elements in a line is set by the frame rate, so a line
can take several rows.

A CEA-708 element carries two bytes, which are usually parts of different codes, so it is decoded as two halves joined
by a `|`, one per byte: `DF0 |DFW1` is the last byte of a Define Window command and the first of its parameters. A CEA-608
element is one byte pair and is decoded as a whole.

The columns are padded to a fixed width in screen cells, not bytes, so they line up in a monospaced view with a UTF-8
font. East Asian wide characters (CJK, kana, Hangul and fullwidth forms) take two cells, and every other character takes
one. All output files are UTF-8.

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

Timestamps start at zero at the start of the asset. If the timestamps in a transport stream jump (a splice, an encoder
restart or a looped playout), the timeline carries on from the previous frame and a warning is written to the debug
file, so the timestamps stay elapsed playback time. Hours count on past 23.

Aggregated Text
---------------

If there is text detected in the line, it will be pulled together and gets labeled with the Channel or Service Number
that it is associated with. This is done to easily be able to see what the text spells out.

```
TEXT: Ch1 - "." Ch3 - "SO" Svc3 - "ÉTIREMENT."
```

CEA-608 caption channels are labeled `Ch1` to `Ch4`, and CEA-608 Text Mode services `T1` to `T4`: after a Text Restart
`{TR}` or Resume Text Display `{RTD}`, the channel's text is labeled as its Text Mode service until a caption code returns
it to captions. CEA-708 services are labeled `Svc1` to `Svc63`. A CEA-608 Special or Extended Character sent twice in a
row is shown once, as a decoder shows it.

Errors
------

When a line contains a code that could not be decoded, an `ERRORS:` line follows it, listing a number for each one. The
same number is in the warning written to the debug file, which says what was wrong.

Hex Data
--------

![Hex Data](html/assets/content-images/ccdHexData.png)

The Hex Data representation of the Closed Captioning Construct is intended to be the most direct representation of the
`cc_data()` structure, which is comprised of the following fields:
 
`{cc_valid}{cc_type}:{cc_data_1}{cc_data_2}`

The `cc_valid` field determines whether the subsequent fields should considered valid and be interpreted. This field
will either be 'F' for Valid Line 21, 'P' for Valid DTVCC, or 'X' for Invalid. The `cc_type` field determines the type
of data that will be in cc_data_1 and cc_data_2. This field can have the following values: '1' Line 21 Field 1 Data, '2'
Line 21 Field 2 Data, 'S' DTVCC Packet Data Start, 'D' DTVCC Packet Data. The fields `cc_data_1` and `cc_data_2` are the
actual two bytes of payload data, which are represented in hexadecimal.


Decoded Data and Supplemental Decoded Data
------------------------------------------

Decoded Data and Supplemental Decoded Data are intended to be the inline decode of the corresponding Hex Data using
the 608 and 708 Specs. The Decoded Data is a high level decode of the code referenced by the hex value, and the
supplemental decoded data describes the flags and values referenced by the hex value. Supplemental decoded data plays
a larger role in 708 because more information is crammed into bit fields in the magic numbers.

### CEA-608 NULL Data

![608 NULL Data](html/assets/content-images/ccd608NULLData.png)

CEA-608 tends to have its fill data be NULL Data. NULL Data is just `0x00` with a parity bit in the msb position. Since
this is placed in specific to 608, it gets decoded to NULL (as opposed to 708 NULL Data, which is shown as a blank line).

### CEA-608 Characters

A pair of characters is shown in the channel it was sent on, and each character in the Supplemental Decoded Data. A
pair padded with a NULL is shown as one character.

```
F1:C8E5    Ch1: "He"    Chan-1:  "H"  "e"
F1:EF80    Ch1 - "o"    Channel - 1:  "o"
```

Special Characters and Extended Characters are shown with their channel. For an Extended Character, the Supplemental
Decoded Data names its character set.

```
F1:9137    Ch1  "♪"     _________________
F1:9220    Ch1  "Á"     _Spanish or Misc_
```

### CEA-608 Global Control Codes

![608 Control Codes](html/assets/content-images/ccd608GlobalCtrlCodes.png)

There is not much decoding to do around Global Control Codes in CEA-608. Basically it is just the Channel Number that
gets decoded and added to the Decoded Data. The Supplemental Decoded Data field is basically used to try to spell out
the control code as much as space permits, rather than using the code that is found in the Decoded Data.

The channel shown is the one the code applies to. Control codes, PACs, Mid Row codes and Special and Extended
Characters each name their channel, and the characters that follow are shown in that channel. On field 2 a control code
sent with the field 1 first byte (`0x14`/`0x1C`) is shown as CC3/CC4, since the field decides the channel.

### CEA-608 Preamble Access Codes (PAC)

![608 PAC](html/assets/content-images/ccd608PAC.png)

PACs come in two types, Address PACs which describe where the cursor should be located, and Style PACs which describe
what style/color should be used when writing to the screen. The Decoded Data field denotes that the decoded code is a
PAC, and which channel it is associated with. The Supplemental Decoded Data field spells out the Address and Style
information of the PAC.

### CEA-608 Mid Row Style Change Code

Midrow Style Changes come in two types, Foreground and Background Style Change. Additionally the Foreground MidRow Style
Change can indicate that the text is underlined and the Background MidRow Style Change can indicate that the background
is semi-transparent. The Decoded Data gives the channel, and the Supplemental Decoded Data the color and flag.

```
F1:9123    Ch1 - MDR    FG:Green - Undrln
F1:91AE    Ch1 - MDR    FG:Italics  White
F1:10AD    Ch1 - MDR    BG:Mgnta - SemiTr
F1:97AD    Ch1 - MDR    _BG:Transparent__
F1:97AE    Ch1 - MDR    _____FG:Black____
```

The foreground colors are White, Green, Blue, Cyan, Red, Yellow, Magenta and Italics White. The background colors are
White, Green, Blue, Cyan, Red, Yellow, Magenta and Black. The Background Transparent and Foreground Black codes share
their first byte with the Tab Offset codes.

### CEA-608 Tab Control Code

![608 Tab](html/assets/content-images/ccd608Tab.png)

Tab Controls, in combination with an Address PAC can point the cursor to anywhere inside of the text window.

### CEA-608 Character Set Selection

The same first byte also carries the optional character set selection codes (CEA-608-E Table 4). They are not meant for
North American captioning, so seeing one usually means the stream was authored for another market.

```
F1:97A4    Ch1 - CHS    CharSet: Std Norm
```

### CEA-608 Extended Data Services (XDS)

XDS packets are sent on field 2 between captions. Their bytes are shown as XDS rather than decoded as captions: the start
of a packet gives its class and type, then come its data bytes, then the end of the packet with its checksum.

```
F2:0183    F2 - XDS     XDS Current Start
F2:C1C2    F2 - XDS     XDS Data   41 42
F2:8FEA    F2 - XDS     XDS End/Checksum
```

### CEA-708 Packet Structure

CEA-708 is able to support 64 services, rather than the 4 channels in 608, in part by defining a variable length packet
structure and then associating each group of packets to a specific service. The packet structure is made up of
a Packet Start followed by a number of Data Packets. Receiving another Packet Start ends the current packet and tells
the closed captioning service to process all of the data that was in the previous packet structure. To make the start
of the packet structure stick out in the decoded data, it has an arrow on the Packet Start: `<--`.

```
PS:4CD4    <--1|B:20    <--1P024|B:20,S06
```

The first byte is the packet header: `<--1` is sequence number 1, and `P024` is a packet size of 24 bytes. The second
byte is the header of the packet's first Service Block: `B:20` is a block size of 20 bytes, and `S06` is service 6.

Each Service Block in the packet starts with a header like this one. A block of size 0 is a null block, which pads the
rest of the packet. A service number of 7 or more is sent in an extra byte, and shown as `SEx` followed by that byte.

```
PD:0000    B:00|B:00    B:NULL  |B:NULL
PS:0CE7    <--0|B:07    <--0P024|B:07,SEx
PD:1041    S:16|G0:A    ExSvc:16|G0Svc:16
```

A byte outside any packet (before the first Packet Start, or past the end of a packet) is shown as `____` / `________`.

### CEA-708 Characters

Characters are shown with their code set in the Decoded Data. The Supplemental Decoded Data shows the service of a G0
character, and the character itself for the other sets.

```
PD:C954    G1:É|G0:T    G1:  "É"|G0Svc:03
PD:1025    EXT1|G2:…    Extended|G2:  '…'
PD:10A0    EXT1|G3㏄    Extended|G3: '㏄'
```

G0 is ASCII, apart from 0x7F, which is the music note `♩`. G1 is Latin-1. G2 and G3 are reached through the EXT1 code
(0x10): G2 holds symbols such as `…`, `™`, `Œ` and box-drawing characters, and G3 holds the `[CC]` icon. A G2 or G3 code
with no character assigned is shown as `G2:?` / `G2:?-22?`.

### CEA-708 C0 Codes

The C0 codes NUL, ETX, BS, FF, CR and HCR are shown by name. NUL is used as padding.

```
PD:0500    SPL2|NUL     Col-05 |C0: NUL
PD:0300    ETX |B:00    C0: ETX |B:NULL
```

P16 (0x18) is followed by a 16-bit character code in two bytes. The character set of P16 is not signalled in the stream,
so the two bytes are shown as hex, as `P16:06:1` and `P16:34:2`, and the command byte as `P16?` / `P16:UNH?`.

Any other C0 code is unknown, and is shown as `C0:?` with its value and a suffix, followed by the bytes it takes:
`C0:01???` takes no bytes; `C0:11?E1` takes one, shown as `C0:xx?E2`; and `C0:19?X1` takes two, shown as `C0:xx?X2`
and `C0:xx?X3`.

### CEA-708 Extended Codes

The C2 and C3 code sets, reached through EXT1, are reserved for future use. Caption Inspector skips them by their
length, so the codes after them decode correctly, and shows them as `C2:1` / `C3:1` for the command byte and `C2:+` /
`C3:+` for each byte after it. The C3 codes 0x90-0x9F are followed by a header byte whose low five bits give the number
of data bytes, shown as `C3:L`.

```
PD:1090    EXT1|C3:1    Extended|C3:1-90?
PD:C241    C3:L|C3:+    C3:L-C2?|C3:+-41?
```

### CEA-708 Set Current Window

Set Current Window specifies the window to which all subsequent window style and pen commands are directed.

![708 Set Current Window](html/assets/content-images/ccd708SetCurrentWindow.png)


Define Window
-------------

Define Window creates a window and initializes the window with the parameters listed in the command.

![708 Define Windows](html/assets/content-images/ccd708DefineWindow.png)

Clear Windows
--------------

Clear Windows removes any existing text from the specified windows in the window map and replaces the entire window
with the window fill color.

![708 Clear Windows](html/assets/content-images/ccd708ClearWindow.png)

Delete Windows
--------------

Delete Windows removes all specified windows.

![708 Delete Windows](html/assets/content-images/ccd708DeleteWindow.png)

Display Windows
---------------

Display Windows causes the specified windows to be visible.

![708 Display Windows](html/assets/content-images/ccd708DisplayWindow.png)

Hide Windows
------------

Hide Windows causes the specified windows to be removed from the screen.

![708 Hide Windows](html/assets/content-images/ccd708HideWindow.png)

Toggle Windows
--------------

Toggle Windows causes all specified windows to toggle whether they are displayed or hidden.

![708 Toggle Windows](html/assets/content-images/ccd708ToggleWindow.png)

Set Window Attributes
---------------------

Set Window Attributes assigns the specified style attributes to the current window.

![708 Set Window Attributes](html/assets/content-images/ccd708SetWindowAttributes.png)

NOTE: You need to multiply the high bit (value above the 'H') by 4 and add it to the low bits (value above the 'L') to
get the final code for the Border Type.

Set Pen Attributes
------------------

Set Pen Attributes assigns pen style attributes for the currently defined window. Text written to the current window
will have the attributes specified by the most recent Set Pen Attributes command written to the window.

![708 Set Pen Attributes](html/assets/content-images/ccd708SetPenAttributes.png)

Set Pen Color
-------------

Set Pen Color assigns the pen color attributes for the current window. Text written to that window will have the color
attributes specified by the most recent Set Pen Color command.

![708 Set Pen Color](html/assets/content-images/ccd708SetPenColor.png)

Set Pen Location
----------------

Set Pen Location repositions the pen cursor for the current window, as specified by the current window ID. When the
window justification type is 'left' the next group of text written to the current window will start at the specified
row and column, and justification will be ignored. When the window justification type is not left and the print
direction is left-to-right or right-to-left, the column parameter shall be ignored. When the window justification type
is not left and the print direction is top-to-bottom or bottom-to-top, the row parameter shall be ignored. When the
window justification type is not left, text shall be formatted based upon the current window justification type.

![708 Set Pen Location](html/assets/content-images/ccd708SetPenLocation.png)

Delay
-----

Delay suspends interpretation of the command input buffer. Once the delay expires captioning resumes.

![708 Delay](html/assets/content-images/ccd708Delay.png)

Delay Cancel
------------

Delay Cancel terminates an active Delay command and captioning resumes.

![708 Delay Cancel](html/assets/content-images/ccd708DelayCancel.png)

Reset
-----

Reset command reinitializes the service for which it is received.

![708 Reset](html/assets/content-images/ccd708Reset.png)

Reserved
--------

Reserved Codes are just that, Reserved Codes. They are not believed to be used.

![708 Reserved](html/assets/content-images/ccd708Reserved.png)
