#pragma once
#include <cassert>
#include <memory>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>

#include <mtkahypar.h>

#include "router.hpp"

void partition(const std::string &fpga_graph_file, char* netlist_graph_file);
std::pair<std::vector<std::vector<int32_t>>, std::vector<std::vector<int32_t>>> fpga_graph_from_file(const std::string &filename);
