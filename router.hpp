#pragma once
#include <mtkahypar.h>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>
#include <queue>
#include <limits>
#include <iostream>

class router {
public:
  router(std::vector<std::vector<int32_t>> fpga_graph,
         mt_kahypar_hypergraph_t& hypergraph, mt_kahypar_partitioned_hypergraph_t& partitioned_hypergraph);
  void route();

private:
  int num_fpgas;
  std::vector<std::vector<int32_t>> fpga_graph;
  std::vector<std::vector<int32_t>> fpga_route_graph;
  mt_kahypar_hypergraph_t& hypergraph;
  mt_kahypar_partitioned_hypergraph_t& partitioned_hypergraph;

  std::vector<int32_t> route_path(int32_t source_node, int32_t target_node, int32_t maximum_hop_count);
};

typedef struct edge_path {
  int32_t length;
  mt_kahypar_partition_id_t source_part;
  mt_kahypar_partition_id_t drain_part;
} edge_path_t;
