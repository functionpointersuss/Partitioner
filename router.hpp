#include <mtkahypar.h>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <map>
#include <vector>
#include <queue>
#include <limits>
#include <iostream>
#include "hypergraph.hpp"

void route(std::vector<std::vector<int32_t>> fpga_delay_graph, std::vector<std::vector<int32_t>> fpga_band_graph, std::vector<std::vector<int32_t>> fpga_hop_graph,
                   std::vector<hypergraph_edge_t> edge_partition_list);
std::vector<int32_t> route_path(const std::vector<std::vector<int32_t>>& fpga_delay_graph, int32_t num_fpgas, int32_t source_node, int32_t drain_node, int32_t maximum_hop_count);
std::pair<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> create_fpga_graph_luts (int32_t num_fpgas,
                                                                                                          std::vector<std::vector<int32_t>> fpga_delay_graph,
                                                                                                          std::vector<std::vector<int32_t>> fpga_hop_graph);
