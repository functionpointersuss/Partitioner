#include <iostream>
#include <vector>
#include "partitioner.hpp"
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
  int num_gates = mt_kahypar_num_hyperedges(hypergraph);
  int num_fpgas = (num_gates+fpga_size-1) / fpga_size;

  std::cout << "Design Size: " << num_gates << '\n';
  std::cout << "FPGA Size: " << fpga_size << '\n';
  std::cout << "Num FPGAs Used : " << num_fpgas << '\n';

  std::cout << num_fpgas << std::endl;
  std::cout << netlist_graph_file << std::endl;

  // Create FPGA Graph
  auto [fpga_delay_graph, fpga_band_graph, fpga_hop_graph] = fpga_graph_from_file(fpga_graph_file, num_fpgas);

  partition(fpga_graph_file, netlist_graph_file, fpga_size);


  // Route Hypergraph
  std::cout << "********************************************************************************\n";
  std::cout << "*                             System Routing...                                *\n";
  std::cout << "********************************************************************************\n";

  router(fpga_delay_graph, fpga_band_graph, hypergraph, partitioned_hg);
  router.route();



  return 0;
}
