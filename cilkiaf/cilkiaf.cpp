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


CilkiafImpl_t::CilkiafImpl_t() 
#ifdef CILKIAF_GLOBAL
: iaf(65536, read_maxcache())
#endif
{
  uint64_t maxcache = read_maxcache();

  if (__cilkrts_is_initialized()) {
    local_iafs.reserve(__cilkrts_get_nworkers());
    for (size_t i = 0; i < __cilkrts_get_nworkers(); i++)
      local_iafs.emplace_back(65536, maxcache);

  } else {
    assert(false);
  }
}

CilkiafImpl_t::~CilkiafImpl_t() {

  if (getenv("CILKIAF_PRINT"))
  {
#ifdef CILKIAF_GLOBAL
    iaf.dump_success_function(outs_red, iaf.get_success_function(), 1);
#endif
    if (atoi(getenv("CILKIAF_PRINT")) == 1)
      return;
    for (size_t i = 0; i < local_iafs.size(); i++) {
      local_iafs[i].dump_success_function(outs_red, local_iafs[i].get_success_function(), 1);
    }
  }
}

std::mutex test_lock;

inline void locktest()
{
  const std::lock_guard<std::mutex> lock(test_lock);
  
} 

void CilkiafImpl_t::register_write(uint64_t addr, int32_t num_bytes) {
#ifdef TRACE_CALLS
  outs_red << "[" << worker_number() << "] Writing" << std::endl;
#endif
  locktest();
  return;
  int32_t nbytes2 = num_bytes;
  uint64_t addr2 = addr;
  do {
    local_iafs[worker_number()].memory_access(addr2 / CACHE_LINE_SIZE);
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
  locktest();
  return;

  local_iafs[worker_number()].memory_access(addr / CACHE_LINE_SIZE);

#ifdef CILKIAF_GLOBAL
  const std::lock_guard<std::mutex> lock(iaf_lock);
  iaf.memory_access(addr / CACHE_LINE_SIZE);
#endif
}


