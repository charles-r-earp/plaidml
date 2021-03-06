// Copyright 2017-2018 Intel Corporation.

#include "tile/hal/opencl/device_memory.h"

#include <utility>

#include "tile/hal/opencl/cl_mem_arena.h"
#include "tile/hal/opencl/cl_mem_buffer.h"

namespace vertexai {
namespace tile {
namespace hal {
namespace opencl {

DeviceMemory::DeviceMemory(const std::shared_ptr<DeviceState>& device_state) : device_state_{device_state} {}

std::shared_ptr<hal::Buffer> DeviceMemory::MakeBuffer(std::uint64_t size, BufferAccessMask /* access */) {
  Err err;
  CLObj<cl_mem> mem = ocl::CreateBuffer(device_state_->cl_ctx().get(), CL_MEM_READ_WRITE, size, nullptr, err.ptr());
  Err::Check(err, "Unable to allocate device-local memory");
  return std::make_shared<CLMemBuffer>(device_state_, size, std::move(mem));
}

std::shared_ptr<hal::Arena> DeviceMemory::MakeArena(std::uint64_t size, BufferAccessMask /* access */) {
  Err err;
  CLObj<cl_mem> mem = ocl::CreateBuffer(device_state_->cl_ctx().get(), CL_MEM_READ_WRITE, size, nullptr, err.ptr());
  Err::Check(err, "Unable to allocate device-local memory");
  return std::make_shared<CLMemArena>(device_state_, size, std::move(mem));
}

}  // namespace opencl
}  // namespace hal
}  // namespace tile
}  // namespace vertexai
