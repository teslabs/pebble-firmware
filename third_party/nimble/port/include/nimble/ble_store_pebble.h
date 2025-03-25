/*
 * Copyright 2025 Core Devices LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BLE_STORE_PEBBLE_H_
#define _BLE_STORE_PEBBLE_H_

#include <bluetooth/bonding_sync.h>

void ble_store_pebble_init(void);
void ble_store_pebble_add_bonding(const BleBonding *bonding);
void ble_store_pebble_remove_bonding(const BleBonding *bonding);

#endif