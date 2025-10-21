#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream> 
#include "types.h"

std::string trim(const std::string& str);
Evidence parseEvidenceString(const std::string& evidence_str);
BayesianNetwork parseBIF(const std::string& filename);

#endif