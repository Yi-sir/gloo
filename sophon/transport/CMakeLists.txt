set(SOPHON_TRANSPORT_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/address.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/buffer.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/context.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/device.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/pair.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/unbound_buffer.cc"
  )

set(SOPHON_TRANSPORT_HDRS
  "${CMAKE_CURRENT_SOURCE_DIR}/address.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/buffer.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/context.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/device.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/pair.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/unbound_buffer.h"
  )

if(SOPHON_HAVE_TRANSPORT_TCP)
  add_subdirectory(tcp)
endif()

if(SOPHON_HAVE_TRANSPORT_IBVERBS)
  add_subdirectory(ibverbs)
endif()

if(SOPHON_HAVE_TRANSPORT_UV)
  add_subdirectory(uv)
endif()

if(SOPHON_HAVE_TRANSPORT_SOPHGO)
  add_subdirectory(sophgo)
endif()

list(APPEND SOPHON_SRCS ${SOPHON_TRANSPORT_SRCS})
list(APPEND SOPHON_HDRS ${SOPHON_TRANSPORT_HDRS})
set(SOPHON_SRCS ${SOPHON_SRCS} PARENT_SCOPE)
set(SOPHON_HDRS ${SOPHON_HDRS} PARENT_SCOPE)
