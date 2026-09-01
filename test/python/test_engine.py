#!/usr/bin/env python
# -*- coding: latin-1 -*- 

import ctypes
import os
import sys
import subprocess
import shutil
import importlib.util
from datetime import datetime

CAPTION_INSPECTOR_EXE = '../../caption-inspector'
CAPTION_INSPECTOR_LIBRARY = './libci-test.1.0.0.dylib'
OUTPUT_FILENAME = 'compiled_output.xml'

py_test_suites = ['autodetect_file', 'external_adaptor', 'output_utils']
py_test_suites_names = {'autodetect_file': 'Test Suite: Autodetect File',
                        'external_adaptor': 'Test Suite: External Adaptor',
                        'output_utils': 'Test Suite: Output Utilities'}
c_integ_test_suites = ['itest__buffer_utils', 'itest__pipeline_utils']
c_unit_test_suites = ['utest__buffer_utils_c', 'utest__cc_utils_c', 'utest__external_adaptor_c',
                      'utest__output_utils_c', 'utest__pipeline_utils_c', 'utest__types_c',
                      'utest__cc_data_output_c', 'utest__mcc_decode_c']


# Optional external tooling. pytest runs the Python test suites; xunit-viewer
# turns the XUnit XML into a browsable HTML report. Both are optional - when a
# tool is missing we say so clearly and carry on rather than failing obscurely.
HAVE_PYTEST = importlib.util.find_spec('pytest') is not None
HAVE_XUNIT_VIEWER = shutil.which('xunit-viewer') is not None
# Invoke pytest as a module so it is found whenever the package is importable,
# even if the 'pytest' console script is not on PATH.
PYTEST_CMD = '"' + sys.executable + '" -m pytest'


def run_test(test_name):
    if not HAVE_PYTEST:
        print("SKIPPED '" + test_name + "': pytest is not installed (install with: pip install pytest).")
        return
    os.system(PYTEST_CMD + ' -o junit_suite_name=' + test_name + ' --junitxml ' + test_name + '.xml test__' + test_name + '.py')
    print('Testing Complete: Results written to file - ' + test_name + '.xml')
#    os.system('xunit-viewer --results=' + test_name + '.xml --output=' + test_name + '.html --title="Caption Inspector Python Test Suite"')
#    print('XML Results file: ' + test_name + '.xml converted to HTML: ' + test_name + '.html')
#    os.system('open ' + test_name + '.html')


if __name__ == "__main__":
    if len(sys.argv) == 2 and str(sys.argv[1]) != "docker":
        run_test(str(sys.argv[1]))
    else:
        exe_ver = '???'
        lib_ver = '???'
        response = subprocess.check_output([CAPTION_INSPECTOR_EXE, '-v'], stderr=subprocess.STDOUT)
        if response.split()[0] == b"Version:":
            exe_ver = response.strip().decode('utf-8')
        clib = ctypes.CDLL(CAPTION_INSPECTOR_LIBRARY)
        clib.ExtrnlAdptrGetVersion.restype = ctypes.c_char_p
        response = clib.ExtrnlAdptrGetVersion()
        lib_ver = ctypes.c_char_p(response).value.decode('utf-8')
#        if exe_ver != lib_ver:
#            print("Version Mismatch! " + exe_ver + " vs. " + lib_ver)
#            exit(1)
        date_str = str(datetime.now().strftime('%Y_%m_%d__%H_%M_%S'))
        out_file_name = date_str + '_test_output'
        file = open(out_file_name + '.xml', "w")
        file.write("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")
        file.write("<testsuites name=\"Caption Inspector " + exe_ver + " Tests\">\n")
        file.close()
        retval = 0
        if HAVE_PYTEST:
            for test in py_test_suites:
                if os.system(PYTEST_CMD + ' -o junit_suite_name="' + py_test_suites_names[test] + '" --junitxml ' + date_str + '_' + test + '.xml test__' + test + '.py') != 0 or retval != 0:
                    retval = 1
                os.system('cat ' + date_str + '_' + test + '.xml >> ' + out_file_name + '.xml')
#                os.system('rm ' + date_str + '_' + test + '.xml')
        else:
            print("\nWARNING: pytest is not installed - skipping the Python test suites (" +
                  ", ".join(py_test_suites) + "). Install with: pip install pytest\n")
        for test in c_integ_test_suites:
            if os.system('../' + test + ' ' + date_str + '_' + test + '.xml') != 0 or retval != 0:
                retval = 1
            os.system('cat ' + date_str + '_' + test + '.xml >> ' + out_file_name + '.xml')
#            os.system('rm ' + date_str + '_' + test + '.xml')
        for test in c_unit_test_suites:
            if os.system('../' + test + ' ' + date_str + '_' + test + '.xml') != 0 or retval != 0:
                retval = 1
            os.system('cat ' + date_str + '_' + test + '.xml >> ' + out_file_name + '.xml')
#            os.system('rm ' + date_str + '_' + test + '.xml')
        os.system("echo \"</testsuites>\n\n\" >> " + out_file_name + '.xml')
        if HAVE_XUNIT_VIEWER:
            os.system('xunit-viewer --results=' + out_file_name + '.xml --output=' + out_file_name +
                      '.html --title="Caption Inspector ' + exe_ver + ' Full Test Suite"')
            print('XML Results file: ' + out_file_name + '.xml converted to HTML: ' + out_file_name + '.html')
            # 'open' is macOS-only; only pop the report for an interactive local run.
            if (len(sys.argv) == 1 or str(sys.argv[1]) != "docker") and sys.platform == 'darwin':
                os.system('open ' + out_file_name + '.html')
        else:
            print("\nNOTE: xunit-viewer is not installed - skipping the HTML report; raw results are in " +
                  out_file_name + ".xml. Install: https://github.com/lukejpreston/xunit-viewer\n")
        sys.exit(retval)

