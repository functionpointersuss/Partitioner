#pragma once
#include <cassert>
#include <memory>
#include <vector>
#include <thread>
#include <iostream>
#include <mtkahypar.h>

#include "hypergraph.hpp"

std::vector<hypergraph_edge_t> partition(char* netlist_graph_file, const int32_t fpga_size, int32_t& num_fpgas);
