/*
 * Copyright 2022      Andreas Schneider <asn@cryptomilk.org>
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
 */

#ifndef _CMOCKA_VERSION_H
#define _CMOCKA_VERSION_H

/* cmocka version macros */
#define CM_VERSION_INT(a, b, c) ((a) << 16 | (b) << 8 | (c))
#define CM_VERSION_DOT(a, b, c) a ##.## b ##.## c
#define CM_VERSION(a, b, c) CMOCKA_VERSION_DOT(a, b, c)

/* cmocka version */
#define CMOCKA_VERSION_MAJOR  @cmocka_VERSION_MAJOR@
#define CMOCKA_VERSION_MINOR  @cmocka_VERSION_MINOR@
#define CMOCKA_VERSION_MICRO  @cmocka_VERSION_PATCH@

#define CMOCKA_VERSION_INT CM_VERSION_INT(CMOCKA_VERSION_MAJOR, \
                                          CMOCKA_VERSION_MINOR, \
                                          CMOCKA_VERSION_MICRO)
#define CMOCKA_VERSION     CM_VERSION(CMOCKA_VERSION_MAJOR, \
                                      CMOCKA_VERSION_MINOR, \
                                      CMOCKA_VERSION_MICRO)

#endif /* _CMOCKA_VERSION_H */
