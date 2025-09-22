#include <iostream>
#include <vector>
#include "partitioner.hpp"

int main (int argc, char *argv[]) {

  if (argc < 3) {
    std::cout << "./partitioner fpga_graph_file netlist_graph_file\n";
    return -1;
  }
  std::string fpga_graph_file(argv[1]);
  char *netlist_graph_file = argv[2];

  partition(fpga_graph_file, netlist_graph_file);

  return 0;
}
