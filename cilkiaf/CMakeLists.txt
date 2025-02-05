# Build for the Cilkiaf runtime support library.

set(CILKIAF_SOURCES
  cilkiaf.cpp
  csirt.cpp
  Increment-and-Freeze/bounded_iaf.cc
  Increment-and-Freeze/increment_and_freeze.cc
  Increment-and-Freeze/projection.cc
  )

set(CILKIAF_BITCODE_SOURCE
  cilkiaf.cpp
  Increment-and-Freeze/bounded_iaf.cc
  Increment-and-Freeze/increment_and_freeze.cc
  Increment-and-Freeze/projection.cc
  )

include_directories(${CILKTOOLS_SOURCE_DIR}/include Increment-and-Freeze)

set(CILKIAF_CFLAGS ${SANITIZER_COMMON_CFLAGS})
append_list_if(CILKTOOLS_HAS_CILK -fopencilk CILKIAF_CFLAGS)
append_list_if(CILKTOOLS_HAS_FDEBUG_DEFAULT_VERSION_EQ_4_FLAG
  -fdebug-default-version=4 CILKIAF_CFLAGS)
append_rtti_flag(OFF CILKIAF_CFLAGS)

set(CILKIAF_COMMON_DEFINITIONS)
append_list_if(CILKTOOLS_HAS_CILK SERIAL_TOOL=0 CILKIAF_COMMON_DEFINITIONS)

set(CILKIAF_DYNAMIC_LINK_FLAGS)
append_list_if(CILKTOOLS_HAS_CILK -fopencilk CILKIAF_DYNAMIC_LINK_FLAGS)

set(CILKIAF_DYNAMIC_CFLAGS ${CILKIAF_CFLAGS})
set(CILKIAF_DYNAMIC_DEFINITIONS ${CILKIAF_COMMON_DEFINITIONS})

set(CILKIAF_COMMON_LIBS ${SANITIZER_CXX_ABI_LIBRARY} ${SANITIZER_COMMON_LINK_LIBS})

set(CILKIAF_DYNAMIC_LIBS ${CILKIAF_COMMON_LIBS})

# Setup flags and defs for cilktool bitcode build
set(CILKIAF_BITCODE_CFLAGS ${CILKIAF_CFLAGS} -emit-llvm)
set(CILKIAF_BITCODE_DEFINITIONS ${CILKIAF_COMMON_DEFINITIONS}
    "CILKIAF_VIS=__attribute__((noinline))")

# Build Cilkiaf runtimes shipped with Clang.
add_cilktools_component(cilkiaf)

if (APPLE)
  #TODO: Fix the apple section
    add_cilktools_runtime(clang_rt.cilkiaf
      STATIC
      OS ${CILKTOOL_SUPPORTED_OS}
      ARCHS ${CILKIAF_SUPPORTED_ARCH}
      SOURCES ${CILKIAF_SOURCES}
      CFLAGS ${CILKIAF_CFLAGS}
      DEFS ${CILKIAF_COMMON_DEFINITIONS}
      PARENT_TARGET cilkiaf)

    add_cilktools_runtime(clang_rt.cilkiaf
      SHARED
      OS ${CILKTOOL_SUPPORTED_OS}
      ARCHS ${CILKIAF_SUPPORTED_ARCH}
      SOURCES ${CILKIAF_SOURCES}
      CFLAGS ${CILKIAF_DYNAMIC_CFLAGS}
      LINK_FLAGS ${CILKIAF_DYNAMIC_LINK_FLAGS}
      LINK_LIBS ${CILKIAF_DYNAMIC_LIBS}
      DEFS ${CILKIAF_DYNAMIC_DEFINITIONS}
      PARENT_TARGET cilkiaf)

else()
  # Compile Cilkiaf sources into an object library
  add_cilktools_object_libraries(RTCilkiaf_dynamic
    ARCHS ${CILKIAF_SUPPORTED_ARCH}
    SOURCES ${CILKIAF_SOURCES}
    CFLAGS ${CILKIAF_DYNAMIC_CFLAGS}
    DEFS ${CILKIAF_DYNAMIC_DEFINITIONS}
    DEPS ${CILKIAF_OBJ_DEPS})

  add_cilktools_object_libraries(RTCilkiaf
    ARCHS ${CILKIAF_SUPPORTED_ARCH}
    SOURCES ${CILKIAF_SOURCES}
    CFLAGS ${CILKIAF_CFLAGS}
    DEFS ${CILKIAF_COMMON_DEFINITIONS}
    DEPS ${CILKIAF_OBJ_DEPS})

  foreach (arch ${CILKIAF_SUPPORTED_ARCH})
    add_cilktools_bitcode(cilkiaf
      ARCHS ${arch}
      SOURCES ${CILKIAF_BITCODE_SOURCE}
      CFLAGS ${CILKIAF_BITCODE_CFLAGS}
      DEFS ${CILKIAF_BITCODE_DEFINITIONS}
      PARENT_TARGET cilkiaf)

    add_cilktools_runtime(clang_rt.cilkiaf
      STATIC
      ARCHS ${arch}
      OBJECT_LIBS RTCilkiaf
      CFLAGS ${CILKIAF_CFLAGS}
      LINK_FLAGS ${CILKIAF_STATIC_LINK_FLAGS}
      DEFS ${CILKIAF_COMMON_DEFINITIONS}
      PARENT_TARGET cilkiaf)

    add_cilktools_runtime(clang_rt.cilkiaf
      SHARED
      ARCHS ${arch}
      OBJECT_LIBS RTCilkiaf_dynamic
      CFLAGS ${CILKIAF_DYNAMIC_CFLAGS}
      LINK_FLAGS ${CILKIAF_DYNAMIC_LINK_FLAGS}
      LINK_LIBS ${CILKIAF_DYNAMIC_LIBS}
      DEFS ${CILKIAF_DYNAMIC_DEFINITIONS}
      PARENT_TARGET cilkiaf)
  endforeach()
endif()

if (CILKTOOLS_INCLUDE_TESTS)
  # TOdo: Add tests
endif()
