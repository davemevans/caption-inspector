#!/usr/bin/env python
# -*- coding: latin-1 -*- 

import ctypes
import os
import pytest

CAPTION_INSPECTOR_LIBRARY = './libci-test.1.0.0.dylib'

UNK_CAPTIONS_FILE = 0
SCC_CAPTIONS_FILE = 1
MCC_CAPTIONS_FILE = 2
MPEG_BINARY_FILE = 3
MOV_BINARY_FILE = 4

# MOV/MP4 file-type detection is only compiled in when the library is built with
# GPAC. Probe the library so the MOV test skips cleanly on a build without it
# rather than failing (a .mov is reported as an MPEG file when GPAC is absent).
MOV_SUPPORTED = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY).ExtrnlAdptrIsMovSupported() != 0


class TestClass(object):
    def test__Determine_SCC_File(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        file_type = clib.DetermineFileType("../media/Plan9fromOuterSpace.scc".encode('utf-8'))
        assert file_type is SCC_CAPTIONS_FILE

    def test__Determine_MCC_File(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        file_type = clib.DetermineFileType("../media/NightOfTheLivingDead.mcc".encode('utf-8'))
        assert file_type is MCC_CAPTIONS_FILE

    def test__Determine_MPG_File(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        file_type = clib.DetermineFileType("../media/BigBuckBunny_160x90-24fps.mpg".encode('utf-8'))
        assert file_type is MPEG_BINARY_FILE

    def test__Determine_TS_File(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        file_type = clib.DetermineFileType("../media/BigBuckBunny_256x144-24fps.ts".encode('utf-8'))
        assert file_type is MPEG_BINARY_FILE

    @pytest.mark.skipif(not MOV_SUPPORTED, reason="MOV support requires GPAC (COMPILE_GPAC); library built without it")
    def test__Determine_MOV_File(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        file_type = clib.DetermineFileType("../media/BigBuckBunny_160x90-24fps.mov".encode('utf-8'))
        assert file_type is MOV_BINARY_FILE

    def test__Determine_UNK_File(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        file_type = clib.DetermineFileType("../media/LoremIpsum.txt".encode('utf-8'))
        assert file_type is UNK_CAPTIONS_FILE

    def test__Determine_DF_NDF(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        is_drop_frame = clib.DetermineDropFrame("../media/BigBuckBunny_160x90-24fps.mpg".encode('utf-8'), 0, 0)
        assert is_drop_frame == 0

    def test__Determine_DF_IDF(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        is_drop_frame = clib.DetermineDropFrame("../media/BigBuckBunny_160x90-24fps.mov".encode('utf-8'), 0, 0)
        assert is_drop_frame == 0
        is_drop_frame = clib.DetermineDropFrame("../media/BigBuckBunny_256x144-24fps.ts".encode('utf-8'), 0, 0)
        assert is_drop_frame == 1

    def test__Determine_DF_Report(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        if os.path.exists("./tmp") is not True:
            os.mkdir("tmp")
        is_drop_frame = clib.DetermineDropFrame(f"../media/BigBuckBunny_160x90-24fps.mpg".encode('utf-8'), 1,
                                             './tmp'.encode('utf-8'))
        assert os.path.isfile(f"./tmp/BigBuckBunny_160x90-24fps.inf") is True
        assert is_drop_frame == 0
        if os.path.isfile(f"./tmp/BigBuckBunny_160x90-24fps.inf"):
            os.remove(f"./tmp/BigBuckBunny_160x90-24fps.inf")
            assert os.path.isfile(f"./tmp/BigBuckBunny_160x90-24fps.inf") is False
        os.removedirs("./tmp")

    def test__Determine_DF_Report_NULL(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        is_drop_frame = clib.DetermineDropFrame("../media/BigBuckBunny_160x90-24fps.mpg".encode('utf-8'), 1, 0)
        assert os.path.isfile("../media/BigBuckBunny_160x90-24fps.inf") is True
        assert is_drop_frame == 0
        if os.path.isfile("../media/BigBuckBunny_160x90-24fps.inf"):
            os.remove("../media/BigBuckBunny_160x90-24fps.inf")
            assert os.path.isfile("../media/BigBuckBunny_160x90-24fps.inf") is False


if __name__ == "__main__":
    TestClass().test__Determine_SCC_File()
    TestClass().test__Determine_MCC_File()
    TestClass().test__Determine_MPG_File()
    TestClass().test__Determine_TS_File()
    TestClass().test__Determine_MOV_File()
    TestClass().test__Determine_UNK_File()
    TestClass().test__Determine_DF_NDF()
    TestClass().test__Determine_DF_IDF()
    TestClass().test__Determine_DF_Report()
    TestClass().test__Determine_DF_Report_NULL()
