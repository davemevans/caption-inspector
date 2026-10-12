#!/usr/bin/env python
# -*- coding: latin-1 -*- 

import ctypes
import difflib
import os

CAPTION_INSPECTOR_LIBRARY = './libci-test.1.0.0.dylib'


def compare_text(first, second, ctr, ignore):
    if ctr in ignore:
        return True
    if first == second:
        return True
    print("Different [Line-%d]: " % ctr)
    d = difflib.Differ()
    print("".join(d.compare([first], [second])))
    return False


def compare_files(generated_file, master_file, ignore):
    print('Comparing: %s vs. %s' % (generated_file.name, master_file.name))
    counter = 0
    generated_line = generated_file.readline()
    while generated_line:
        counter = counter + 1
        master_line = master_file.readline()
        identical = compare_text(generated_line, master_line, counter, ignore)
        assert identical is True
        generated_line = generated_file.readline()
    # Output that stops short of the master (e.g. lost trailing captions) is a failure too.
    master_line = master_file.readline()
    if master_line:
        print("Generated file ends at line %d, master continues: %s" % (counter, master_line))
    assert not master_line


class TestClass(object):
    def test__EA_Establish_SCC_Pipeline(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        if os.path.exists('./tmp') is not True:
            os.mkdir('tmp')
        retval = clib.ExtrnlAdptrPlumbSccPipeline('../media/Plan9fromOuterSpace.scc'.encode('utf-8'),
                                                  './tmp'.encode('utf-8'), 2997)
        assert retval is 1
        clib.ExtrnlAdptrDriveDecodePipeline()
        assert os.path.exists('./tmp/Plan9fromOuterSpace-C1.608'.encode('utf-8')) is True
        generated_file = open("./tmp/Plan9fromOuterSpace-C1.608", "r")
        master_file = open("../media/Plan9fromOuterSpace-C1.608", "r")
        compare_files(generated_file, master_file, [1])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/Plan9fromOuterSpace.mcc'.encode('utf-8')) is True
        generated_file = open("./tmp/Plan9fromOuterSpace.mcc", "r")
        master_file = open("../media/Plan9fromOuterSpace.mcc", "r")
        compare_files(generated_file, master_file, [41,43,44])
        generated_file.close()
        master_file.close()
        os.remove("./tmp/Plan9fromOuterSpace-C1.608")
        os.remove("./tmp/Plan9fromOuterSpace.mcc")
        os.remove("./tmp/Plan9fromOuterSpace.ccd")
        os.removedirs("./tmp")

    def test__EA_Establish_MCC_Pipeline(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        if os.path.exists('./tmp') is not True:
            os.mkdir('tmp')
        retval = clib.ExtrnlAdptrPlumbMccPipeline('../media/NightOfTheLivingDead.mcc'.encode('utf-8'),
                                                  './tmp'.encode('utf-8'))
        assert retval is 1
        clib.ExtrnlAdptrDriveDecodePipeline()
        assert os.path.exists('./tmp/NightOfTheLivingDead-C1.608'.encode('utf-8')) is True
        generated_file = open("./tmp/NightOfTheLivingDead-C1.608", "r")
        master_file = open("../media/NightOfTheLivingDead-C1.608", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/NightOfTheLivingDead-S1.708'.encode('utf-8')) is True
        generated_file = open("./tmp/NightOfTheLivingDead-S1.708", "r")
        master_file = open("../media/NightOfTheLivingDead-S1.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        os.remove("./tmp/NightOfTheLivingDead-C1.608")
        os.remove("./tmp/NightOfTheLivingDead-S1.708")
        os.remove("./tmp/NightOfTheLivingDead.ccd")
        os.removedirs("./tmp")

    def test__EA_Establish_HEVC_Pipeline(self):
        # BigBuckBunny_256x144-24fps-hevc.ts is the H.264 BigBuckBunny asset re-encoded to HEVC with its
        # captions carried as A/53 SEI, so it must decode to the same 608/708 output as the H.264 masters.
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        if os.path.exists('./tmp') is not True:
            os.mkdir('tmp')
        retval = clib.ExtrnlAdptrPlumbMpegPipeline('../media/BigBuckBunny_256x144-24fps-hevc.ts'.encode('utf-8'),
                                                   './tmp'.encode('utf-8'), 1)
        assert retval is 1
        clib.ExtrnlAdptrDriveDecodePipeline()
        for suffix in ['C1.608', 'C3.608', 'S1.708', 'S2.708', 'S3.708', 'S4.708', 'S5.708', 'S6.708']:
            generated_name = './tmp/BigBuckBunny_256x144-24fps-hevc-' + suffix
            assert os.path.exists(generated_name.encode('utf-8')) is True
            generated_file = open(generated_name, "r")
            master_file = open('../media/BigBuckBunny_256x144-24fps-' + suffix, "r")
            compare_files(generated_file, master_file, [1])  # line 1 names the asset
            generated_file.close()
            master_file.close()
        for leftover in os.listdir('./tmp'):
            if leftover.startswith('BigBuckBunny_256x144-24fps-hevc'):
                os.remove('./tmp/' + leftover)

    def test__EA_Establish_MPG_Pipeline_w_Artifacts(self):
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        if os.path.exists('./tmp') is not True:
            os.mkdir('tmp')
        retval = clib.ExtrnlAdptrPlumbMpegPipeline('../media/BigBuckBunny_256x144-24fps.ts'.encode('utf-8'),
                                                   './tmp'.encode('utf-8'), 1)
        assert retval is 1
        clib.ExtrnlAdptrDriveDecodePipeline()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-C1.608'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-C1.608", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-C1.608", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-C3.608'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-C3.608", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-C3.608", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-S1.708'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-S1.708", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-S1.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-S2.708'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-S2.708", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-S2.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-S3.708'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-S3.708", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-S3.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-S4.708'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-S4.708", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-S4.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-S5.708'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-S5.708", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-S5.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps-S6.708'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps-S6.708", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps-S6.708", "r")
        compare_files(generated_file, master_file, [])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps.ccd'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps.ccd", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps.ccd", "r")
        compare_files(generated_file, master_file, [35, 36])
        generated_file.close()
        master_file.close()
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps.inf'.encode('utf-8')) is True
        assert os.path.exists('./tmp/BigBuckBunny_256x144-24fps.mcc'.encode('utf-8')) is True
        generated_file = open("./tmp/BigBuckBunny_256x144-24fps.mcc", "r")
        master_file = open("../media/BigBuckBunny_256x144-24fps.mcc", "r")
        compare_files(generated_file, master_file, [41, 43, 44])
        generated_file.close()
        master_file.close()
        os.remove("./tmp/BigBuckBunny_256x144-24fps-C1.608")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-C3.608")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-S1.708")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-S2.708")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-S3.708")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-S4.708")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-S5.708")
        os.remove("./tmp/BigBuckBunny_256x144-24fps-S6.708")
        os.remove("./tmp/BigBuckBunny_256x144-24fps.ccd")
        os.remove("./tmp/BigBuckBunny_256x144-24fps.inf")
        os.remove("./tmp/BigBuckBunny_256x144-24fps.mcc")
        os.removedirs("./tmp")


if __name__ == "__main__":
    TestClass().test__EA_Establish_SCC_Pipeline()
    TestClass().test__EA_Establish_MCC_Pipeline()
    TestClass().test__EA_Establish_MPG_Pipeline_w_Artifacts()
