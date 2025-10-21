#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream> 
#include "types.h"

std::string getValueString(const Variable& var, int index);
double getConditionalProbabilityFromCPT(const Variable& target_var, const std::vector<int>& config_vector_ancestors, int target_value_idx, const BayesianNetwork& bn);
std::map<std::string, std::map<std::string, double>> calculateProbabilitiesWithEvidence(const BayesianNetwork& reordered_bn, const Evidence& evidence);



#endif