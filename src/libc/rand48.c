/****************************************************************************
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

 #include <stdlib.h>
 #include <stdint.h>
#include <string.h>

 static unsigned short int g_seed48[7] =
 {
   0,
   0,
   0,
   0xe66d,
   0xdeec,
   0x5,
   0xb
 };
 
 static uint64_t rand48_step(unsigned short int *xi,
                             unsigned short int *lc)
 {
   uint64_t a;
   uint64_t x;
 
   x = xi[0] | ((xi[1] + 0ul) << 16) | ((xi[2] + 0ull) << 32);
   a = lc[0] | ((lc[1] + 0ul) << 16) | ((lc[2] + 0ull) << 32);
   x = a * x + lc[3];
 
   xi[0] = x;
   xi[1] = x >> 16;
   xi[2] = x >> 32;
   return x & 0xffffffffffffull;
 }
 
 void srand48(long seed)
 {
   unsigned short int p[3];
 
   p[0] = 0x330e;
   p[1] = seed;
   p[2] = seed >> 16;
   seed48(p);
 }

 unsigned short int *seed48(unsigned short int seed16v[3])
 {
   static unsigned short int p[3];
 
   memcpy(p, g_seed48, sizeof(p));
   memcpy(g_seed48, seed16v, sizeof(p));
   return p;
 }
 

 long jrand48(unsigned short int s[3])
 {
   return (long)(rand48_step(s, g_seed48 + 3) >> 16);
 }
