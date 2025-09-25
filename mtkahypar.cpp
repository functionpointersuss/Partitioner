#include "partitioner.hpp"

std::vector< partition(const std::string &fpga_graph_file, char* netlist_graph_file, const int32_t num_fpgas) {
  mt_kahypar_error_t error{};

  // Initialize
  mt_kahypar_initialize(
    std::thread::hardware_concurrency() /* use all available cores */,
    true /* activate interleaved NUMA allocation policy */ );


  // Setup partitioning context
  mt_kahypar_context_t* context = mt_kahypar_context_from_preset(DEFAULT);
  // In the following, we partition a hypergraph into two blocks
  // with an allowed imbalance of 3% and optimize the connective metric (KM1)
  mt_kahypar_set_seed(42 /* seed */);
  // Enable logging
  mt_kahypar_status_t status =
    mt_kahypar_set_context_parameter(context, VERBOSE, "1", &error);
  assert(status == SUCCESS);

  // Load Hypergraph for DEFAULT preset
  mt_kahypar_hypergraph_t hypergraph =
    mt_kahypar_read_hypergraph_from_file(netlist_graph_file,
      context, HMETIS /* file format */, &error);
  if (hypergraph.hypergraph == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  mt_kahypar_set_partitioning_parameters(context,
    num_fpgas /* number of blocks */, 0.10 /* imbalance parameter */,
    KM1 /* objective function */);

  // Partition Hypergraph
  mt_kahypar_partitioned_hypergraph_t partitioned_hg =
    mt_kahypar_partition(hypergraph, context, &error);
  if (partitioned_hg.partitioned_hg == nullptr) {
    std::cout << error.msg << std::endl; std::exit(1);
  }

  // Process hypergraph
  // We want to convert the format given by mt kahypar into a standard format
  std::vector<hypergraph_edge_t> edge_partitions;
  std::vector<uint64_t> drain_partitions;
  for (int64_t edge = 0; edge < mt_kahypar_num_hyperedges(hypergraph); edge++) {
    mt_kahypar_hypernode_id_t num_nodes_in_edge = mt_kahypar_hyperedge_size(hypergraph, edge);
    mt_kahypar_hypernode_id_t *pin_buffer = (mt_kahypar_hypernode_id_t*) malloc(sizeof(mt_kahypar_hypernode_id_t)*num_nodes_in_edge);
    mt_kahypar_partition_id_t source_part;

    mt_kahypar_get_hyperedge_pins(hypergraph, edge, pin_buffer);
    source_part = mt_kahypar_block_id(partitioned_hypergraph, pin_buffer[0]);

    drain_partitions.insert(edge_partitions.end(), pin_buffer+1, pin_buffer+num_nodes_in_edge);
    edge_partitions.push_back({.source = source_part, .drains = std::move(drain_partitions)});
  }

  mt_kahypar_free_context(context);
  mt_kahypar_free_hypergraph(hypergraph);
  mt_kahypar_free_partitioned_hypergraph(partitioned_hg);

  return edge_partitions;
}
