#include <iostream>
#include <vector>
#include "mtkahypar.hpp"
#include "router.hpp"
#include "fpgagraph.hpp"
#include "hypergraph.hpp"

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

  // Only partition to a subset of FPGAs, based on the design size
  int num_fpgas;

  std::vector<hypergraph_edge_t> edge_partition_list = partition(netlist_graph_file, fpga_size, num_fpgas);

  // Create FPGA Graph
  auto [fpga_delay_graph, fpga_band_graph, fpga_hop_graph] = fpga_graph_from_file(fpga_graph_file, num_fpgas);



  // Route Hypergraph
  std::cout << "********************************************************************************\n";
  std::cout << "*                             System Routing...                                *\n";
  std::cout << "********************************************************************************\n";

  route(fpga_delay_graph, fpga_band_graph, fpga_hop_graph, edge_partition_list);

  return 0;
}
