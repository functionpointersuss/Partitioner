#include "partitioner.hpp"
// Install library interface via 'sudo make install.mtkahypar' in build folder
// Compile with: g++ -std=c++14 -DNDEBUG -O3 partition_hypergraph.cc -o example -lmtkahypar
int main(int argc, char* argv[]) {
  mt_kahypar_error_t error{};

  // Create FPGA Graph
  // Example: Basic 2D mesh connection of 4 fpgas
  std::vector<std::vector<int32_t>> fpga_graph = {
    { 0,  1,  1, -1},
    { 1,  0, -1,  1},
    { 1, -1,  0,  1},
    {-1,  1,  1,  0}
  };
  int32_t num_fpgas = fpga_graph.size();

  // Initialize
  mt_kahypar_initialize(
    std::thread::hardware_concurrency() /* use all available cores */,
    true /* activate interleaved NUMA allocation policy */ );


  // Setup partitioning context
  mt_kahypar_context_t* context = mt_kahypar_context_from_preset(DEFAULT);
  // In the following, we partition a hypergraph into two blocks
  // with an allowed imbalance of 3% and optimize the connective metric (KM1)
  mt_kahypar_set_partitioning_parameters(context,
    num_fpgas /* number of blocks */, 0.10 /* imbalance parameter */,
    KM1 /* objective function */);
  mt_kahypar_set_seed(42 /* seed */);
  // Enable logging
  mt_kahypar_status_t status =
    mt_kahypar_set_context_parameter(context, VERBOSE, "1", &error);
  assert(status == SUCCESS);

  // Load Hypergraph for DEFAULT preset
  mt_kahypar_hypergraph_t hypergraph =
    mt_kahypar_read_hypergraph_from_file("basic.hgr",
      context, HMETIS /* file format */, &error);
  if (hypergraph.hypergraph == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  // Partition Hypergraph
  mt_kahypar_partitioned_hypergraph_t partitioned_hg =
    mt_kahypar_partition(hypergraph, context, &error);
  if (partitioned_hg.partitioned_hg == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  // Route Hypergraph
  router router(num_fpgas, fpga_graph, hypergraph, partitioned_hg);
  router.route();

  mt_kahypar_free_context(context);
  mt_kahypar_free_hypergraph(hypergraph);
  mt_kahypar_free_partitioned_hypergraph(partitioned_hg);
}

