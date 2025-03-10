#include "cilkiaf.h"

std::unique_ptr<CilkiafImpl_t> tool = std::make_unique<CilkiafImpl_t>();

std::unique_ptr<std::ofstream> outf;

#ifndef OUTS_CERR
cilk::ostream_reducer<char> outs_red([]() -> std::basic_ostream<char>& {
            const char* envstr = getenv("CILKSCALE_OUT");
            if (envstr)
            return *(outf = std::make_unique<std::ofstream>(envstr));
            return std::cout;
            }());
#endif


constexpr short sampling_log2 = 10;
constexpr size_t seed = 98721893579823;


CilkiafImpl_t::CilkiafImpl_t() 
#ifdef CILKIAF_GLOBAL
: iaf(sampling_log2, seed, 65536, read_maxcache())
#endif
{
  uint64_t maxcache = read_maxcache();

  if (__cilkrts_is_initialized()) {
    local_iafs.reserve(__cilkrts_get_nworkers());
    for (size_t i = 0; i < __cilkrts_get_nworkers(); i++)
      local_iafs.emplace_back(sampling_log2, seed, 65536, maxcache);
#ifdef IAF_VERIFY
    local_verify_iafs.reserve(__cilkrts_get_nworkers());
    for (size_t i = 0; i < __cilkrts_get_nworkers(); i++)
      local_verify_iafs.emplace_back(0, seed, 65536, maxcache);
#endif

  } else {
    assert(false);
  }
}

CilkiafImpl_t::~CilkiafImpl_t() {
#ifdef IAF_VERIFY
  for (size_t i = 0; i < local_iafs.size(); i++) {
    outs_red << "sampled " << i << std::endl;
    local_iafs[i].csv_success_function(outs_red, local_iafs[i].get_success_function(), 1);
    outs_red << "verify " << i << std::endl;
    local_verify_iafs[i].csv_success_function(outs_red, local_verify_iafs[i].get_success_function(), 1);
  }
  return;
#endif

  if (getenv("CILKIAF_PRINT"))
  {
#ifdef CILKIAF_GLOBAL
    iaf.csv_success_function(outs_red, iaf.get_success_function(), 1);
#endif
    if (atoi(getenv("CILKIAF_PRINT")) == 1)
      return;
    for (size_t i = 0; i < local_iafs.size(); i++) {
      local_iafs[i].csv_success_function(outs_red, local_iafs[i].get_success_function(), 1);
    }
  }
}

void CilkiafImpl_t::register_write(uint64_t addr, int32_t num_bytes) {
#ifdef TRACE_CALLS
  outs_red << "[" << worker_number() << "] Writing" << std::endl;
#endif
  int32_t nbytes2 = num_bytes;
  uint64_t addr2 = addr;
  do {
    local_iafs[worker_number()].memory_access(addr2 / CACHE_LINE_SIZE);
#ifdef IAF_VERIFY
    local_verify_iafs[worker_number()].memory_access(addr2 / CACHE_LINE_SIZE);
#endif
    nbytes2 -= CACHE_LINE_SIZE;
    addr2 += CACHE_LINE_SIZE;
  } while(nbytes2 > 0);
#ifdef CILKIAF_GLOBAL
  const std::lock_guard<std::mutex> lock(iaf_lock);

  do {
    iaf.memory_access(addr / CACHE_LINE_SIZE);
    num_bytes -= CACHE_LINE_SIZE;
    addr += CACHE_LINE_SIZE;
  } while(num_bytes > 0);
#endif
}


void CilkiafImpl_t::register_write_one(uint64_t addr) {
#ifdef TRACE_CALLS
  outs_red << "[" << worker_number() << "] Writing One" << std::endl;
#endif

  local_iafs[worker_number()].memory_access(addr / CACHE_LINE_SIZE);
#ifdef IAF_VERIFY
  local_verify_iafs[worker_number()].memory_access(addr / CACHE_LINE_SIZE);
#endif

#ifdef CILKIAF_GLOBAL
  const std::lock_guard<std::mutex> lock(iaf_lock);
  iaf.memory_access(addr / CACHE_LINE_SIZE);
#endif
}


