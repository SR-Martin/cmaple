enable_testing()

file(COPY "${PROJECT_SOURCE_DIR}/example" DESTINATION "${PROJECT_BINARY_DIR}")

add_executable(
  cmaple_maintest EXCLUDE_FROM_ALL         ## todo: should be called 'SeqRegions_test'
  cmaple_test.cpp
  phylonode_test.cpp
  seqregions_test.cpp
  model_test.cpp
  alignment_test.cpp
  matrix_test.cpp
  ratevariation_test.cpp
  sequence_test.cpp
  seqregion_test.cpp
  mutation_test.cpp
)
target_link_libraries(
  cmaple_maintest
  GTest::gtest_main    ## contains the main() function. No need to write one yourself
  cmaple_utils
  ncl nclextra
  cmaple_model
  cmaple_alignment
  cmaple_tree
  maple
)

include(GoogleTest)
gtest_discover_tests(cmaple_maintest)
