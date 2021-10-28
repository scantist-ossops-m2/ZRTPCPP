//
//Licensed under the Apache License, Version 2.0 (the "License");
//you may not use this file except in compliance with the License.
//You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
//Unless required by applicable law or agreed to in writing, software
//distributed under the License is distributed on an "AS IS" BASIS,
//WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//See the License for the specific language governing permissions and
//limitations under the License.
//
// Created by werner on 26.01.20.
// Copyright (c) 2020 Werner Dittmann. All rights reserved.
//

#ifndef LIBZRTPCPP_CONSTANTS_H
#define LIBZRTPCPP_CONSTANTS_H

#ifndef SHA256_DIGEST_LENGTH
#define SHA256_DIGEST_LENGTH 32
#endif

#ifndef SHA384_DIGEST_LENGTH
#define SHA384_DIGEST_LENGTH 48
#endif

#ifndef SKEIN256_DIGEST_LENGTH
#define SKEIN256_DIGEST_LENGTH  32
#endif

#ifndef SKEIN384_DIGEST_LENGTH
#define SKEIN384_DIGEST_LENGTH  48
#endif

// Prepare to support digest algorithms up to 512 bit (64 bytes)
#define MAX_DIGEST_LENGTH       64
#define IMPL_MAX_DIGEST_LENGTH  64

// max. number of parallel supported ZRTP protocol versions.
#define MAX_ZRTP_VERSIONS       2

// currently only 1.10 supported
#define SUPPORTED_ZRTP_VERSIONS       1

// Integer representation of highest supported ZRTP protocol version
#define HIGHEST_ZRTP_VERION    12

#endif //LIBZRTPCPP_CONSTANTS_H
