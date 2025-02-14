#include "cilkiaf.h"

extern std::unique_ptr<CilkiafImpl_t> tool;

CILKTOOL_API void __csi_init() {
  if (__cilkrts_is_initialized()) {
    //__cilkrts_internal_set_nworkers(1);
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
  //if (prop.is_read_before_write_in_bb)
  //  return;
  if (num_bytes <= CACHE_LINE_SIZE)
    tool->register_write_one((uint64_t)addr);
  else
    tool->register_write((uint64_t)addr, num_bytes);
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
  if (num_bytes <= CACHE_LINE_SIZE)
    tool->register_write_one((uint64_t)addr);
  else
    tool->register_write((uint64_t)addr, num_bytes);
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

