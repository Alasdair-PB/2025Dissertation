#include "ProfilingModule.h"
#define LOCTEXT_NAMESPACE "FProfilingModule"

void FProfilingModule::StartupModule(){}
void FProfilingModule::ShutdownModule(){}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FProfilingModule, Profiling)