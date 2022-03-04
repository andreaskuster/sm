/*
 * Copyright 2022 ETH Zurich and University of Bologna.
 * Copyright and related rights are licensed under the Solderpad Hardware
 * License, Version 0.51 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://solderpad.org/licenses/SHL-0.51. Unless required by applicable law
 * or agreed to in writing, software, hardware and materials distributed under
 * this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * Author: Andreas Kuster <kustera@ethz.ch>
 * Description: IO-PMP setup and configuration library
 */

#include <stdint.h>

#include <sbi/sbi_console.h>

#include <sbi/riscv_encoding.h>
#include "iopmp.h"

#define IOPMP_BASE      0x50010000
#define IOPMP_ADDR_BASE (IOPMP_BASE + 0x0)
#define IOPMP_CFG_BASE  (IOPMP_BASE + 0x80)
#define IOPMP_NUM_PMP   16

int iopmp_granule = -1;

void init_iopmp() {

    // enforce granularity detection
    if(iopmp_granule < 0){
        detect_iopmp_granule();
    }

    /*
     * Check IO-PMP address registers
     */
    for(int i = 0; i < IOPMP_NUM_PMP; i++){
        // read/write base registers
        volatile uintptr_t *pmpaddr = (volatile uintptr_t *) (IOPMP_ADDR_BASE + (i*0x8ULL));
        // read old value: init -> set zero
        uintptr_t old_pmpaddr = 0;
        // set new value
        *pmpaddr = 0xffffffffffffffffULL;
        uintptr_t match = (0x3FFFFFFFFFFFFF & ~((iopmp_granule>>2)-1));

        // check if read-back value matches
        if (*pmpaddr != match) {
            sbi_printf("IO-PMP%d: address read/write failed: %lx\n", i, *pmpaddr);
        } else {
            sbi_printf("IO-PMP%d: address read/write succeeded\n", i);
        }
        // restore previous value
        *pmpaddr = old_pmpaddr;
    }

    /*
     * Check IO-PMP config
     */
    // read/write base registers
    volatile uintptr_t *pmpcfg0 = (volatile uintptr_t *) IOPMP_CFG_BASE;
    volatile uintptr_t *pmpcfg1 = (volatile uintptr_t *) (IOPMP_CFG_BASE + 0x8ULL);

    // read old value: init -> set zero
    uintptr_t old_cfg0 = 0;
    uintptr_t old_cfg1 = 0;
    if(IOPMP_NUM_PMP > 8) {
        old_cfg1 = 0;
    }

    // set new value
    *pmpcfg0 = 0xffffffffffffffffULL;
    if(IOPMP_NUM_PMP > 8) {
        *pmpcfg1 = 0xffffffffffffffffULL;
    }

    // check if read-back value matches
    if (*pmpcfg0 != 0xffffffffffffffffULL || (IOPMP_NUM_PMP > 8 && *pmpcfg1 != 0xffffffffffffffffULL)) {
        sbi_printf("IO-PMP: cfg read/write failed\n");
    } else {
        sbi_printf("IO-PMP: cfg read/write succeeded\n");
    }

    // restore previous value
    *pmpcfg0 = old_cfg0;
    if(IOPMP_NUM_PMP > 8) {
        *pmpcfg1 = old_cfg1;
    }
}

void detect_iopmp_granule() {

    // detect pmp granule size according to the RISC-V PMP specs
    volatile uintptr_t *pmpcfg0 = (volatile uintptr_t *) IOPMP_CFG_BASE;
    volatile uintptr_t *pmpaddr0 = (volatile uintptr_t *) IOPMP_ADDR_BASE;

    //  write 0 to config & 0xff..ff to address
    *pmpcfg0 = 0x0ULL;
    *pmpaddr0 = 0xffffffffffffffffULL;

    // read address and extract num_zeros on lsb
    uintptr_t ret = *pmpaddr0;
    int g = 2;
    for (uintptr_t i = 1; i; i <<= 1) {
        if ((ret & i) != 0)
            break;
        g++;
    }
    iopmp_granule = 1UL << g;

    // print result
    sbi_printf("IO-PMP granularity: %d\n", iopmp_granule);
}

iopmpcfg_t set_iopmp(iopmpcfg_t p) {

    if(p.slot < 0 || p.slot >= IOPMP_NUM_PMP){
        //sbi_printf("PMP invalid slot!\n");
        return p;
    }
    // POST: 0 <= p.slot < 16

    volatile uintptr_t *pmpcfg = (volatile uintptr_t *) (IOPMP_CFG_BASE + ((p.slot < 8) ? 0x0ULL : 0x8ULL));

    uintptr_t mask, shift;
    if(p.slot >= 8){
        shift = p.slot-8;
    } else {
        shift = p.slot;
    }
    mask = (0xff << (8 * shift));

    *pmpcfg = *pmpcfg & ~mask;

    volatile uintptr_t *pmpaddr = (volatile uintptr_t *) (IOPMP_ADDR_BASE + (8 * p.slot));
    *pmpaddr = p.a0;

    *pmpcfg =  ((p.cfg << (8 * shift)) & mask) | (*pmpcfg & ~mask);

    //sbi_printf("Set IO-PMP: slot: %lx, addr: %lx, cfg: %lx\n", p.slot, p.a0, p.cfg);

    return p;
}

iopmpcfg_t set_iopmp_napot(uintptr_t base, uintptr_t range, uintptr_t slot) {
    // set pmp with r/w/x access
    return set_iopmp_napot_access(base, range, (PMP_W | PMP_R | PMP_X), slot);
}

iopmpcfg_t set_iopmp_napot_access(uintptr_t base, uintptr_t range, uintptr_t access, uintptr_t slot) {
    iopmpcfg_t p;
    p.cfg = access | (range > iopmp_granule ? PMP_A_NAPOT : PMP_A_NA4);
    p.a0 = (base + (range / 2 - 1)) >> PMP_SHIFT;
    p.slot = slot;
    return set_iopmp(p);
}

iopmpcfg_t set_iopmp_allow_all(uintptr_t slot) {
    iopmpcfg_t p;
    p.cfg = (PMP_W | PMP_R | PMP_X) | PMP_A_NAPOT;
    p.a0 = 0xFFFFFFFFFFFFFFFF;
    p.slot = slot;
    return set_iopmp(p);
}
