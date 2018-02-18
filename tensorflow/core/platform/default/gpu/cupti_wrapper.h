/* Copyright 2015 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_CORE_PLATFORM_DEFAULT_CUPTI_WRAPPER_H_
#define TENSORFLOW_CORE_PLATFORM_DEFAULT_CUPTI_WRAPPER_H_

#if GOOGLE_CUDA

#include <stddef.h>
#include <stdint.h>
#if defined(WIN32)
#include "extras/CUPTI/include/cupti.h"
#else
#include "cuda/extras/CUPTI/include/cupti.h"
#endif
namespace perftools {
namespace gputools {
namespace profiler {

// Wraps the CUPTI API so that we can dynamically load the library.
class CuptiWrapper {
 public:
  CuptiWrapper() {}

  // CUPTI activity API
  CUptiResult ActivityDisable(CUpti_ActivityKind kind);

  CUptiResult ActivityEnable(CUpti_ActivityKind kind);

  CUptiResult ActivityFlushAll(uint32_t flag);

  CUptiResult ActivityGetNextRecord(uint8_t* buffer,
                                    size_t valid_buffer_size_bytes,
                                    CUpti_Activity** record);

  CUptiResult ActivityGetNumDroppedRecords(CUcontext context,
                                           uint32_t stream_id, size_t* dropped);

  CUptiResult ActivityRegisterCallbacks(
      CUpti_BuffersCallbackRequestFunc func_buffer_requested,
      CUpti_BuffersCallbackCompleteFunc func_buffer_completed);

  CUptiResult GetDeviceId(CUcontext context, uint32_t* deviceId);

  CUptiResult GetTimestamp(uint64_t* timestamp);

  // CUPTI callback API
  CUptiResult EnableCallback(uint32_t enable, CUpti_SubscriberHandle subscriber,
                             CUpti_CallbackDomain domain,
                             CUpti_CallbackId cbid);

  CUptiResult EnableDomain(uint32_t enable, CUpti_SubscriberHandle subscriber,
                           CUpti_CallbackDomain domain);

  CUptiResult Subscribe(CUpti_SubscriberHandle* subscriber,
                        CUpti_CallbackFunc callback, void* userdata);

  CUptiResult Unsubscribe(CUpti_SubscriberHandle subscriber);
  // new
  CUptiResult MetricGetIdFromName ( CUdevice device, const char* metricName, CUpti_MetricID* metric );
  CUptiResult MetricGetNumProperties ( CUpti_MetricID metric, uint32_t* numProp );
  CUptiResult MetricGetNumEvents(CUpti_MetricID metric, uint32_t* numEvents);
  CUptiResult MetricCreateEventGroupSets ( CUcontext context, size_t metricIdArraySizeBytes, CUpti_MetricID* metricIdArray, CUpti_EventGroupSets** eventGroupPasses );
  CUptiResult MetricGetValue ( CUdevice device, CUpti_MetricID metric, size_t eventIdArraySizeBytes, CUpti_EventID* eventIdArray, size_t eventValueArraySizeBytes, uint64_t* eventValueArray, uint64_t timeDuration, CUpti_MetricValue* metricValue );
  CUptiResult MetricGetAttribute ( CUpti_MetricID metric, CUpti_MetricAttribute attrib, size_t* valueSize, void* value );
  CUptiResult SetEventCollectionMode(CUcontext context, CUpti_EventCollectionMode mode);
  CUptiResult EventGroupSetAttribute(CUpti_EventGroup eventGroup, CUpti_EventGroupAttribute attrib, size_t valueSize, void* value);
  CUptiResult EventGroupEnable(CUpti_EventGroup eventGroup);
  CUptiResult DeviceGetEventDomainAttribute(CUdevice device, CUpti_EventDomainID eventDomain, CUpti_EventDomainAttribute attrib, size_t* valueSize, void* value);
  CUptiResult EventGroupGetAttribute(CUpti_EventGroup eventGroup, CUpti_EventGroupAttribute attrib, size_t* valueSize, void* value);
  CUptiResult EventGroupReadEvent(CUpti_EventGroup eventGroup, CUpti_ReadEventFlags flags, CUpti_EventID event, size_t* eventValueBufferSizeBytes, uint64_t* eventValueBuffer);
  CUptiResult EventGroupDisable(CUpti_EventGroup eventGroup);
  CUptiResult EventGetAttribute(CUpti_EventID event, CUpti_EventAttribute attrib, size_t* valueSize, void* value);
  CUptiResult ActivityEnableLatencyTimestamps(uint8_t enable);
};

}  // namespace profiler
}  // namespace gputools
}  // namespace perftools

#endif  // GOOGLE_CUDA

#endif  // TENSORFLOW_CORE_PLATFORM_DEFAULT_CUPTI_WRAPPER_H_
