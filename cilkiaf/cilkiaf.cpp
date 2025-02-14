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


CilkiafImpl_t::CilkiafImpl_t() : iaf(4*read_maxcache(), read_maxcache()), local_iafs(__cilkrts_get_nworkers(), iaf)
                                 // Not only are reducer callbacks not implemented, the hyperobject
                                 // is not even default constructed unless explicitly constructed.
{
  if (__cilkrts_is_initialized()) {
    //local_iafs.reserve(__cilkrts_get_nworkers());

    //local_iafs.resize(__cilkrts_get_nworkers());
  } else {
    assert(false);
  }
}

CilkiafImpl_t::~CilkiafImpl_t() {

  if (getenv("CILKIAF_PRINT"))
  {
    //outs_red << &local_iafs << "!" << std::endl;
    //outs_red << &iaf << "!" << std::endl;
    iaf.dump_success_function(outs_red, iaf.get_success_function(), 1);
    if (atoi(getenv("CILKIAF_PRINT")) == 1)
      return;
    for (size_t i = 0; i < local_iafs.size(); i++) {
      local_iafs[i].dump_success_function(outs_red, local_iafs[i].get_success_function(), 1);
    }
  }
}

void CilkiafImpl_t::register_write(uint64_t addr, int32_t num_bytes, source_loc_t store) {
#ifdef TRACE_CALLS
  //outs_red << "[" << worker_number() << "] Writing" << std::endl;
#endif
  int32_t nbytes2 = num_bytes;
  uint64_t addr2 = addr;
  do {
    //outs_red << &local_iafs << "!" << std::endl;
    //outs_red << this << "!!" << std::endl;
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


