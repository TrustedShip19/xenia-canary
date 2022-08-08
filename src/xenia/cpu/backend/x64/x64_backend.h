/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_CPU_BACKEND_X64_X64_BACKEND_H_
#define XENIA_CPU_BACKEND_X64_X64_BACKEND_H_

#include <memory>

#include "xenia/base/cvar.h"
#include "xenia/cpu/backend/backend.h"

DECLARE_int32(x64_extension_mask);

namespace xe {
class Exception;
}  // namespace xe
namespace xe {
namespace cpu {
namespace backend {
namespace x64 {

class X64CodeCache;

typedef void* (*HostToGuestThunk)(void* target, void* arg0, void* arg1);
typedef void* (*GuestToHostThunk)(void* target, void* arg0, void* arg1);
typedef void (*ResolveFunctionThunk)();

// located prior to the ctx register
// some things it would be nice to have be per-emulator instance instead of per
// context (somehow placing a global X64BackendCtx prior to membase, so we can
// negatively index the membase reg)
struct X64BackendContext {
  void* ResolveFunction_Ptr;  // cached pointer to resolvefunction
  unsigned int mxcsr_fpu;     // currently, the way we implement rounding mode
                              // affects both vmx and the fpu
  unsigned int mxcsr_vmx;
  unsigned int flags;   // bit 0 = 0 if mxcsr is fpu, else it is vmx
  unsigned int Ox1000;  // constant 0x1000 so we can shrink each tail emitted
                        // add of it by... 2 bytes lol
};
constexpr unsigned int DEFAULT_VMX_MXCSR =
    0x8000 |                   // flush to zero
    0x0040 | (_MM_MASK_MASK);  // default rounding mode for vmx

constexpr unsigned int DEFAULT_FPU_MXCSR = 0x1F80;
extern const uint32_t mxcsr_table[8];
class X64Backend : public Backend {
 public:
  static const uint32_t kForceReturnAddress = 0x9FFF0000u;

  explicit X64Backend();
  ~X64Backend() override;

  X64CodeCache* code_cache() const { return code_cache_.get(); }
  uintptr_t emitter_data() const { return emitter_data_; }

  // Call a generated function, saving all stack parameters.
  HostToGuestThunk host_to_guest_thunk() const { return host_to_guest_thunk_; }
  // Function that guest code can call to transition into host code.
  GuestToHostThunk guest_to_host_thunk() const { return guest_to_host_thunk_; }
  // Function that thunks to the ResolveFunction in X64Emitter.
  ResolveFunctionThunk resolve_function_thunk() const {
    return resolve_function_thunk_;
  }

  bool Initialize(Processor* processor) override;

  void CommitExecutableRange(uint32_t guest_low, uint32_t guest_high) override;

  std::unique_ptr<Assembler> CreateAssembler() override;

  std::unique_ptr<GuestFunction> CreateGuestFunction(Module* module,
                                                     uint32_t address) override;

  uint64_t CalculateNextHostInstruction(ThreadDebugInfo* thread_info,
                                        uint64_t current_pc) override;

  void InstallBreakpoint(Breakpoint* breakpoint) override;
  void InstallBreakpoint(Breakpoint* breakpoint, Function* fn) override;
  void UninstallBreakpoint(Breakpoint* breakpoint) override;
  virtual void InitializeBackendContext(void* ctx) override;

  X64BackendContext* BackendContextForGuestContext(void* ctx) {
    return reinterpret_cast<X64BackendContext*>(
        reinterpret_cast<intptr_t>(ctx) - sizeof(X64BackendContext));
  }
  virtual void SetGuestRoundingMode(void* ctx, unsigned int mode) override;

 private:
  static bool ExceptionCallbackThunk(Exception* ex, void* data);
  bool ExceptionCallback(Exception* ex);

  uintptr_t capstone_handle_ = 0;

  std::unique_ptr<X64CodeCache> code_cache_;
  uintptr_t emitter_data_ = 0;

  HostToGuestThunk host_to_guest_thunk_;
  GuestToHostThunk guest_to_host_thunk_;
  ResolveFunctionThunk resolve_function_thunk_;
};

}  // namespace x64
}  // namespace backend
}  // namespace cpu
}  // namespace xe

#endif  // XENIA_CPU_BACKEND_X64_X64_BACKEND_H_
