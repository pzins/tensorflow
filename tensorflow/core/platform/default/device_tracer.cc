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
#include "tensorflow/core/cuptiTracer.h"
#include "tensorflow/core/platform/device_tracer.h"
#include <sstream>
#if GOOGLE_CUDA

#include <stdlib.h>
#include <memory>

#include "tensorflow/core/common_runtime/step_stats_collector.h"
#include "tensorflow/core/framework/step_stats.pb.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/cupti_wrapper.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/platform/macros.h"
#include "tensorflow/core/platform/mem.h"
#include "tensorflow/core/platform/mutex.h"
#include "tensorflow/core/platform/tracing.h"
#include "tensorflow/core/lib/strings/str_util.h"

namespace {

// Maps a MemcpyKind enum to a const string.
const char *getMemcpyKindString(CUpti_ActivityMemcpyKind kind) {
  switch (kind) {
    case CUPTI_ACTIVITY_MEMCPY_KIND_HTOD:
      return "HtoD";
    case CUPTI_ACTIVITY_MEMCPY_KIND_DTOH:
      return "DtoH";
    case CUPTI_ACTIVITY_MEMCPY_KIND_HTOA:
      return "HtoA";
    case CUPTI_ACTIVITY_MEMCPY_KIND_ATOH:
      return "AtoH";
    case CUPTI_ACTIVITY_MEMCPY_KIND_ATOA:
      return "AtoA";
    case CUPTI_ACTIVITY_MEMCPY_KIND_ATOD:
      return "AtoD";
    case CUPTI_ACTIVITY_MEMCPY_KIND_DTOA:
      return "DtoA";
    case CUPTI_ACTIVITY_MEMCPY_KIND_DTOD:
      return "DtoD";
    case CUPTI_ACTIVITY_MEMCPY_KIND_HTOH:
      return "HtoH";
    case CUPTI_ACTIVITY_MEMCPY_KIND_PTOP:
      return "PtoP";
    default:
      break;
  }
  return "<unknown>";
}

// Maps a MemoryKind enum to a const string.
const char *getMemoryKindString(CUpti_ActivityMemoryKind kind) {
  switch (kind) {
    case CUPTI_ACTIVITY_MEMORY_KIND_UNKNOWN:
      return "Unknown";
    case CUPTI_ACTIVITY_MEMORY_KIND_PAGEABLE:
      return "Pageable";
    case CUPTI_ACTIVITY_MEMORY_KIND_PINNED:
      return "Pinned";
    case CUPTI_ACTIVITY_MEMORY_KIND_DEVICE:
      return "Device";
    case CUPTI_ACTIVITY_MEMORY_KIND_ARRAY:
      return "Array";
    default:
      break;
  }
  return "<unknown>";
}

// Maps an OverheadKind enum to a const string.
const char *getActivityOverheadKindString(CUpti_ActivityOverheadKind kind) {
  switch (kind) {
    case CUPTI_ACTIVITY_OVERHEAD_DRIVER_COMPILER:
      return "COMPILER";
    case CUPTI_ACTIVITY_OVERHEAD_CUPTI_BUFFER_FLUSH:
      return "BUFFER_FLUSH";
    case CUPTI_ACTIVITY_OVERHEAD_CUPTI_INSTRUMENTATION:
      return "INSTRUMENTATION";
    case CUPTI_ACTIVITY_OVERHEAD_CUPTI_RESOURCE:
      return "RESOURCE";
    default:
      break;
  }
  return "<unknown>";
}

}  // namespace

namespace tensorflow {
namespace devicetracer {

// Forward declaration.
class CUPTIManager;

// Returns a pointer to the CUPTIManager singleton.
CUPTIManager *GetCUPTIManager();

// Callback interface for consumers of CUPTI tracing.
class CUPTIClient {
 public:
  virtual ~CUPTIClient() {}

  // Invoked for each CUPTI activity reported.
  virtual void ActivityCallback(const CUpti_Activity &activity) = 0;
};

#define CUPTI_CALL(call)                                            \
  do {                                                              \
    CUptiResult _status = cupti_wrapper_->call;                     \
    if (_status != CUPTI_SUCCESS) {                                 \
      LOG(ERROR) << "cuda call " << #call << " failed " << _status; \
    }                                                               \
  } while (0)

// Singleton class to manage registration of CUPTI callbacks.
class CUPTIManager {
 public:
  CUPTIManager() {
    cupti_wrapper_.reset(new perftools::gputools::profiler::CuptiWrapper());
    CUPTI_CALL(ActivityRegisterCallbacks(BufferRequested, BufferCompleted));
  }

  // Enables tracing and delivers event callbacks to 'client'.
  // Does not take ownership of client.  Client's lifetime must persist
  // until tracing is disabled.
  Status EnableTrace(CUPTIClient *client);

  // Disable tracing.  No further events will be delivered to 'client'.
  Status DisableTrace();

 private:
  // Static functions which we can use as CUPTI callbacks.
  static void BufferRequested(uint8_t **buffer, size_t *size,
                              size_t *maxNumRecords) {
    GetCUPTIManager()->InternalBufferRequested(buffer, size, maxNumRecords);
  }
  static void BufferCompleted(CUcontext ctx, uint32_t streamId, uint8_t *buffer,
                              size_t size, size_t validSize) {
    GetCUPTIManager()->InternalBufferCompleted(ctx, streamId, buffer, size,
                                               validSize);
  }
  // These methods are called by the static stubs above.
  void InternalBufferRequested(uint8_t **buffer, size_t *size,
                               size_t *maxNumRecords);
  void InternalBufferCompleted(CUcontext ctx, uint32_t streamId,
                               uint8_t *buffer, size_t size, size_t validSize);

  // Size of buffers used for CUPTI tracing.
  static constexpr size_t kBufferSize = 32 * 1024;
  // Required alignment of CUPTI buffers.
  static constexpr size_t kBufferAlignment = 8;

  mutex mu_;
  CUPTIClient *client_ GUARDED_BY(mu_);
  std::unique_ptr<perftools::gputools::profiler::CuptiWrapper> cupti_wrapper_;

  TF_DISALLOW_COPY_AND_ASSIGN(CUPTIManager);
};

Status CUPTIManager::EnableTrace(CUPTIClient *client) {
  mutex_lock l(mu_);
  // TODO(pbar) Work out the minimal set to trace.
  // We can currently manage without driver/runtime tracing.
  // CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_CONTEXT));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_DRIVER));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_RUNTIME));
  // These might be useful for annotations but require NVTX API.
  // CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_NAME));
  // CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_MARKER));

  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_KERNEL));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY2));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_MEMSET));
  CUPTI_CALL(ActivityEnable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  client_ = client;
  return Status::OK();
}

Status CUPTIManager::DisableTrace() {
  // We turn off all tracing regardless.
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_NAME));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_MARKER));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_OVERHEAD));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_CONTEXT));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_DRIVER));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_RUNTIME));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_DEVICE));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_KERNEL));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY2));
  CUPTI_CALL(ActivityDisable(CUPTI_ACTIVITY_KIND_MEMSET));
  CUPTI_CALL(ActivityFlushAll(CUPTI_ACTIVITY_FLAG_FLUSH_FORCED));
  {
    // Don't acquire this lock until Flush returns, since Flush
    // will potentially cause callbacks into BufferCompleted.
    mutex_lock l(mu_);
    client_ = nullptr;
  }
  return Status::OK();
}

void CUPTIManager::InternalBufferRequested(uint8_t **buffer, size_t *size,
                                           size_t *maxNumRecords) {
  VLOG(2) << "BufferRequested";
  void *p = port::AlignedMalloc(kBufferSize, kBufferAlignment);
  *size = kBufferSize;
  *buffer = reinterpret_cast<uint8_t *>(p);
  *maxNumRecords = 0;
}

void CUPTIManager::InternalBufferCompleted(CUcontext ctx, uint32_t streamId,
                                           uint8_t *buffer, size_t size,
                                           size_t validSize) {
  VLOG(2) << "BufferCompleted";
  CUptiResult status;
  CUpti_Activity *record = nullptr;
  mutex_lock l(mu_);  // Hold mu_ while using client_.
  if (client_ && validSize > 0) {
    do {
      status =
          cupti_wrapper_->ActivityGetNextRecord(buffer, validSize, &record);
      if (status == CUPTI_SUCCESS) {
        client_->ActivityCallback(*record);
      } else {
        break;
      }
    } while (1);

    // report any records dropped from the queue
    size_t dropped;
    CUPTI_CALL(ActivityGetNumDroppedRecords(ctx, streamId, &dropped));
    if (dropped != 0) {
      LOG(WARNING) << "Dropped " << dropped << " activity records";
    }
  }
  port::AlignedFree(buffer);
}

CUPTIManager *GetCUPTIManager() {
  static CUPTIManager *manager = new CUPTIManager();
  return manager;
}

#ifdef _MSC_VER
#define __thread __declspec(thread)
#endif

// TODO(pbar) Move this to platform specific header file?
// Static thread local variable for POD types.
#define TF_STATIC_THREAD_LOCAL_POD(_Type_, _var_)                  \
  static __thread _Type_ s_obj_##_var_;                            \
  namespace {                                                      \
  class ThreadLocal_##_var_ {                                      \
   public:                                                         \
    ThreadLocal_##_var_() {}                                       \
    void Init() {}                                                 \
    inline _Type_ *pointer() const { return &s_obj_##_var_; }      \
    inline _Type_ *safe_pointer() const { return &s_obj_##_var_; } \
    _Type_ &get() const { return s_obj_##_var_; }                  \
    bool is_native_tls() const { return true; }                    \
                                                                   \
   private:                                                        \
    TF_DISALLOW_COPY_AND_ASSIGN(ThreadLocal_##_var_);              \
  } _var_;                                                         \
  }  // namespace

// Thread-local state recording the most recent annotation (if any).
// When non-null, this points to a string in the active annotation
// of the current thread.  The annotation is guaranteed to remain live
// for the duration of the CUPTI API callback.
TF_STATIC_THREAD_LOCAL_POD(const char *, tls_current_annotation);


// User data for event collection callback
typedef struct MetricData_st {
  // the device where metric is being collected
  CUdevice device;
  // the set of event groups to collect for a pass
  CUpti_EventGroupSet *eventGroups;
  // the current number of events collected in eventIdArray and
  // eventValueArray
  uint32_t eventIdx;
  // the number of entries in eventIdArray and eventValueArray
  uint32_t numEvents;
  // array of event ids
  CUpti_EventID *eventIdArray;
  // array of event values
  uint64_t *eventValueArray;
} MetricData_t;

typedef struct cupti_eventData_st {
  CUpti_EventGroup eventGroup;
  CUpti_EventID* eventId;
} cupti_eventData;

// Structure to hold data collected by callback
typedef struct RuntimeApiTrace_st {
  cupti_eventData eventData;
  uint64_t* eventVal;
  int nb_events;
} RuntimeApiTrace_t;

static void
displayEventVal(RuntimeApiTrace_t *trace, std::vector<std::vector<string>>& v)
{
    for(int i = 0; i < v.size(); ++i) {
        for(int j = 0; j < v.at(i).size(); ++j) {
            std::cout << v.at(i).at(j) << " : " << (unsigned long long) trace[i].eventVal[j] << std::endl;
        }
    }
}


class DeviceTracerImpl : public DeviceTracer,
                         public CUPTIClient,
                         public port::Tracing::Engine {
 public:
  DeviceTracerImpl();
  ~DeviceTracerImpl() override;

  // DeviceTracer interface:
  Status Start() override;
  Status Stop() override;
  Status Collect(StepStatsCollector *collector) override;

  // port::Tracing::Engine interface:
  bool IsEnabled() const override {
    // We only register the Engine while tracing is enabled.
    return true;
  }
  Annotation *PushAnnotation(StringPiece name) override {
    VLOG(2) << "PushAnnotation " << name;
    struct Impl : public port::Tracing::Engine::Annotation {
      string annotation;
      explicit Impl(StringPiece n) : annotation(n.ToString()) {
        // Remember the most recent ScopedAnnotation for each thread.
        tls_current_annotation.get() = annotation.c_str();
      }
      ~Impl() override { tls_current_annotation.get() = nullptr; }
    };
    return new Impl(name);
  }
  Tracer *StartTracing(StringPiece label, bool is_expensive) override {
    // We don't do anything with 'TraceMe' regions yet.
    return nullptr;
  }

 protected:
  // This callback is used exclusively by CUPTIManager.
  friend class CUPTIManager;
  void ActivityCallback(const CUpti_Activity &activity) override;

 private:
  // Internal struct to record kernel launches.
  struct KernelRecord {
    uint64_t start_timestamp;
    uint64_t end_timestamp;
    uint64_t queued_timestamp;
    uint64_t submitted_timestamp;
    uint32 device_id;
    uint32 stream_id;
    uint32 correlation_id;
  };
  // Internal struct to record memcpy operations.
  struct MemcpyRecord {
    uint64_t start_timestamp;
    uint64_t end_timestamp;
    uint32 device_id;
    uint32 stream_id;
    uint32 correlation_id;
    uint8 copyKind;
    uint8 srcKind;
    uint8 dstKind;
    uint64 bytes;
  };

  // This is the subscriber callback which is invoked directly by CUPTI.
  // The 'userdata' argument will be a pointer to the active 'DeviceTracerImpl'.
  static void CUPTIAPI ApiCallback(void *userdata, CUpti_CallbackDomain domain,
                                   CUpti_CallbackId cbid, const void *cbdata);

  // Records the mapping between correlation ID and kernel name.
  void AddCorrelationId(uint32 correlation_id, const string &name);

  // Returns the current system time in microseconds.
  inline int64 NowInUsec() { return Env::Default()->NowMicros(); }

  CUPTIManager *cupti_manager_;
  std::unique_ptr<perftools::gputools::profiler::CuptiWrapper> cupti_wrapper_;
  CUpti_SubscriberHandle subscriber_;

  mutex trace_mu_;
  static constexpr size_t kMaxRecords = 1024 * 1024;
  std::map<uint32, string> correlations_ GUARDED_BY(trace_mu_);
  std::vector<KernelRecord> kernel_records_ GUARDED_BY(trace_mu_);
  std::vector<MemcpyRecord> memcpy_records_ GUARDED_BY(trace_mu_);

  mutex mu_;
  bool enabled_ GUARDED_BY(mu_);
  int64 start_walltime_us_ GUARDED_BY(mu_);
  int64 end_walltime_us_ GUARDED_BY(mu_);
  uint64_t start_timestamp_ GUARDED_BY(mu_);
  uint64_t end_timestamp_ GUARDED_BY(mu_);

  TF_DISALLOW_COPY_AND_ASSIGN(DeviceTracerImpl);


    CUcontext CUDAcontext;
    CUdevice CUDAdevice;
    // CUpti_SubscriberHandle subscriber;
    MetricData_t* metricData;

    std::vector<string> metricName;
    int nb_metrics;
    CUpti_MetricID* metricId;
    unsigned int pass;
    CUpti_MetricValue *metricValue;


    CUptiResult cuptiErr;
    CUpti_SubscriberHandle subscriber;
    cupti_eventData cuptiEvent;
    RuntimeApiTrace_t* trace;
    std::vector<std::vector<string>> events_str;
    int nb_domains;
    unsigned int cupti_mode;

};

DeviceTracerImpl::DeviceTracerImpl() {
  VLOG(1) << "DeviceTracer created.";
  cupti_manager_ = GetCUPTIManager();
  CHECK(cupti_manager_);
  cupti_wrapper_.reset(new perftools::gputools::profiler::CuptiWrapper());
  enabled_ = false;
  cupti_mode = 0;
}

DeviceTracerImpl::~DeviceTracerImpl() {
  // Unregister the CUPTI callbacks if needed to prevent them from accessing
  // freed memory.
  Stop().IgnoreError();
}

Status DeviceTracerImpl::Start() {
  VLOG(1) << "DeviceTracer::Start";
  mutex_lock l(mu_);
  if (enabled_) {
    return errors::FailedPrecondition("DeviceTracer is already enabled.");
  }
  cupti_wrapper_->ActivityEnableLatencyTimestamps(true);

    CUresult res = cuCtxGetCurrent(&CUDAcontext);
    res = cuDeviceGet(&CUDAdevice, 0);
    const char* env_p;
    if(!(env_p = std::getenv("CUPTI"))) {
        env_p = "";
    }
    if(string(env_p) == "metrics") {
        cupti_mode = 1;
        if(!(env_p = std::getenv("CUPTI_METRICS"))) {
            env_p = "";
        }
         std::string s(env_p);
         metricName = str_util::Split(s, ' ');
         nb_metrics = metricName.size();

        CUptiResult ret;
        ret = cupti_wrapper_->Subscribe(
            &subscriber_, static_cast<CUpti_CallbackFunc>(ApiCallback), this);
        if (ret == CUPTI_ERROR_MAX_LIMIT_REACHED) {
            return errors::Unavailable("CUPTI subcriber limit reached.");
        } else if (ret != CUPTI_SUCCESS) {
            return errors::Internal("Failed to create CUPTI subcriber.");
        }

        cupti_wrapper_->EnableCallback(1, subscriber_, CUPTI_CB_DOMAIN_RUNTIME_API,
                                       CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020);



         metricId = new CUpti_MetricID[nb_metrics];
         metricData = new MetricData_t[nb_metrics];
         metricValue = new CUpti_MetricValue[nb_metrics];
         CUpti_EventGroupSets *passData[nb_metrics];

         for(int i = 0; i < nb_metrics; ++i) {
             // allocate space to hold all the events needed for the metric
             CUPTI_CALL(MetricGetIdFromName(CUDAdevice, metricName.at(i).c_str(), &metricId[i]));
             CUPTI_CALL(MetricGetNumEvents(metricId[i], &metricData[i].numEvents));
             metricData[i].device = CUDAdevice;
             metricData[i].eventIdArray = (CUpti_EventID *)malloc(metricData[i].numEvents * sizeof(CUpti_EventID));
             metricData[i].eventValueArray = (uint64_t *)malloc(metricData[i].numEvents * sizeof(uint64_t));
             metricData[i].eventIdx = 0;
             // get the number of passes required to collect all the events
             // needed for the metric and the event groups for each pass
             CUPTI_CALL(MetricCreateEventGroupSets(CUDAcontext, sizeof(metricId[i]), &metricId[i], &passData[i]));
             metricData[i].eventGroups = passData[i]->sets ;
         }
     } else if(string(env_p) == "events")  {
         cupti_mode = 2;
         if(!(env_p = std::getenv("CUPTI_EVENTS"))) {
             env_p = "";
         }
          std::string s(env_p);
          std::vector<string> tmp = str_util::Split(s, ';');
          nb_domains = tmp.size();

          trace = new RuntimeApiTrace_t[nb_domains];
          for(int i = 0; i < nb_domains; ++i) {
              events_str.push_back(str_util::Split(tmp.at(i), ' '));
              trace[i].nb_events = events_str.at(events_str.size() - 1).size();
          }

          CUresult res = cuCtxGetCurrent(&CUDAcontext);
          res = cuDeviceGet(&CUDAdevice, 0);


          for(int i = 0; i < nb_domains; ++i) {

              trace[i].eventVal = new uint64_t[trace[i].nb_events];
              // just set a inital random value to find easily errors
              for(int o = 0; o < trace[i].nb_events; ++o) trace[i].eventVal[o] = 18;
              trace[i].eventData.eventId = new CUpti_EventID[trace[i].nb_events];
             cuptiErr = cupti_wrapper_->EventGroupCreate(CUDAcontext, &trace[i].eventData.eventGroup, 0);
             for(int j = 0; j < trace[i].nb_events; ++j) {
                 cuptiErr = cupti_wrapper_->EventGetIdFromName(CUDAdevice, events_str.at(i).at(j).c_str(), &trace[i].eventData.eventId[i]);
                 cuptiErr = cupti_wrapper_->EventGroupAddEvent(trace[i].eventData.eventGroup, trace[i].eventData.eventId[i]);
             }
         }

         CUptiResult ret;
         ret = cupti_wrapper_->Subscribe(
             &subscriber_, static_cast<CUpti_CallbackFunc>(ApiCallback), this);
             if (ret == CUPTI_ERROR_MAX_LIMIT_REACHED) {
                 return errors::Unavailable("CUPTI subcriber limit reached.");
             } else if (ret != CUPTI_SUCCESS) {
                 return errors::Internal("Failed to create CUPTI subcriber.");
             }

         cuptiErr = cupti_wrapper_->EnableCallback(1, subscriber_, CUPTI_CB_DOMAIN_RUNTIME_API,
                                        CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020);


     } else {
      // There can only be one CUPTI subscriber.  If we can't create one then
      // there is another trace in progress (possibly by external code).
      CUptiResult ret;
      ret = cupti_wrapper_->Subscribe(
          &subscriber_, static_cast<CUpti_CallbackFunc>(ApiCallback), this);
      if (ret == CUPTI_ERROR_MAX_LIMIT_REACHED) {
        return errors::Unavailable("CUPTI subcriber limit reached.");
      } else if (ret != CUPTI_SUCCESS) {
        return errors::Internal("Failed to create CUPTI subcriber.");
      }
  }

  // Register as a TraceEngine to receive ScopedAnnotations.
  port::Tracing::RegisterEngine(this);

  // Intercept launch and memcpy calls to capture the Op name annotation.
  // TODO(pbar) Add callbacks for memcpy variants.
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel));
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_RUNTIME_API,
                            CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy_v3020));
  CUPTI_CALL(EnableCallback(
      /*enable=*/1, subscriber_, CUPTI_CB_DOMAIN_RUNTIME_API,
      CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyAsync_v3020));

  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH_v2));
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2));
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD_v2));
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2));
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD_v2));
  CUPTI_CALL(EnableCallback(/*enable=*/1, subscriber_,
                            CUPTI_CB_DOMAIN_DRIVER_API,
                            CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2));

  TF_RETURN_IF_ERROR(cupti_manager_->EnableTrace(this));

  CUPTI_CALL(GetTimestamp(&start_timestamp_));
  start_walltime_us_ = NowInUsec();
  enabled_ = true;
  return Status::OK();
}

Status DeviceTracerImpl::Stop() {

    if(cupti_mode == 1) {
      unsigned int pass;
      for(int i = 0; i < nb_metrics; ++i) {

      // use all the collected events to calculate the metric value
     CUPTI_CALL(MetricGetValue(CUDAdevice, metricId[i],
                                    metricData[i].numEvents * sizeof(CUpti_EventID),
                                    metricData[i].eventIdArray,
                                    metricData[i].numEvents * sizeof(uint64_t),
                                    metricData[i].eventValueArray,
                                    0, &metricValue[i]));
                                    //TODO kernelDuration instead of 0 normally

     // const char* metricName = "ipc";
     // print metric value, we format based on the value kind
     {
       CUpti_MetricValueKind valueKind;
       size_t valueKindSize = sizeof(valueKind);
       CUPTI_CALL(MetricGetAttribute(metricId[i], CUPTI_METRIC_ATTR_VALUE_KIND,
                                          &valueKindSize, &valueKind));
       switch (valueKind) {
       case CUPTI_METRIC_VALUE_KIND_DOUBLE:
         printf("Metric %s = %f\n", metricName[i].c_str(), metricValue[i].metricValueDouble);
         break;
       case CUPTI_METRIC_VALUE_KIND_UINT64:
         printf("Metric %s = %llu\n", metricName[i].c_str(),
                (unsigned long long)metricValue[i].metricValueUint64);
         break;
       case CUPTI_METRIC_VALUE_KIND_INT64:
         printf("Metric %s = %lld\n", metricName[i].c_str(),
                (long long)metricValue[i].metricValueInt64);
         break;
       case CUPTI_METRIC_VALUE_KIND_PERCENT:
         printf("Metric %s = %f%%\n", metricName[i].c_str(), metricValue[i].metricValuePercent);
         break;
       case CUPTI_METRIC_VALUE_KIND_THROUGHPUT:
         printf("Metric %s = %llu bytes/sec\n", metricName[i].c_str(),
                (unsigned long long)metricValue[i].metricValueThroughput);
         break;
       case CUPTI_METRIC_VALUE_KIND_UTILIZATION_LEVEL:
         printf("Metric %s = utilization level %u\n", metricName[i].c_str(),
                (unsigned int)metricValue[i].metricValueUtilizationLevel);
         break;
       default:
         fprintf(stderr, "error: unknown value kind\n");
         exit(-1);
       }
     }
     }
 } else if (cupti_mode == 2){
     displayEventVal(trace, events_str);

 }


  VLOG(1) << "DeviceTracer::Stop";
  mutex_lock l(mu_);
  if (!enabled_) {
    return Status::OK();
  }
  CUPTI_CALL(Unsubscribe(subscriber_));
  port::Tracing::RegisterEngine(nullptr);
  TF_RETURN_IF_ERROR(cupti_manager_->DisableTrace());
  end_walltime_us_ = NowInUsec();
  CUPTI_CALL(GetTimestamp(&end_timestamp_));
  enabled_ = false;
  return Status::OK();
}

void DeviceTracerImpl::AddCorrelationId(uint32 correlation_id,
                                        const string &name) {
  VLOG(2) << correlation_id << " : " << name;
  mutex_lock l(trace_mu_);
  if (correlations_.size() >= kMaxRecords) return;
  correlations_.emplace(correlation_id, name);
}

/*static*/ void DeviceTracerImpl::ApiCallback(void *userdata,
                                              CUpti_CallbackDomain domain,
                                              CUpti_CallbackId cbid,
                                              const void *cbdata) {
  auto *cbInfo = reinterpret_cast<const CUpti_CallbackData *>(cbdata);
  DeviceTracerImpl *tracer = reinterpret_cast<DeviceTracerImpl *>(userdata);
  VLOG(2) << "ApiCallback " << domain << ":" << cbid
          << " func: " << cbInfo->functionName;

  if(tracer->cupti_mode == 1) {
      // on entry, enable all the event groups being collected this pass,
      // for metrics we collect for all instances of the event
      if (cbInfo->callbackSite == CUPTI_API_ENTER) {
          cudaDeviceSynchronize();
          CUptiResult res;
          res = tracer->cupti_wrapper_->SetEventCollectionMode(cbInfo->context, CUPTI_EVENT_COLLECTION_MODE_KERNEL);
          if(res != CUPTI_SUCCESS) std::cout << "SetEventCollectionMode failed" << std::endl;
          for(int j = 0; j < tracer->nb_metrics; ++j) {
              for (int i = 0; i < tracer->metricData[j].eventGroups->numEventGroups; i++) {
                  uint32_t all = 1;
                  res = tracer->cupti_wrapper_->EventGroupSetAttribute(tracer->metricData[j].eventGroups->eventGroups[i],
                                                                        CUPTI_EVENT_GROUP_ATTR_PROFILE_ALL_DOMAIN_INSTANCES,
                                                                        sizeof(all), &all);
                  if(res != CUPTI_SUCCESS) std::cout << "EventGroupSetAttribute failed" << std::endl;
                  res = tracer->cupti_wrapper_->EventGroupEnable(tracer->metricData[j].eventGroups->eventGroups[i]);
              }
          }
      }


      // on exit, read and record event values
      if (cbInfo->callbackSite == CUPTI_API_EXIT) {
          cudaDeviceSynchronize();
          // for each group, read the event values from the group and record
          // in tracer->metricData
          for(int idx = 0; idx < tracer->nb_metrics; ++idx) {
              for (int i = 0; i < tracer->metricData[idx].eventGroups->numEventGroups; i++) {
                CUpti_EventGroup group = tracer->metricData[idx].eventGroups->eventGroups[i];
                CUpti_EventDomainID groupDomain;
                uint32_t numEvents, numInstances, numTotalInstances;
                CUpti_EventID *eventIds;
                size_t groupDomainSize = sizeof(groupDomain);
                size_t numEventsSize = sizeof(numEvents);
                size_t numInstancesSize = sizeof(numInstances);
                size_t numTotalInstancesSize = sizeof(numTotalInstances);
                uint64_t *values, normalized, sum;
                size_t valuesSize, eventIdsSize;
                CUptiResult res;
                res = tracer->cupti_wrapper_->EventGroupGetAttribute(group,
                                          CUPTI_EVENT_GROUP_ATTR_EVENT_DOMAIN_ID,
                                          &groupDomainSize, &groupDomain);
                if(res != CUPTI_SUCCESS) std::cout << "EventGroupGetAttribute failed" << std::endl;
                res = tracer->cupti_wrapper_->DeviceGetEventDomainAttribute(tracer->metricData[idx].device, groupDomain,
                                                              CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT,
                                                              &numTotalInstancesSize, &numTotalInstances);
                if(res != CUPTI_SUCCESS) std::cout << "DeviceGetEventDomainAttribute failed" << std::endl;
                res = tracer->cupti_wrapper_->EventGroupGetAttribute(group,
                                           CUPTI_EVENT_GROUP_ATTR_INSTANCE_COUNT,
                                           &numInstancesSize, &numInstances);
                if(res != CUPTI_SUCCESS) std::cout << "EventGroupGetAttribute2 failed" << std::endl;
                res = tracer->cupti_wrapper_->EventGroupGetAttribute(group,
                                          CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS,
                                           &numEventsSize, &numEvents);
                if(res != CUPTI_SUCCESS) std::cout << "EventGroupGetAttribute3 failed" << std::endl;
                eventIdsSize = numEvents * sizeof(CUpti_EventID);
                eventIds = (CUpti_EventID *)malloc(eventIdsSize);
                res = tracer->cupti_wrapper_->EventGroupGetAttribute(group,
                                           CUPTI_EVENT_GROUP_ATTR_EVENTS,
                                           &eventIdsSize, eventIds);
                if(res != CUPTI_SUCCESS) std::cout << "EventGroupGetAttribute4 failed" << std::endl;

                valuesSize = sizeof(uint64_t) * numInstances;
                values = (uint64_t *)malloc(valuesSize);

                tracer->metricData[idx].eventIdx = 0;
                for (int j = 0; j < numEvents; j++) {
                    res = tracer->cupti_wrapper_->EventGroupReadEvent(group, CUPTI_EVENT_READ_FLAG_NONE,
                                                      eventIds[j], &valuesSize, values);
                    if(res != CUPTI_SUCCESS) std::cout << "EventGroupReadEvent failed" << std::endl;

                    if (tracer->metricData[idx].eventIdx >= tracer->metricData[idx].numEvents) {
                        fprintf(stderr, "error: too many events collected, metric expects only %d\n", (int)tracer->metricData[idx].numEvents);
                        continue;
                        exit(-1);
                    }

                    // sum collect event values from all instances
                    sum = 0;
                    for (int k = 0; k < numInstances; k++)
                        sum += values[k];

                    // normalize the event value to represent the total number of
                    // domain instances on the device
                    normalized = (sum * numTotalInstances) / numInstances;

                    tracer->metricData[idx].eventIdArray[tracer->metricData[idx].eventIdx] = eventIds[j];
                    tracer->metricData[idx].eventValueArray[tracer->metricData[idx].eventIdx] = normalized;
                    tracer->metricData[idx].eventIdx++;

                    // print collected value
                    {
                        char eventName[128];
                        size_t eventNameSize = sizeof(eventName) - 1;
                        tracer->cupti_wrapper_->EventGetAttribute(eventIds[j], CUPTI_EVENT_ATTR_NAME,
                                                          &eventNameSize, eventName);
                        eventName[127] = '\0';
                        printf("\t%s = %llu (", eventName, (unsigned long long)sum);
                        if (numInstances > 1) {
                          for (int k = 0; k < numInstances; k++) {
                            if (k != 0)
                              printf(", ");
                            printf("%llu", (unsigned long long)values[k]);
                          }
                        }

                        printf(")\n");
                        printf("\t%s (normalized) (%llu * %u) / %u = %llu\n",
                               eventName, (unsigned long long)sum,
                               numTotalInstances, numInstances,
                               (unsigned long long)normalized);
                    }
                }
                free(values);
              }
              for (int i = 0; i < tracer->metricData[idx].eventGroups->numEventGroups; i++)
                tracer->cupti_wrapper_->EventGroupDisable(tracer->metricData[idx].eventGroups->eventGroups[i]);
          }
      }
  } else if (tracer->cupti_mode == 2){

        if (cbid == CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020 &&
             cbInfo->callbackSite == CUPTI_API_ENTER) {
          cudaDeviceSynchronize();
          tracer->cupti_wrapper_->SetEventCollectionMode(cbInfo->context,
                                                 CUPTI_EVENT_COLLECTION_MODE_KERNEL);
          for(int i = 0; i < tracer->nb_domains; ++i) {
              tracer->cupti_wrapper_->EventGroupEnable(tracer->trace[i].eventData.eventGroup);
          }
      } else if(cbid == CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020 &&
          cbInfo->callbackSite == CUPTI_API_EXIT) {
        cudaDeviceSynchronize();

        for(int i = 0; i < tracer->nb_domains; ++i) {
            size_t numCountersRead = 0;
            size_t arraySizeBytes = sizeof(CUpti_EventID) * tracer->trace[i].nb_events;
            size_t bufferSizeBytes = sizeof(uint64_t) * tracer->trace[i].nb_events;


            tracer->cupti_wrapper_->EventGroupReadAllEvents(tracer->trace[i].eventData.eventGroup,
                                                        CUPTI_EVENT_READ_FLAG_NONE,
                                                        &bufferSizeBytes,
                                                        tracer->trace[i].eventVal,
                                                        &arraySizeBytes,
                                                        tracer->trace[i].eventData.eventId,
                                                        &numCountersRead);

        }
        for(int i = 0; i < tracer->nb_domains; ++i) {
            tracer->cupti_wrapper_->EventGroupDisable(tracer->trace[i].eventData.eventGroup);
        }
    }
  }
  // API callbacks are invoked synchronously on the thread making the
  // CUDA API call.  If this pointer is non-null then the ScopedAnnotation
  // must be valid.
  const char *tls_annotation = tls_current_annotation.get();

  if ((domain == CUPTI_CB_DOMAIN_DRIVER_API) &&
      (cbid == CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel)) {
    if (cbInfo->callbackSite == CUPTI_API_ENTER) {
      auto *params = reinterpret_cast<const cuLaunchKernel_params *>(
          cbInfo->functionParams);
      if (VLOG_IS_ON(2)) {
        VLOG(2) << "LAUNCH stream " << params->hStream << " correllation "
                << cbInfo->correlationId << " kernel " << cbInfo->symbolName;
      }
      const string annotation =
          tls_annotation ? tls_annotation : cbInfo->symbolName;
      tracer->AddCorrelationId(cbInfo->correlationId, annotation);
    }
  } else if ((domain == CUPTI_CB_DOMAIN_RUNTIME_API) &&
             (cbid == CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy_v3020 ||
              cbid == CUPTI_RUNTIME_TRACE_CBID_cudaMemcpyAsync_v3020)) {
    if (cbInfo->callbackSite == CUPTI_API_ENTER) {
      if (VLOG_IS_ON(2)) {
        auto *funcParams = reinterpret_cast<const cudaMemcpy_v3020_params *>(
            cbInfo->functionParams);
        size_t count = funcParams->count;
        enum cudaMemcpyKind kind = funcParams->kind;
        VLOG(2) << "MEMCPY count " << count << " kind " << kind;
      }
      if (tls_annotation) {
        const string annotation = tls_annotation;
        tracer->AddCorrelationId(cbInfo->correlationId, annotation);
      }
    }
  } else if ((domain == CUPTI_CB_DOMAIN_DRIVER_API) &&
             (cbid == CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD_v2 ||
              cbid == CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH_v2 ||
              cbid == CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD_v2 ||
              cbid == CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2 ||
              cbid == CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2 ||
              cbid == CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2)) {
    if (cbInfo->callbackSite == CUPTI_API_EXIT && tls_annotation) {
      const string annotation = tls_annotation;
      tracer->AddCorrelationId(cbInfo->correlationId, annotation);
    }
  } else {
    VLOG(1) << "Unhandled API Callback for " << domain << " " << cbid;
  }
}

void DeviceTracerImpl::ActivityCallback(const CUpti_Activity &record) {
  VLOG(2) << "ActivityCallback " << record.kind;
  mutex_lock l(trace_mu_);
  switch (record.kind) {
    case CUPTI_ACTIVITY_KIND_DRIVER: {
      CUpti_ActivityAPI *api = (CUpti_ActivityAPI *) &record;
      std::stringstream ss;
      ss << api->cbid;
      tracepoint(cuptiTracer, driver_api_entry, "driver_api", ss.str().c_str(), api->start);
      tracepoint(cuptiTracer, driver_api_exit, "driver_api", ss.str().c_str(), api->end);
    //   printf("DRIVER cbid=%u process %u, thread %u, correlation %u\n", api->cbid, api->processId, api->threadId, api->correlationId);
      break;}
    case CUPTI_ACTIVITY_KIND_RUNTIME:{
      CUpti_ActivityAPI *api = (CUpti_ActivityAPI *) &record;
      std::stringstream ss;
      ss << api->cbid;
      tracepoint(cuptiTracer, runtime_api_entry, "runtime_api", ss.str().c_str(), api->start);
      tracepoint(cuptiTracer, runtime_api_exit, "runtime_api", ss.str().c_str(), api->end);
    //   printf("RUNTIME cbid=%u process %u, thread %u, correlation %u\n", api->cbid, api->processId, api->threadId, api->correlationId);
      break;}
    case CUPTI_ACTIVITY_KIND_MEMCPY: {
      if (memcpy_records_.size() >= kMaxRecords) return;
      auto *memcpy = reinterpret_cast<const CUpti_ActivityMemcpy *>(&record);
      memcpy_records_.push_back(MemcpyRecord{
          memcpy->start, memcpy->end, memcpy->deviceId, memcpy->streamId,
          memcpy->correlationId, memcpy->copyKind, memcpy->srcKind,
          memcpy->dstKind, memcpy->bytes});
      break;
    }
    case CUPTI_ACTIVITY_KIND_MEMCPY2: {
      if (memcpy_records_.size() >= kMaxRecords) return;
      auto *memcpy = reinterpret_cast<const CUpti_ActivityMemcpy2 *>(&record);
      memcpy_records_.push_back(MemcpyRecord{
          memcpy->start, memcpy->end, memcpy->deviceId, memcpy->streamId,
          memcpy->correlationId, memcpy->copyKind, memcpy->srcKind,
          memcpy->dstKind, memcpy->bytes});
      break;
    }
    case CUPTI_ACTIVITY_KIND_KERNEL:
    case CUPTI_ACTIVITY_KIND_CONCURRENT_KERNEL: {
      if (kernel_records_.size() >= kMaxRecords) return;
      auto *kernel = reinterpret_cast<const CUpti_ActivityKernel4 *>(&record);
      kernel_records_.push_back(KernelRecord{kernel->start, kernel->end,
                                             kernel->queued, kernel->submitted,
                                             kernel->deviceId, kernel->streamId,
                                             kernel->correlationId});
      break;
    }
    default:
      VLOG(1) << "ActivityCallback unhandled kind";
      break;
  }
}

Status DeviceTracerImpl::Collect(StepStatsCollector *collector) {
  mutex_lock l(mu_);
  if (enabled_) {
    return errors::FailedPrecondition("DeviceTracer is still enabled.");
  }

  // TODO(pbar) Handle device IDs and prefix properly.
  const string prefix = "";
  const int id = 0;
  const string stream_device =
      strings::StrCat(prefix, "/device:GPU:", id, "/stream:");
  const string memcpy_device =
      strings::StrCat(prefix, "/device:GPU:", id, "/memcpy");

  mutex_lock l2(trace_mu_);
  for (const auto &rec : kernel_records_) {
    auto it = correlations_.find(rec.correlation_id);
    const string name = (it != correlations_.cend()) ? it->second : "unknown";
    NodeExecStats *ns = new NodeExecStats;
    int64 start_time = start_walltime_us_ + ((rec.start_timestamp - start_timestamp_) / 1000);
    int64 queued_time = start_walltime_us_ + ((rec.queued_timestamp - start_timestamp_) / 1000);
    ns->set_all_start_micros(start_time);
    ns->set_op_start_rel_micros(0);
    auto elapsed_us =
        std::max<int64>((rec.end_timestamp - rec.start_timestamp) / 1000, 1);
    ns->set_op_end_rel_micros(elapsed_us);
    ns->set_all_end_rel_micros(elapsed_us);
    ns->set_node_name(name);
    // TODO(pbar) Generate details based on the kernel activity record.
    // ns->set_timeline_label(details);
    auto nscopy = new NodeExecStats;
    *nscopy = *ns;
    collector->Save(strings::StrCat(stream_device, "all"), ns);
    collector->Save(strings::StrCat(stream_device, rec.stream_id), nscopy);
    tracepoint(cuptiTracer, kernel_begin, "kernels", name.c_str(), start_time);
    tracepoint(cuptiTracer, kernel_end, "kernels", name.c_str(), elapsed_us + start_time);
    tracepoint(cuptiTracer, kernel_queued, "kernels", name.c_str(), queued_time);
  }
  for (const auto &rec : memcpy_records_) {
    auto it = correlations_.find(rec.correlation_id);
    const string name = (it != correlations_.cend()) ? it->second : "unknown";
    NodeExecStats *ns = new NodeExecStats;
    int64 start_time = start_walltime_us_ + ((rec.start_timestamp - start_timestamp_) / 1000);
    ns->set_all_start_micros(start_time);
    ns->set_op_start_rel_micros(0);
    auto elapsed_us =
        std::max<int64>((rec.end_timestamp - rec.start_timestamp) / 1000, 1);
    ns->set_op_end_rel_micros(elapsed_us);
    ns->set_all_end_rel_micros(elapsed_us);
    auto copyKind = static_cast<CUpti_ActivityMemcpyKind>(rec.copyKind);
    auto srcKind = static_cast<CUpti_ActivityMemoryKind>(rec.srcKind);
    auto dstKind = static_cast<CUpti_ActivityMemoryKind>(rec.dstKind);
    const string details = strings::Printf(
        "MEMCPY%s %llu bytes (%s to %s)", getMemcpyKindString(copyKind),
        rec.bytes, getMemoryKindString(srcKind), getMemoryKindString(dstKind));
    ns->set_node_name(
        strings::StrCat(name, ":MEMCPY", getMemcpyKindString(copyKind)));
    ns->set_timeline_label(details);
    auto nscopy = new NodeExecStats;
    *nscopy = *ns;
    collector->Save(memcpy_device, ns);
    collector->Save(strings::StrCat(stream_device, rec.stream_id), nscopy);
    tracepoint(cuptiTracer, memcpy_begin, "memcpy", name.c_str(), details.c_str(), start_time);
    tracepoint(cuptiTracer, memcpy_end, "memcpy", name.c_str(), details.c_str(), start_time + elapsed_us);
  }
  return Status::OK();
}

}  // namespace devicetracer

std::unique_ptr<DeviceTracer> CreateDeviceTracer() {
  std::unique_ptr<DeviceTracer> tracer(new devicetracer::DeviceTracerImpl());
  return tracer;
}

}  // namespace tensorflow

#else  // GOOGLE_CUDA

namespace tensorflow {

std::unique_ptr<DeviceTracer> CreateDeviceTracer() { return nullptr; }

}  // namespace tensorflow

#endif  // GOOGLE_CUDA
