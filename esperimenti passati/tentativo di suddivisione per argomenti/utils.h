#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <map>
#include <sstream> 
#include "types.h"

Evidence parseEvidenceString(const std::string& evidence_str);
std::string getValueString(const Variable& var, int index);

#endif