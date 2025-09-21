cmake_minimum_required(VERSION 4.1.1)

project (MaPart CXX)

find_package(MtKaHyPar)
if(MtKaHyPar_FOUND)
  message("Linked library")
  add_executable(partitioner partitioner.cpp router.cpp)
  target_link_libraries(partitioner MtKaHyPar::mtkahypar)
endif()
