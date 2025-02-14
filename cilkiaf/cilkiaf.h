#pragma once

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

unsigned inline worker_number() {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  return __cilkrts_get_worker_number();
#pragma clang diagnostic pop
}


class CilkiafImpl_t {
  std::mutex iaf_lock;
#ifdef CILKIAF_GLOBAL
  BoundedIAF iaf;
#endif
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
  
  CilkiafImpl_t();
  ~CilkiafImpl_t();
  void register_write(uint64_t addr, int32_t num_bytes);
  void register_write_one(uint64_t addr);
};

