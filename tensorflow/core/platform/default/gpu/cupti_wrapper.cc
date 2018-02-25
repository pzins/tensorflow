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

#include "tensorflow/core/platform/default/gpu/cupti_wrapper.h"

#if GOOGLE_CUDA

#include <string>

#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/stream_executor.h"

namespace perftools {
namespace gputools {
namespace profiler {

namespace dynload {

#define LIBCUPTI_WRAP(__name)                                               \
  struct DynLoadShim__##__name {                                            \
    static const char* kName;                                               \
    using FuncPointerT = std::add_pointer<decltype(::__name)>::type;        \
    static void* GetDsoHandle() {                                           \
      static auto status = perftools::gputools::internal::CachedDsoLoader:: \
          GetLibcuptiDsoHandle();                                           \
      return status.ValueOrDie();                                           \
    }                                                                       \
    static FuncPointerT DynLoad() {                                         \
      static void* f;                                                       \
      TF_CHECK_OK(::tensorflow::Env::Default()->GetSymbolFromLibrary(       \
          GetDsoHandle(), kName, &f))                                       \
          << "could not find " << kName << "in libcupti DSO";               \
      return reinterpret_cast<FuncPointerT>(f);                             \
    }                                                                       \
    template <typename... Args>                                             \
    CUptiResult operator()(Args... args) {                                  \
      return DynLoad()(args...);                                            \
    }                                                                       \
  } __name;                                                                 \
  const char* DynLoadShim__##__name::kName = #__name;

LIBCUPTI_WRAP(cuptiActivityDisable);
LIBCUPTI_WRAP(cuptiActivityEnable);
LIBCUPTI_WRAP(cuptiActivityFlushAll);
LIBCUPTI_WRAP(cuptiActivityGetNextRecord);
LIBCUPTI_WRAP(cuptiActivityGetNumDroppedRecords);
LIBCUPTI_WRAP(cuptiActivityRegisterCallbacks);
LIBCUPTI_WRAP(cuptiGetTimestamp);
LIBCUPTI_WRAP(cuptiEnableCallback);
LIBCUPTI_WRAP(cuptiEnableDomain);
LIBCUPTI_WRAP(cuptiSubscribe);
LIBCUPTI_WRAP(cuptiUnsubscribe);

// new
LIBCUPTI_WRAP(cuptiMetricGetIdFromName);
LIBCUPTI_WRAP(cuptiMetricGetNumProperties);
LIBCUPTI_WRAP(cuptiMetricGetNumEvents);
LIBCUPTI_WRAP(cuptiMetricCreateEventGroupSets);
LIBCUPTI_WRAP(cuptiMetricGetValue);
LIBCUPTI_WRAP(cuptiMetricGetAttribute);
LIBCUPTI_WRAP(cuptiSetEventCollectionMode);
LIBCUPTI_WRAP(cuptiEventGroupSetAttribute);
LIBCUPTI_WRAP(cuptiEventGroupEnable);
LIBCUPTI_WRAP(cuptiDeviceGetEventDomainAttribute);
LIBCUPTI_WRAP(cuptiEventGroupGetAttribute);
LIBCUPTI_WRAP(cuptiEventGroupReadEvent);
LIBCUPTI_WRAP(cuptiEventGetAttribute);
LIBCUPTI_WRAP(cuptiEventGroupDisable);
LIBCUPTI_WRAP(cuptiActivityEnableLatencyTimestamps);

LIBCUPTI_WRAP(cuptiEventGroupAddEvent);
LIBCUPTI_WRAP(cuptiEventGetIdFromName);
LIBCUPTI_WRAP(cuptiEventGroupCreate);
LIBCUPTI_WRAP(cuptiEventGroupReadAllEvents);

} //namespace dynload

CUptiResult CuptiWrapper::EventGroupReadAllEvents(CUpti_EventGroup eventGroup, CUpti_ReadEventFlags flags, size_t* eventValueBufferSizeBytes, uint64_t* eventValueBuffer, size_t* eventIdArraySizeBytes, CUpti_EventID* eventIdArray, size_t* numEventIdsRead) {
    return dynload::cuptiEventGroupReadAllEvents(eventGroup, flags, eventValueBufferSizeBytes, eventValueBuffer, eventIdArraySizeBytes, eventIdArray, numEventIdsRead);
}

CUptiResult CuptiWrapper::ActivityEnableLatencyTimestamps(uint8_t enable) {
    return dynload::cuptiActivityEnableLatencyTimestamps(enable);
}


CUptiResult CuptiWrapper::ActivityDisable(CUpti_ActivityKind kind) {
  return dynload::cuptiActivityDisable(kind);
}

CUptiResult CuptiWrapper::ActivityEnable(CUpti_ActivityKind kind) {
  return dynload::cuptiActivityEnable(kind);
}

CUptiResult CuptiWrapper::ActivityFlushAll(uint32_t flag) {
  return dynload::cuptiActivityFlushAll(flag);
}

CUptiResult CuptiWrapper::ActivityGetNextRecord(uint8_t* buffer,
                                                size_t valid_buffer_size_bytes,
                                                CUpti_Activity** record) {
  return dynload::cuptiActivityGetNextRecord(buffer, valid_buffer_size_bytes,
                                             record);
}

CUptiResult CuptiWrapper::ActivityGetNumDroppedRecords(CUcontext context,
                                                       uint32_t stream_id,
                                                       size_t* dropped) {
  return dynload::cuptiActivityGetNumDroppedRecords(context, stream_id,
                                                    dropped);
}

CUptiResult CuptiWrapper::ActivityRegisterCallbacks(
    CUpti_BuffersCallbackRequestFunc func_buffer_requested,
    CUpti_BuffersCallbackCompleteFunc func_buffer_completed) {
  return dynload::cuptiActivityRegisterCallbacks(func_buffer_requested,
                                                 func_buffer_completed);
}

CUptiResult CuptiWrapper::GetTimestamp(uint64_t* timestamp) {
  return dynload::cuptiGetTimestamp(timestamp);
}

CUptiResult CuptiWrapper::EnableCallback(uint32_t enable,
                                         CUpti_SubscriberHandle subscriber,
                                         CUpti_CallbackDomain domain,
                                         CUpti_CallbackId cbid) {
  return dynload::cuptiEnableCallback(enable, subscriber, domain, cbid);
}

CUptiResult CuptiWrapper::EnableDomain(uint32_t enable,
                                       CUpti_SubscriberHandle subscriber,
                                       CUpti_CallbackDomain domain) {
  return dynload::cuptiEnableDomain(enable, subscriber, domain);
}

CUptiResult CuptiWrapper::Subscribe(CUpti_SubscriberHandle* subscriber,
                                    CUpti_CallbackFunc callback,
                                    void* userdata) {
  return dynload::cuptiSubscribe(subscriber, callback, userdata);
}

CUptiResult CuptiWrapper::Unsubscribe(CUpti_SubscriberHandle subscriber) {
  return dynload::cuptiUnsubscribe(subscriber);
}


CUptiResult CuptiWrapper::MetricGetIdFromName(CUdevice device, const char* metricName, CUpti_MetricID* metric) {
    return dynload::cuptiMetricGetIdFromName(device, metricName, metric);
}

CUptiResult CuptiWrapper::MetricGetNumProperties(CUpti_MetricID metric, uint32_t* numProp) {
    return dynload::cuptiMetricGetNumProperties(metric, numProp);
}

CUptiResult CuptiWrapper::MetricGetNumEvents(CUpti_MetricID metric, uint32_t* numEvents) {
    return dynload::cuptiMetricGetNumEvents(metric, numEvents);
}

CUptiResult CuptiWrapper::MetricCreateEventGroupSets(CUcontext context, size_t metricIdArraySizeBytes, CUpti_MetricID* metricIdArray, CUpti_EventGroupSets** eventGroupPasses) {
    return dynload::cuptiMetricCreateEventGroupSets(context, metricIdArraySizeBytes, metricIdArray, eventGroupPasses);
}

CUptiResult CuptiWrapper::MetricGetValue(CUdevice device, CUpti_MetricID metric, size_t eventIdArraySizeBytes, CUpti_EventID* eventIdArray, size_t eventValueArraySizeBytes, uint64_t* eventValueArray, uint64_t timeDuration, CUpti_MetricValue* metricValue) {
    return dynload::cuptiMetricGetValue(device, metric, eventIdArraySizeBytes, eventIdArray, eventValueArraySizeBytes, eventValueArray, timeDuration, metricValue);
}

CUptiResult CuptiWrapper::MetricGetAttribute(CUpti_MetricID metric, CUpti_MetricAttribute attrib, size_t* valueSize, void* value) {
    return dynload::cuptiMetricGetAttribute(metric, attrib, valueSize, value);
}

CUptiResult CuptiWrapper::SetEventCollectionMode(CUcontext context, CUpti_EventCollectionMode mode) {
    return dynload::cuptiSetEventCollectionMode(context, mode);
}

CUptiResult CuptiWrapper::EventGroupSetAttribute(CUpti_EventGroup eventGroup, CUpti_EventGroupAttribute attrib, size_t valueSize, void* value) {
    return dynload::cuptiEventGroupSetAttribute(eventGroup, attrib, valueSize, value);
}

CUptiResult CuptiWrapper::EventGroupEnable(CUpti_EventGroup eventGroup) {
    return dynload::cuptiEventGroupEnable(eventGroup);
}

CUptiResult CuptiWrapper::DeviceGetEventDomainAttribute(CUdevice device, CUpti_EventDomainID eventDomain, CUpti_EventDomainAttribute attrib, size_t* valueSize, void* value) {
    return dynload::cuptiDeviceGetEventDomainAttribute(device, eventDomain, attrib, valueSize, value);
}

CUptiResult CuptiWrapper::EventGroupGetAttribute(CUpti_EventGroup eventGroup, CUpti_EventGroupAttribute attrib, size_t* valueSize, void* value) {
    return dynload::cuptiEventGroupGetAttribute(eventGroup, attrib, valueSize, value);
}

CUptiResult CuptiWrapper::EventGroupReadEvent(CUpti_EventGroup eventGroup, CUpti_ReadEventFlags flags, CUpti_EventID event, size_t* eventValueBufferSizeBytes, uint64_t* eventValueBuffer) {
    return dynload::cuptiEventGroupReadEvent(eventGroup, flags, event, eventValueBufferSizeBytes, eventValueBuffer);
}


CUptiResult CuptiWrapper::EventGroupDisable(CUpti_EventGroup eventGroup) {
    return dynload::cuptiEventGroupDisable(eventGroup);
}

CUptiResult CuptiWrapper::EventGetAttribute(CUpti_EventID event, CUpti_EventAttribute attrib, size_t* valueSize, void* value) {
    return dynload::cuptiEventGetAttribute(event, attrib, valueSize, value);
}


CUptiResult CuptiWrapper::EventGroupAddEvent(CUpti_EventGroup eventGroup, CUpti_EventID event) {
    return dynload::cuptiEventGroupAddEvent(eventGroup, event);
}

CUptiResult CuptiWrapper::EventGetIdFromName(CUdevice device, const char* eventName, CUpti_EventID* event) {
    return dynload::cuptiEventGetIdFromName(device, eventName, event);
}

CUptiResult CuptiWrapper::EventGroupCreate(CUcontext context, CUpti_EventGroup* eventGroup, uint32_t flags) {
    return dynload::cuptiEventGroupCreate(context, eventGroup, flags);
}




}  // namespace profiler
}  // namespace gputools
}  // namespace perftools

#endif  // GOOGLE_CUDA
