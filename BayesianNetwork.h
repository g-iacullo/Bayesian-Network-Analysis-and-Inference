
#ifndef BAYESIAN_NETWORK_H
#define BAYESIAN_NETWORK_H

#include <string>
#include <vector>
#include <map>
#include <sstream> // Necessario per parseEvidenceString e forse trim

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

// --- Funzioni Utility (spostate qui da Utils.h) ---
std::string trim(const std::string& str);
Evidence parseEvidenceString(const std::string& evidence_str);
// --- Fine Funzioni Utility ---

// Function declarations for Bayesian Network logic
BayesianNetwork parseBIF(const std::string& filename);
std::vector<int> topological_sort(const BayesianNetwork& bn);
BayesianNetwork reorder_network_topologically(const BayesianNetwork& original_bn, const std::vector<int>& topological_order);
double getConditionalProbabilityFromCPT(const Variable& target_var, const std::vector<int>& config_vector_ancestors, int target_value_idx, const BayesianNetwork& bn);
std::map<std::string, std::map<std::string, double>> calculateProbabilitiesWithEvidence(const BayesianNetwork& reordered_bn, const Evidence& evidence);
std::string getValueString(const Variable& var, int index);

#endif // BAYESIAN_NETWORK_H





