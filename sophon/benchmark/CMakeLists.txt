set(SOPHON_BENCHMARK_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/main.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/options.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/runner.cc"
  )

add_executable(benchmark ${SOPHON_BENCHMARK_SRCS})
target_link_libraries(benchmark sophon)

if(SOPHON_INSTALL)
  install(TARGETS benchmark DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif()

if(USE_CUDA)
  set(SOPHON_BENCHMARK_CUDA_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/cuda_main.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/options.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/runner.cc"
    )

  add_executable(benchmark_cuda ${SOPHON_BENCHMARK_CUDA_SRCS})
  target_link_libraries(benchmark_cuda sophon_cuda ${SOPHON_CUDA_LIBRARIES})

  if(SOPHON_INSTALL)
    install(TARGETS benchmark_cuda DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  endif()
endif()

if(USE_ROCM)
  set(SOPHON_BENCHMARK_HIP_SRCS
    "${HIPIFY_OUTPUT_DIR}/benchmark/hip_main.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/options.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/runner.cc"
    )

  sophon_hip_add_executable(benchmark_hip ${SOPHON_BENCHMARK_HIP_SRCS})
  target_link_libraries(benchmark_hip sophon_hip)

  if(SOPHON_INSTALL)
    install(TARGETS benchmark_hip DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
  endif()
endif()
