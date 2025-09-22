cmake_minimum_required(VERSION 3.1.1)

project (MaPart CXX)

find_package(MtKaHyPar)
if(MtKaHyPar_FOUND)
  message("Linked library")
  add_executable(partitioner main.cpp partitioner.cpp router.cpp)
  target_link_libraries(partitioner MtKaHyPar::mtkahypar)

  add_executable(fpga_graph_test tests/fpga_graph_test.cpp partitioner.cpp router.cpp)
  target_link_libraries(fpga_graph_test MtKaHyPar::mtkahypar)

  enable_testing()

  add_test(
  NAME fpga_graph_test
  COMMAND $<TARGET_FILE:fpga_graph_test>
  )
endif()
