/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include <version.h>
#define IS_ZEPHYR_VERSION_GT(major, minor) \
	(KERNEL_VERSION_MAJOR > (major) || (KERNEL_VERSION_MAJOR == (major) && KERNEL_VERSION_MINOR > (minor)))

#include <assert.h>
#include <float.h>
#include <kernel.h>
#include <sys/util.h>

#if IS_ZEPHYR_VERSION_GT(2,6)
# include <sys/reboot.h>
#else
# include <power/reboot.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <zephyr.h>

#include <tvm/runtime/c_runtime_api.h>
#include <tvm/runtime/crt/logging.h>
#include <tvm/runtime/crt/stack_allocator.h>

#include "input_data.h"
#include "output_data.h"
#include "tvmgen_default.h"

#include "zephyr_uart.h"

#ifdef CONFIG_ARCH_POSIX
#include "posix_board_if.h"
#endif

// Memory footprint for running the inference model
#define WORKSPACE_SIZE TVMGEN_DEFAULT_WORKSPACE_SIZE
static uint8_t g_aot_memory[WORKSPACE_SIZE];
tvm_workspace_t app_workspace;

static uint8_t rx_buffer[128];

void TVMLogf(const char* msg, ...) {
  char buffer[256];
  int size;
  va_list args;
  va_start(args, msg);
  size = vsprintf(buffer, msg, args);
  va_end(args);
  TVMPlatformWriteSerial(buffer, (uint32_t)size);
}

void TVMPlatformAbort(tvm_crt_error_t error) {
  TVMLogf("TVMPlatformAbort: %08x\n", error);
  sys_reboot(SYS_REBOOT_COLD);

  // Halt
  while(true);
}

tvm_crt_error_t TVMPlatformMemoryAllocate(size_t num_bytes, DLDevice dev, void** out_ptr) {
  return StackMemoryManager_Allocate(&app_workspace, num_bytes, out_ptr);
}

tvm_crt_error_t TVMPlatformMemoryFree(void* ptr, DLDevice dev) {
  return StackMemoryManager_Free(&app_workspace, ptr);
}

/*
 * Both TVMBackendAllocWorkspace() and TVMBackendFreeWorkspace() below need to
 * be implemented in TF-M since tvmgen_default_run_model() relies on it. For
 * that TVMPlatformMemoryAllocate() and TVMPlatformMemoryFree(), which are
 * platform-specific, need to be implemented in TF-M.
 * TVMPlatformMemoryAllocate() and TVMPlatformMemoryFree() (above) are part of
 * TVM C Runtime (crt/include/tvm/runtime/crt/stack_allocator.h).
 */

void* TVMBackendAllocWorkspace(int device_type, int device_id, uint64_t nbytes, int dtype_code_hint,
                               int dtype_bits_hint) {
  tvm_crt_error_t err = kTvmErrorNoError;
  void* ptr = 0;
  DLDevice dev = {device_type, device_id};
  assert(nbytes > 0);
  err = TVMPlatformMemoryAllocate(nbytes, dev, &ptr);
  CHECK_EQ(err, kTvmErrorNoError,
           "TVMBackendAllocWorkspace(%d, %d, %" PRIu64 ", %d, %d) -> %" PRId32, device_type,
           device_id, nbytes, dtype_code_hint, dtype_bits_hint, err);
  return ptr;
}

int TVMBackendFreeWorkspace(int device_type, int device_id, void* ptr) {
  tvm_crt_error_t err = kTvmErrorNoError;
  DLDevice dev = {device_type, device_id};
  err = TVMPlatformMemoryFree(ptr, dev);
  return err;
}

void do_inference() {
  int ret_val, i, num_chars;
  char printf_buffer[64];

  StackMemoryManager_Init(&app_workspace, g_aot_memory, WORKSPACE_SIZE);

  // tvmgen_default_run_model() is automatically generated by TVM
  // Please see README.md for details on how to generate it using
  // 'tvmc compile' command.
  //
  // input_data is defined in input_data.h
  // output_data is defined in output_data.h
  // Both input_data and output_data must be manually set to match
  // the input/output tensor shapes for the compiled model using
  // 'tvmc compile' (sine model in this case).
  //
  ret_val = tvmgen_default_run_model(input_data, output_data); // <================= HERE WE CALL THE INFERENCE MODEL

  if (ret_val != 0) {
    TVMLogf("Error: %d\n", ret_val);
    TVMPlatformAbort(kTvmErrorPlatformCheckFailure);
  }

  // Print output_data[]
  for (i = 0; i < OUTPUT_DATA_LEN; i++) {
    num_chars = sprintf(printf_buffer, "output_data[%d]: %f\r\n", i, output_data[i]);
    TVMPlatformWriteSerial(printf_buffer, num_chars);
  }
}


void main(void) {
  // Just initialize Zephyr's UART device
  TVMPlatformUARTInit();
  
  TVMLogf("Hit 'X' to do inference:\r\n");
  while (true) {
    int bytes_read = TVMPlatformUartRxRead(rx_buffer, sizeof(rx_buffer));
    if (bytes_read > 0) {
      if (rx_buffer[0] == 'X') {
        do_inference();
      }
    }
  }
}
