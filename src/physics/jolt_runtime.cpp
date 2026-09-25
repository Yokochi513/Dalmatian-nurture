#include "physics/jolt_runtime.hpp"

// Jolt.h は他の Jolt のヘッダより先に読む必要がある
#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

namespace dal::physics {

JoltRuntime::JoltRuntime()
{
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

JoltRuntime::~JoltRuntime()
{
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
}

} // namespace dal::physics
