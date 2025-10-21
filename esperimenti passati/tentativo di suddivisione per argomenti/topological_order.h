#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream> 
#include "types.h"

void dfs_topological_sort_helper(int u, const BayesianNetwork &bn,
                                 std::vector<bool> &visited,
                                 std::vector<bool> &recursion_stack, // Aggiunto per rilevamento cicli
                                 std::vector<int> &result);
std::vector<int> topological_sort(const BayesianNetwork& bn);
BayesianNetwork reorder_network_topologically(const BayesianNetwork& original_bn, const std::vector<int>& topological_order);


#endif