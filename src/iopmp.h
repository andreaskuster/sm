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

typedef struct {
  uintptr_t cfg;
  uintptr_t a0;
  uintptr_t slot;
} iopmpcfg_t;

void init_iopmp();
void detect_iopmp_granule();
iopmpcfg_t set_iopmp(iopmpcfg_t p);
iopmpcfg_t set_iopmp_napot(uintptr_t base, uintptr_t range, uintptr_t slot);
iopmpcfg_t set_iopmp_napot_access(uintptr_t base, uintptr_t range, uintptr_t access, uintptr_t slot);
iopmpcfg_t set_iopmp_allow_all(uintptr_t slot);
