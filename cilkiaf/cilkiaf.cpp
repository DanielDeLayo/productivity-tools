#include <cilk/cilk_api.h>
#include <cilk/ostream_reducer.h>
#include <csi/csi.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string.h>
#include <mutex>

#include "bounded_iaf.h"

#define TRACE_CALLS 1
#undef TRACE_CALLS

#include "outs_red.h"

#define CACHE_LINE_SIZE 64

#define CILKTOOL_API extern "C" __attribute__((visibility("default")))

CILKTOOL_API
int __cilkrts_is_initialized(void);
CILKTOOL_API
void __cilkrts_internal_set_nworkers(unsigned int nworkers);
CILKTOOL_API
unsigned __cilkrts_get_nworkers(void);

static unsigned worker_number() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  return __cilkrts_get_worker_number();
#pragma clang diagnostic pop
}


std::unique_ptr<std::ofstream> outf;
#ifndef OUTS_CERR
cilk::ostream_reducer<char> outs_red([]() -> std::basic_ostream<char>& {
            const char* envstr = getenv("CILKSCALE_OUT");
            if (envstr)
            return *(outf = std::make_unique<std::ofstream>(envstr));
            return std::cout;
            }());
#endif

class CilkiafImpl_t {
  std::mutex iaf_lock;
  BoundedIAF iaf;
  std::vector<BoundedIAF> local_iafs;


  // Need to manually register reducer
  //
  // > warning: reducer callbacks not implemented for structure members
  // > [-Wcilk-ignored]
  struct {
    template <class T>
    static void reducer_register(T& red) {
      __cilkrts_reducer_register(&red, sizeof(red),
          &std::decay_t<decltype(*&red)>::identity,
          &std::decay_t<decltype(*&red)>::reduce);
    }

    template <class T>
    static void reducer_unregister(T& red) {
      __cilkrts_reducer_unregister(&red);
    }

    struct RAII {
      CilkiafImpl_t& this_;

      RAII(decltype(this_) this_) : this_(this_) {
#ifndef OUTS_CERR
        reducer_register(outs_red);
#endif
        const char* envstr = getenv("CILKSCALE_OUT");
      }

      ~RAII() {
#ifndef OUTS_CERR
        reducer_unregister(outs_red);
#endif
      }
    } raii;
  } register_reducers = {.raii{*this}};

public:

  int read_maxcache()
  {
    const char* envstr = getenv("CILKIAF_CACHE");
    if (!envstr)
      return 65536;
    return atoi(envstr) ? atoi(envstr) : 65536;
  }

  
  CilkiafImpl_t() : iaf(4*read_maxcache(), read_maxcache())
         // Not only are reducer callbacks not implemented, the hyperobject
         // is not even default constructed unless explicitly constructed.
  {
    local_iafs.resize(__cilkrts_get_nworkers());
  }

  ~CilkiafImpl_t() {
    
    if (getenv("CILKIAF_PRINT"))
    {
      iaf.dump_success_function(outs_red, iaf.get_success_function(), 1);
      if (getenv("CILKIAF_PRINT") == "1")
        return;
      for (size_t i = 0; i < local_iafs.size(); i++) {
        local_iafs[i].dump_success_function(outs_red, local_iafs[i].get_success_function(), 1);
      }
    }
  }

  void register_write(uint64_t addr, int32_t num_bytes, source_loc_t store) {
    //outs_red << "[" << worker_number() << "] Writing" << std::endl;
    int32_t nbytes2 = num_bytes;
    uint64_t addr2 = addr;
    do {
      local_iafs.at(worker_number()).memory_access(addr2 / CACHE_LINE_SIZE);
      nbytes2 -= CACHE_LINE_SIZE;
      addr2 += CACHE_LINE_SIZE;
    } while(nbytes2 > 0);

    const std::lock_guard<std::mutex> lock(iaf_lock);

    do {
      iaf.memory_access(addr / CACHE_LINE_SIZE);
      num_bytes -= CACHE_LINE_SIZE;
      addr += CACHE_LINE_SIZE;
    } while(num_bytes > 0);
  }
};

static std::unique_ptr<CilkiafImpl_t> tool =
std::make_unique<decltype(tool)::element_type>();

CILKTOOL_API void __csi_init() {
  if (__cilkrts_is_initialized()) {
    __cilkrts_internal_set_nworkers(1);
  } else {
    /*
    // Force the number of Cilk workers to be 1.
    const char *e = getenv("CILK_NWORKERS");
    if (!e || 0 != strcmp(e, "1")) {
    if (setenv("CILK_NWORKERS", "1", 1)) {
    printf("Error setting CILK_NWORKERS to be 1\n");
    exit(1);
    }
    }
     */
  }
}

CILKTOOL_API void __csi_unit_init(const char* const file_name,
    const instrumentation_counts_t counts) {
}


CILKTOOL_API void __csi_before_load(const csi_id_t load_id, const void *addr,
    const int32_t num_bytes, const load_prop_t prop) {
#ifdef TRACE_CALLS
  outs_red
    << "[W" << worker_number() << "] before_load(lid=" << load_id << ", addr="
    << addr << ", nb=" << num_bytes << ", align=" << prop.alignment
    << ", vtab=" << prop.is_vtable_access << ", const=" << prop.is_constant
    << ", stack=" << prop.is_on_stack << ", cap=" << prop.may_be_captured
    << ", atomic=" << prop.is_atomic << ", threadlocal="
    << prop.is_thread_local << ", basic_read_before_write="
    << prop.is_read_before_write_in_bb << ")" << std::endl;
#endif
  if (prop.is_read_before_write_in_bb)
    return;
  auto store = __csi_get_load_source_loc(load_id);
  tool->register_write((uint64_t)addr, num_bytes, *store);
#ifdef TRACE_CALLS
  outs_red << "LOAD ON (" << store->name << ", " << store->line_number << ")" << std::endl;
#endif
}

CILKTOOL_API void __csi_after_load(const csi_id_t load_id, const void *addr,
    const int32_t num_bytes, const load_prop_t prop) {
#ifdef TRACE_CALLS
  outs_red
    << "[W" << worker_number() << "] after_load(lid=" << load_id << ", addr="
    << addr << ", nb=" << num_bytes << ", align=" << prop.alignment
    << ", vtab=" << prop.is_vtable_access << ", const=" << prop.is_constant
    << ", stack=" << prop.is_on_stack << ", cap=" << prop.may_be_captured
    << ", atomic=" << prop.is_atomic << ", threadlocal="
    << prop.is_thread_local << ", basic_read_before_write="
    << prop.is_read_before_write_in_bb << ")" << std::endl;
#endif
}

CILKTOOL_API void __csi_before_store(const csi_id_t store_id, const void *addr,
    const int32_t num_bytes, const store_prop_t prop) {
#ifdef TRACE_CALLS
  outs_red
    << "[W" << worker_number() << "] before_store(sid=" << store_id
    << ", addr=" << addr << ", nb=" << num_bytes << ", align="
    << prop.alignment << ", vtab=" << prop.is_vtable_access << ", const="
    << prop.is_constant << ", stack=" << prop.is_on_stack << ", cap="
    << prop.may_be_captured << ", atomic=" << prop.is_atomic
    << ", threadlocal=" << prop.is_thread_local << ")" << std::endl;
#endif
  auto store = __csi_get_store_source_loc(store_id);
  tool->register_write((uint64_t)addr, num_bytes, *store);
#ifdef TRACE_CALLS
  outs_red << "WRITE ON (" << store->name << ", " << store->line_number << ")" << std::endl;
#endif
}

CILKTOOL_API void __csi_after_store(const csi_id_t store_id, const void *addr,
    const int32_t num_bytes, const store_prop_t prop) {
#ifdef TRACE_CALLS
  outs_red
    << "[W" << worker_number() << "] after_store(sid=" << store_id
    << ", addr=" << addr << ", nb=" << num_bytes << ", align="
    << prop.alignment << ", vtab=" << prop.is_vtable_access << ", const="
    << prop.is_constant << ", stack=" << prop.is_on_stack << ", cap="
    << prop.may_be_captured << ", atomic=" << prop.is_atomic
    << ", threadlocal=" << prop.is_thread_local << ")" << std::endl;
#endif
}

