Changelog
=========

Changes since the last Comcast release of Caption Inspector (`master`, November 2022).

Unreleased
----------

### Output format changes

Several fixes change what the output files contain, so files written by this version will differ from files written
by the Comcast release for the same asset. Compare old and new output with that in mind.

#### All files

- Timestamps start at zero at the start of the stream. MP4 captions were 333 ms early, and the first had a garbage
  timestamp.
- A timestamp discontinuity in a transport stream (a splice, an encoder restart or a looped playout) no longer moves
  the captions after it by about 26 hours. The timeline carries on from the previous frame, with a warning in the debug
  file. Hours count on past 23 instead of wrapping.
- Drop-frame timestamps follow SMPTE counting (2 frames a minute at 29.97, 4 at 59.94), and drop frame is only applied
  at 29.97 and 59.94 fps.

#### `.608`

- Special and Extended Characters are decoded. They were read as PACs, so for example the transparent space showed as a
  phantom `{R2:C16:UL}`.
- Mid-row codes show underline on foreground codes and partial transparency on background codes; the two were swapped.
- Extended Data Services (XDS) packets on field 2 are no longer decoded as captions.
- Field 2 control codes sent with the field 1 first byte (`0x14`/`0x1C`) are decoded as CC3/CC4.
- Text Mode is written to its own files, `-T1.608` to `-T4.608`.
- Character set selection codes are named, e.g. `{CS-Std}`.
- A Special or Extended Character sent twice in a row is shown once.
- The last caption's quote is closed.

#### `.708`

- G2 and G3 characters (reached through EXT1) are decoded.
- G1 is Latin-1: `¢`, `£`, `¥`, `¬` and `¯` were their fullwidth forms, and `ö` to `ú` (0xF6-0xFA) were shifted by one.
- Bytes left over from an earlier packet are no longer decoded when a packet is cut short, and 128-byte packets are
  decoded in full.
- Variable-length C3 commands are skipped by their length, so the text after them decodes.
- The last caption's quote is closed.

#### `.ccd`

All of the `.608` and `.708` changes above, and:

- CEA-708 Packet Start and Service Block headers are shown as `<--1|B:20` / `<--1P024|B:20,S06` (sequence, packet size,
  block size, service), and each later Service Block header as `B:nn` / `B:nn,Snn`. They were `<-Srvc:01` /
  `<--Seq:1 P024-B20`.
- A C0 NUL is shown as `NUL` / `C0: NUL`, not `?00?` / `???-0x00`.
- G1, G2 and G3 characters are shown as `G1:É` / `G1:  "É"`, and skipped C2/C3 codes as `C2:1`, `C3:1`, `C2:+`, `C3:+`
  and `C3:L`.
- XDS bytes are shown as `F2 - XDS`, character set selection codes as `Ch1 - CHS`, and Text Mode text on the `TEXT:`
  line as `T1` to `T4`.
- The channel shown for a CEA-608 code follows the channel the code applies to.
- The columns line up in screen cells, so accented, symbol and East Asian wide characters no longer push the rest of
  the line out of line.
- The last line of a frame is written at frame rates whose `cc_count` is not a multiple of five (e.g. 23.976).

#### `.mcc`

- The CDP sequence counter is 16 bits.
- 29.97 and 59.94 fps are always written as `30DF` / `60DF`, and files that use `60DF` are written as MCC 2.0.
