#pragma once
#include <cassert>
#include <memory>
#include <vector>
#include <thread>
#include <iostream>
#include <mtkahypar.h>

#include "hypergraph.hpp"

void partition(const std::string &fpga_graph_file, char* netlist_graph_file, const int32_t fpga_size);
