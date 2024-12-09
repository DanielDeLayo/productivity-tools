# Build for the Cilkiaf runtime support library.

set(CILKIAF_SOURCES
  cilkiaf.cpp
  csirt.cpp
  bounded_iaf.cpp
  increment_and_freeze.cpp
  projection.cpp
  )

include_directories(${CILKTOOLS_SOURCE_DIR}/include)

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

# Build Cilkiaf runtimes shipped with Clang.
add_cilktools_component(cilkiaf)

if (APPLE)
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
  foreach (arch ${CILKIAF_SUPPORTED_ARCH})
    add_cilktools_runtime(clang_rt.cilkiaf
      STATIC
      ARCHS ${arch}
      SOURCES ${CILKIAF_SOURCES}
      CFLAGS ${CILKIAF_CFLAGS}
      DEFS ${CILKIAF_COMMON_DEFINITIONS}
      PARENT_TARGET cilkiaf)

    add_cilktools_runtime(clang_rt.cilkiaf
      SHARED
      ARCHS ${arch}
      SOURCES ${CILKIAF_SOURCES}
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
