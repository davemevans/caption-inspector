[![Build Status](https://travis-ci.com/Comcast/caption-inspector.svg?branch=master)](https://travis-ci.com/Comcast/caption-inspector)

Caption Inspector
=================

The Caption Inspector project builds a C library, C executable, and Docker image that can be used to extract and decode
Closed-Captions from various Video or Caption File Formats. Caption Inspector Supports CEA-608 and CEA-708 in MPEG-2 and
MPEG-4 (.mpg, .ts, and .mp4 containers), MCC (MacCaption Closed Captions), and SCC (Scenarist Closed Captions) files.

![Overview](./docs/html/assets/content-images/Overview.png)

Caption Inspector has a plugin pipeline architecture that can be configured in various ways and allows the user to add
new plugins to perform various transformations. Currently, the following are the main use cases that the Caption
Inspector Software covers:
* Pulling Captions from a Video Asset and writing them to an MCC Caption File, a [CEA-608 Decode File](./docs/decoded608.md),
a [CEA-708 Decode File](./docs/decoded708.md), and a [Closed Caption Descriptor Decode File](./docs/decodeCCD.md).
* Decoding a MCC Caption file and writing the decoded captions into  a [CEA-608 Decode File](./docs/decoded608.md),
a [CEA-708 Decode File](./docs/decoded708.md), and a [Closed Caption Descriptor Decode File](./docs/decodeCCD.md).
* Decoding a SCC Caption file and writing the decoded captions into a MCC Caption File, a [CEA-608 Decode File](./docs/decoded608.md),
and a [Closed Caption Descriptor Decode File](./docs/decodeCCD.md).

Building and Running the Caption Inspector Executable Locally
-------------------------------------------------------------

Caption Inspector requires the FFmpeg libraries (libavformat, libavcodec and libavutil), which the build locates
with `pkg-config`. FFmpeg 4.4 or later works, including for A/53 captions in HEVC video: the caption decoding tests
pass with FFmpeg 4.4, 5.1, 6.1, 7.1 and 8.1. The Docker image builds a pinned FFmpeg 8.1.2 from source, so that its
test results do not depend on a distribution's FFmpeg version.

On macOS, using [Homebrew](https://brew.sh):
```
brew install ffmpeg
```

On Debian or Ubuntu:
```
sudo apt install clang gcc make pkg-config libavformat-dev libavcodec-dev libavutil-dev uuid-dev zlib1g-dev
```

If FFmpeg is installed where `pkg-config` does not look (for example a source build in a custom prefix), point
`PKG_CONFIG_PATH` at its `lib/pkgconfig` directory. The Dockerfile shows how to build FFmpeg 8.1.2 from source.

Caption Inspector leverages a tool called MediaInfo to determine whether or not an asset is, or is not,
Drop Frame. Install the command line version with `brew install media-info` or `sudo apt install mediainfo`,
or download it from [here](https://mediaarea.net/en/MediaInfo/Download). Without it, video assets are treated
as non-Drop Frame unless set with `-d`. Drop Frame detection reads MediaInfo's "Time code of first frame", which
older versions do not always report (MediaInfo 24.01 and 25.04 report it for the H.264 test asset, 23.04 does
not); use a recent version, or set it with `-d`. The test suite needs it: the video test masters were generated
with Drop Frame detected, so the tests fail without MediaInfo or with a version that does not report it.

```
make caption-inspector
./caption-inspector -h
./caption-inspector -o . test/media/BigBuckBunny_256x144-24fps.ts
./caption-inspector -o . test/media/Plan9fromOuterSpace.scc -f 2400
./caption-inspector -o . test/media/NightOfTheLivingDead.mcc
```

Running with the `-h` option will simply print out the help text. 

Running against the video file `BigBuckBunny_256x144-24fps.ts` demonstrates decoding closed captions from a video file.

Running against the caption file `Plan9fromOuterSpace.scc` demonstrates decoding an SCC file and CEA-608 Captions, as well as
converting the contents of the SCC file into an MCC file.

Running against the caption file `NightOfTheLivingDead.mcc` demonstrates decoding an MCC file, CEA-608 Captions, and CEA-708 Captions.

Building the Caption Inspector Executable Locally with MOV Support
------------------------------------------------------------------

Caption Inspector requires the inclusion of the [GPAC][https://gpac.wp.imt.fr/] Library. Specifically a modified library of GPAC which
gets linked as a shared object is required. This library is located [here][https://github.com/Comcast/gpac-caption-extractor] and must be pulled and built.
Once the GPAC library has been built the same instructions are used as above.

```
git clone https://github.com/Comcast/gpac-caption-extractor.git
cd gpac-caption-extractor
make install
cd ../caption-inspector
make ci_with_gpac
```

Building and Running Caption Inspector in a Docker Container
------------------------------------------------------------
Obviously you need docker running on your local machine to build. Building inside of a docker image will remove the need
to install any dependencies, but comes at the expense of a slightly more complicated command line execution. Another advantage
of building Caption Inspector inside of the Docker Container is that it seamlessly integrates MOV support. All of the steps above
to pull the GPAC library, build it, and then link it to Caption Inspector are done automagically in the Docker File.

```
make docker
docker run -t caption-inspector -h
docker run -tv $(pwd):/files caption-inspector -o /files /files/test/media/BigBuckBunny_256x144-24fps.ts
docker run -tv $(pwd):/files caption-inspector -o /files /files/test/media/Plan9fromOuterSpace.scc -f 2997
docker run -tv $(pwd):/files caption-inspector -o /files /files/test/media/NightOfTheLivingDead.mcc
```

In the `docker run` command, your current working directory will be remapped to `/files` inside of the container, so
you will need to prefix your input and output paths to that so that it can place the files in the correct spot. For
this example, the output file is located in the current directory `./` and the input file is located in a directory
underneath the current directory, specifically `./test/media/*`.

Regression Testing the Caption Inspector Executable
---------------------------------------------------

While the testing is not complete yet, there are several Unit Tests, Integration Tests, and System Tests that run
on the Caption Inspector Codebase. These are intended to be run as integration tests and verify that nothing has
broken as a result of a change. They are run as part of a pull request, but for debugging purposes they can also
be run locally (even in an IDE) before a pull request is issued. The easiest way to run these is inside of docker,
as the dependencies are handled for you. But if you need to run them with an IDE/debugger, they can also be run
from the command line. To run the regression tests from inside of docker, you can build the docker image and run
the tests from the root directory.

```
make docker-test
```

The output generated by this activity can give information about any test case that does not pass, but even more useful
is an HTML file that is the output of the test. It contains all of the tests, and their statuses. It is in the root
directory and takes the form `<date>__<time>_test_output.html`. To view the results you can open the file inside of
a browser.

The Python test suites are run with [pytest](https://docs.pytest.org/) (`pip install pytest`). If pytest is not
installed the Python suites are skipped with a warning and the C tests still run.

The HTML report is produced by [Xunit Viewer](https://github.com/lukejpreston/xunit-viewer), which converts the Xunit
XML into a beautified HTML format. It is optional: if it is not installed, the run notes this and leaves the raw
`<date>__<time>_test_output.xml` in place. Once you have these tools installed, or if you ignore them, you just need to
build and run the regression tests from the test directory.

```
cd test
make test
```

The local test build omits MOV/MP4 (GPAC) support by default, so it builds without any extra dependencies. If you have
built and installed the GPAC caption extractor (see [Building ... with MOV Support](#building-the-caption-inspector-executable-locally-with-mov-support)
above, whose `make install` places it under `/usr/local`), you can include it in the test build:

```
cd test
make test WITH_GPAC=1
```

The Docker test build (`make docker-test`) always includes GPAC.

The tests are a mix of C and Python, depending on which made more sense for the specific test. The build system will
aggregate all of the results from the tests in both languages.

Leveraging the Caption Inspector Functionality from Python
----------------------------------------------------------
C was chosen as the language for Caption Inspector because of interoperability with FFMPEG and ease of implementation of
the specifications. While that choice made sense, there are lots of reasons to want to access this functionality as a
library from a higher level language than C. To that end Python bindings, using CTypes, were added to the C functionality
which can be compiled into a shared library and referenced from Python code. To use this functionality you just need
to build the shared library using the command `make sharedlib`. Then leverage the file `python/cshim.py` inside of your
Python code, making sure that it knows where to find the shared library with the Caption Inspector Code.

The Caption Inspector code that makes this possible, and the library it generates, can likely be reused for other high
level languages such as Java/JNI, Golang/Cgo, etc. No work has been done in those languages, but if you do end up using
it in a new language, please consider submitting the results back to the repository.
