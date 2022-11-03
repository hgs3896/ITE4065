# Sources
set(BWTREE_SOURCE_DIR src)
set(BWTREE_SOURCES
  ${BWTREE_SOURCE_DIR}/bwtree.cc
  # Add your sources here
  # ${BWTREE_SOURCE_DIR}/foo/bar/your_source.cc
  )

# Headers
set(BWTREE_HEADER_DIR include)
set(BWTREE_HEADERS
  ${BWTREE_HEADER_DIR}/atomic_stack.h
  ${BWTREE_HEADER_DIR}/bloom_filter.h
  ${BWTREE_HEADER_DIR}/bwtree.h
  ${BWTREE_HEADER_DIR}/bwtree_test_util.h
  ${BWTREE_HEADER_DIR}/index_logger.h
  ${BWTREE_HEADER_DIR}/macros.h
  ${BWTREE_HEADER_DIR}/multithread_test_util.h
  ${BWTREE_HEADER_DIR}/sorted_small_set.h
  ${BWTREE_HEADER_DIR}/timer.h
  ${BWTREE_HEADER_DIR}/worker_pool.h
  ${BWTREE_HEADER_DIR}/zipf.h
  # Add your headers here
  # ${BWTREE_HEADER_DIR}/foo/bar/your_header.h
  )

add_library(bwtree STATIC ${BWTREE_HEADERS} ${BWTREE_SOURCES})

target_include_directories(bwtree
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${BWTREE_HEADER_DIR}"
  )

