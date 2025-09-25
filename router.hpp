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
#include "hypergraph.hpp"

void route(std::vector<std::vector<int32_t>> fpga_graph, std::vector<std::vector<int32_t>> fpga_band_graph, std::vector<hypergraph_edge_t> edge_partition_list);
std::vector<int32_t> route_path(int32_t source_node, int32_t target_node, int32_t maximum_hop_count);
