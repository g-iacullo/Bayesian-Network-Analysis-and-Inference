#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <map>
#include <sstream> 

// Define a structure for a variable
struct Variable {
    std::string name;
    std::vector<std::string> values;
    std::vector<std::string> parents;
    std::vector<std::vector<double>> cpt;
    int id;
};

// A simple structure to hold the entire Bayesian Network
struct BayesianNetwork {
    std::map<std::string, Variable> variables;
    std::vector<std::vector<int>> adj;
    std::map<std::string, int> name_to_id;
    std::map<int, std::string> id_to_name;
    int next_id = 0;
};

// Type alias for evidence
using Evidence = std::map<std::string, std::string>;


#endif