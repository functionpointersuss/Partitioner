#include <iostream>
#include <vector>
#include "partitioner.hpp"

int main (int argc, char *argv[]) {
  if (argc < 4) {
    std::cout << "./partitioner fpga_graph_file netlist_graph_file fpga_size\n";
    return 1;
  }
  std::string fpga_graph_file(argv[1]);
  char *netlist_graph_file = argv[2];
  int32_t fpga_size;
  try {
    fpga_size = std::stoi(argv[3]);
  }
  catch(const std::invalid_argument& e) {
    std::cerr << "FPGA Size invalid number\n";
    return 1;
  }
  catch(const std::out_of_range& e) {
    std::cerr << "FPGA Size Out of Range\n";
    return 1;
  }

  partition(fpga_graph_file, netlist_graph_file, fpga_size);
  return 0;
}
