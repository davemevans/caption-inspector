CEA-708 Decode File (.708)
==========================

Caption Inspector will output decoded CEA-708 as text with inline tags from the 708 [spec](https://en.wikipedia.org/wiki/CEA-708).
Each Service of 708 captions that are found are written in their own file, regardless of whether or not there are full
and valid captions associated with the specific service.

Here is an example from a Decoded CEA-708 file that was produced by Caption Inspector (service 1 of
`test/media/BigBuckBunny_256x144-24fps.ts`), followed by a description of the different elements in the file.

```
00:00:03,545 - {HDW:11111111}
00:00:03,753 - {TGW:00000010}
00:00:03,920 - {DLW:00000001} {DF0:PopUp-Cntrd:R1-C41:Anchor-UL-V65-H60:Pen-MonoSerif:Pr-0} {SWA:Fill-Transparent-R1G1B1:Brdr-None-R1G1B1:PD-LtoR:SD-LtoR:JD-LtoR:Snap-1.0sec-LtoR} {SPL:R0-C6} {SPA:Pen-[Size:Standard,Offset:Normal]:TextTag-Dialog:FontTag-Default:EdgeType-None} {SPC:FG-Solid-R2G2B2:BG-Solid-R0G0B0:Edg-R1G1B1} "I WIN," {SPL:R1-C0} "WE MOVE IN THERE."
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
the text, they will be printed out directly and will not be escaped out or anything like that. The file is UTF-8.

Characters from all four code sets are printed inside the quotes with the rest of the text: G0 (ASCII, with the music
note `♩` at 0x7F), G1 (Latin-1, e.g. `É`), G2 (e.g. `…`, `™`, `Œ` and box-drawing characters, with the transparent space
shown as a space) and G3 (the `[CC]` icon, shown as `㏄`).

The C0 codes NUL, ETX, BS, FF, CR and HCR are shown inline by name, e.g. `{CR}`, although NUL and ETX can be left out
within text. A 16-bit character sent with P16 is shown as its code, e.g. `{P16:0x0634}`, because the stream does not
say which character set it is in. The reserved C2 and C3 codes are shown as `{C2Cmd}` and `{C3Cmd}`.

Set Current Window
------------------

Set Current Window specifies the window to which all subsequent window style and pen commands are directed.

![Set Current Window](./html/assets/content-images/708SetCurrentWindow.png)


Define Window
-------------

Define Window creates a window and initializes the window with the parameters listed in the command.

![Define Windows](./html/assets/content-images/708DefineWindow.png)

Clear Windows
--------------

Clear Windows removes any existing text from the specified windows in the window map and replaces the entire window
with the window fill color.

![Clear Windows](./html/assets/content-images/708ClearWindows.png)

Delete Windows
--------------

Delete Windows removes all specified windows.

![Delete Windows](./html/assets/content-images/708DeleteWindows.png)

Display Windows
---------------

Display Windows causes the specified windows to be visible.

![Display Windows](./html/assets/content-images/708DisplayWindows.png)

Hide Windows
------------

Hide Windows causes the specified windows to be removed from the screen.

![Hide Windows](./html/assets/content-images/708HideWindows.png)

Toggle Windows
--------------

Toggle Windows causes all specified windows to toggle whether they are displayed or hidden.

![Toggle Windows](./html/assets/content-images/708ToggleWindows.png)

Set Window Attributes
---------------------

Set Window Attributes assigns the specified style attributes to the current window.

![Set Window Attributes](./html/assets/content-images/708SetWindowAttributes.png)

Set Pen Attributes
------------------

Set Pen Attributes assigns pen style attributes for the currently defined window. Text written to the current window
will have the attributes specified by the most recent Set Pen Attributes command written to the window.

![Set Pen Attributes](./html/assets/content-images/708SetPenAttributes.png)

Set Pen Color
-------------

Set Pen Color assigns the pen color attributes for the current window. Text written to that window will have the color
attributes specified by the most recent Set Pen Color command.

![Set Pen Color](./html/assets/content-images/708SetPenColor.png)

Set Pen Location
----------------

Set Pen Location repositions the pen cursor for the current window, as specified by the current window ID. When the
window justification type is 'left' the next group of text written to the current window will start at the specified
row and column, and justification will be ignored. When the window justification type is not left and the print
direction is left-to-right or right-to-left, the column parameter shall be ignored. When the window justification type
is not left and the print direction is top-to-bottom or bottom-to-top, the row parameter shall be ignored. When the
window justification type is not left, text shall be formatted based upon the current window justification type.

![Set Pen Location](./html/assets/content-images/708SetPenLocation.png)

Delay
-----

Delay suspends interpretation of the command input buffer. Once the delay expires captioning resumes.

![Delay](./html/assets/content-images/708Delay.png)

Delay Cancel
------------

Delay Cancel terminates an active Delay command and captioning resumes.

![Delay Cancel](./html/assets/content-images/708DelayCancel.png)

Reset
-----

Reset command reinitializes the service for which it is received.

![Reset](./html/assets/content-images/708Reset.png)

Reserved
--------

Reserved Codes are just that, Reserved Codes. They are not believed to be used.

![Reserved](./html/assets/content-images/708Reserved.png)
