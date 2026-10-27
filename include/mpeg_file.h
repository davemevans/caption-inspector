/**
 * Copyright 2019 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef mpeg_file_h
#define mpeg_file_h

#ifndef DONT_COMPILE_FFMPEG
#include <libavutil/common.h>
#include <libavformat/avformat.h>
#include <libavutil/avstring.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/log.h>
#include <libavutil/error.h>
#endif

#include "types.h"
#include "pipeline_utils.h"

/*----------------------------------------------------------------------------*/
/*--                               Constants                                --*/
/*----------------------------------------------------------------------------*/

// A timestamp that goes backwards, or forwards by more than this, is a
// discontinuity (a splice or encoder restart) rather than elapsed time. Ten
// seconds is FFmpeg's own default (-dts_delta_threshold).
#define MAX_TIMESTAMP_STEP_SECS                             10

/*----------------------------------------------------------------------------*/
/*--                              Structures                                --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--                                Macros                                  --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--                          Exposed Variables                             --*/
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*--                           Exposed Methods                              --*/
/*----------------------------------------------------------------------------*/

boolean MpegFileInitialize( Context*, uint8 );
uint8 MpegFileAddSink( Context*, LinkInfo );
uint8 MpegFileProcNextBuffer( Context*, boolean* );

#endif /* mpeg_file_h */
