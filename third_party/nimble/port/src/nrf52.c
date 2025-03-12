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

#include <system/passert.h>

#undef UNUSED
#include <nrfx.h>

static void (*radio_irq)(void);
static void (*rtc0_irq)(void);
static void (*rng_irq)(void);

void RADIO_IRQHandler(void) {
  if (radio_irq != NULL) {
    radio_irq();
  }
}

void RTC0_IRQHandler(void) {
  if (rtc0_irq != NULL) {
    rtc0_irq();
  }
}

void RNG_IRQHandler(void) {
  if (rng_irq != NULL) {
    rng_irq();
  }
}

void ble_npl_hw_set_isr(int irqn, void (*addr)(void)) {
  switch (irqn) {
    case RADIO_IRQn:
      radio_irq = addr;
      break;
    case RTC0_IRQn:
      rtc0_irq = addr;
      break;
    case RNG_IRQn:
      rng_irq = addr;
      break;
    default:
      WTF;
  }
}

static uint8_t nrf52_clock_hfxo_refcnt;

/**
 * Request HFXO clock be turned on. Note that each request must have a
 * corresponding release.
 *
 * @return int 0: hfxo was already on. 1: hfxo was turned on.
 */
int
nrf52_clock_hfxo_request(void)
{
    int started;

    started = 0;
    vPortEnterCritical();
    PBL_ASSERTN(nrf52_clock_hfxo_refcnt < 0xff);
    if (nrf52_clock_hfxo_refcnt == 0) {
        /* Check the current STATE and SRC of HFCLK */
        if ((NRF_CLOCK->HFCLKSTAT &
             (CLOCK_HFCLKSTAT_SRC_Msk | CLOCK_HFCLKSTAT_STATE_Msk)) !=
            (CLOCK_HFCLKSTAT_SRC_Xtal << CLOCK_HFCLKSTAT_SRC_Pos |
             CLOCK_HFCLKSTAT_STATE_Running << CLOCK_HFCLKSTAT_STATE_Pos)) {
            NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
            NRF_CLOCK->TASKS_HFCLKSTART = 1;
            while (!NRF_CLOCK->EVENTS_HFCLKSTARTED) {
            }
        }
        started = 1;
    }
    ++nrf52_clock_hfxo_refcnt;
    vPortExitCritical();

    return started;
}

/**
 * Release the HFXO. This means that the caller no longer needs the HFXO to be
 * turned on. Each call to release should have been preceeded by a corresponding
 * call to request the HFXO
 *
 *
 * @return int 0: HFXO not stopped by this call (others using it) 1: HFXO
 *         stopped.
 */
int
nrf52_clock_hfxo_release(void)
{
    int stopped;

    stopped = 0;
    vPortEnterCritical();
    PBL_ASSERTN(nrf52_clock_hfxo_refcnt != 0);
    --nrf52_clock_hfxo_refcnt;
    if (nrf52_clock_hfxo_refcnt == 0) {
        NRF_CLOCK->TASKS_HFCLKSTOP = 1;
        stopped = 1;
    }
    vPortExitCritical();

    return stopped;
}
