#pragma once
#include <pcrecomp/context.h>
namespace pcrecomp::runtime { PcRuntime* bridge_runtime(); }
extern "C" void recomp_call_time_import(uint32_t id);
extern "C" void recomp_call_fs_import(uint32_t id);
