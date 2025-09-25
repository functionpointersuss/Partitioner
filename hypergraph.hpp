#pragma once

typedef struct {
  uint64_t source;
  std::vector<uint64_t> drains;
} hypergraph_edge_t;

typedef struct {
  uint64_t dist;
  uint64_t effort;
  uint64_t source;
  uint64_t drain;
} routable_edge_t;
