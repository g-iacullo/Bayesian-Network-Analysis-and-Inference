#include "inference.h" 
#include <iostream>
#include <fstream>
#include <algorithm> 

// Restituisce il valore (stringa) che la variabile assume, dato il suo indice
std::string getValueString(const Variable& var, int index) {
    if (index >= 0 && index < var.values.size()) {
        return var.values[index];
    }
    // Handle error
    return "";
}

// Funzione (DA IMPLEMENTARE) per ottenere la probabilità condizionale dalla CPT
// target_var: la variabile di cui vogliamo la probabilità
// config_vector_ancestors: la configurazione corrente delle variabili già elaborate topologicamente 
// target_value_idx: l'indice del valore che target_var assume
// bn: la rete bayesiana (necessaria per mappare i genitori, e quindi ottenere il loro id a partire dal nome)
double getConditionalProbabilityFromCPT(
    const Variable& target_var, 
    const std::vector<int>& config_vector_ancestors,
    int target_value_idx,
    const BayesianNetwork& bn // Passa la BN per accedere ai dati dei genitori
) {
    // Caso 1: Nessun genitore (variabile radice)
    if (target_var.parents.empty()) {
        if (target_var.cpt.empty() || target_var.cpt[0].size() <= target_value_idx) {
            std::cerr << "Error: CPT for " << target_var.name << " is empty or invalid for value index " << target_value_idx << std::endl;
            return 0.0;
        }
        return target_var.cpt[0][target_value_idx]; // Probabilità marginale diretta
    }

    // Caso 2: Ha genitori
    // Dobbiamo costruire la configurazione dei genitori a partire da quella di tutti gli antenati indiretti fornita come parametro
    std::vector<int> parent_value_indices; 
    for (const std::string& parent_name : target_var.parents) {
        int parent_topo_id = bn.name_to_id.at(parent_name); // Questo è l'ID del genitore nell'ordinamento topologico, è qui che usiamo l'informazione proveniente dalla rete intera

        if (parent_topo_id >= config_vector_ancestors.size()) {
             // Questo genitore non dovrebbe essere un antenato nella configurazione attuale
             std::cerr << "Error: Parent " << parent_name << " (ID " << parent_topo_id << ") not found in ancestor config." << std::endl;
             return 0.0; // O gestire l'errore in altro modo
        }
        
        int parent_val_idx = config_vector_ancestors[parent_topo_id];
        parent_value_indices.push_back(parent_val_idx);
    }

    // Calcola l'indice di riga della CPT (generalizzato al caso di variabili che assumono anche più di 2 valori)
    int cpt_row_index = 0;
    int multiplier = 1;
    for (size_t i = 0; i < target_var.parents.size(); ++i) {
        const Variable& parent_var = bn.variables.at(target_var.parents[target_var.parents.size() - 1 - i]); // Dal meno significativo al più significativo
        cpt_row_index += parent_value_indices[target_var.parents.size() - 1 - i] * multiplier;
        multiplier *= static_cast<int>(parent_var.values.size());
    }

    if (cpt_row_index >= target_var.cpt.size() || target_var.cpt[cpt_row_index].size() <= target_value_idx) {
        std::cerr << "Error: CPT lookup out of bounds for " << target_var.name << " at row " << cpt_row_index << " / val " << target_value_idx << std::endl;
        return 0.0;
    }

    return target_var.cpt[cpt_row_index][target_value_idx];
}


// New function signature for calculating marginals with optional evidence and query variable
std::map<std::string, std::map<std::string, double>> calculateProbabilitiesWithEvidence(
    const BayesianNetwork& reordered_bn,
    const Evidence& evidence // New parameter for evidence
) {
    std::map<std::vector<int>, double> current_joint_probabilities; // Key: config (vector of value indices), Value: joint probability P(config)
    std::map<std::vector<int>, double> current_partial_probabilities;


    // Map variable names to their value indices for faster lookup
    // e.g., for var "a", if "true" is at index 0, "false" at index 1: value_to_idx["a"]["true"] = 0
    std::map<std::string, std::map<std::string, int>> var_value_to_idx;
    for (const auto& pair : reordered_bn.variables) {
        const Variable& var = pair.second;
        for (size_t i = 0; i < var.values.size(); ++i) {
            var_value_to_idx[var.name][var.values[i]] = static_cast<int>(i);
        }
    }

    // Iterate through variables in topological order
    for (int var_topo_id = 0; var_topo_id < reordered_bn.next_id; ++var_topo_id) {
        const std::string& var_name = reordered_bn.id_to_name.at(var_topo_id);
        const Variable& current_var = reordered_bn.variables.at(var_name);

        std::map<std::vector<int>, double> next_joint_probabilities;

        if (var_topo_id == 0) { // First variable (root)
            for (size_t val_idx = 0; val_idx < current_var.values.size(); ++val_idx) {
                std::vector<int> config = {static_cast<int>(val_idx)};
                double prob = getConditionalProbabilityFromCPT(current_var, {}, static_cast<int>(val_idx), reordered_bn);

                // Apply evidence: if this config contradicts evidence, set prob to 0
                if (evidence.count(var_name)) { // Is this variable part of the evidence?
                    int evidence_val_idx = var_value_to_idx.at(var_name).at(evidence.at(var_name));
                    if (val_idx != evidence_val_idx) {
                        prob = 0.0; // This configuration is inconsistent with evidence
                    }
                }
                next_joint_probabilities[config] = prob;
                current_partial_probabilities[config] = prob;
            }
        } else { // Subsequent variables
            for (const auto& entry : current_joint_probabilities) {
                const std::vector<int>& prev_config = entry.first;
                double prev_joint_prob = entry.second;

                for (size_t val_idx = 0; val_idx < current_var.values.size(); ++val_idx) {
                    std::vector<int> current_config = prev_config;
                    current_config.push_back(static_cast<int>(val_idx));

                    double conditional_prob = getConditionalProbabilityFromCPT(
                        current_var,
                        prev_config,
                        static_cast<int>(val_idx),
                        reordered_bn
                    );

                    double joint_prob_for_current_config = prev_joint_prob * conditional_prob;

                    // Apply evidence for the current variable:
                    if (evidence.count(var_name)) {
                        int evidence_val_idx = var_value_to_idx.at(var_name).at(evidence.at(var_name));
                        if (val_idx != evidence_val_idx) {
                            joint_prob_for_current_config = 0.0; // Inconsistent with evidence
                        }
                    }
                    next_joint_probabilities[current_config] = joint_prob_for_current_config;
                    current_partial_probabilities[current_config] = joint_prob_for_current_config;
                }
            }
        }
        current_joint_probabilities = next_joint_probabilities;
    }

    // --- Aggregation and Normalization ---
    std::map<std::string, std::map<std::string, double>> marginal_probabilities;

    // Initialize marginals
    for (const auto& pair : reordered_bn.variables) {
        const Variable& var = pair.second;
        for (const std::string& val_str : var.values) {
            marginal_probabilities[var.name][val_str] = 0.0;
        }
    }

    // Calculate P(Evidence) for normalization (if evidence is present)
    // prob_evidence è 1 se l'evidenza è vuota, altrimenti è la probabilità dell'evidenza 
    double prob_evidence_sum = 0.0;
    for (const auto& entry : current_joint_probabilities) {
        prob_evidence_sum += entry.second;
    }
    

    // Aggregate marginal probabilities for each variable
    for (const auto& entry : current_partial_probabilities) {
        const std::vector<int>& final_config = entry.first;
        double joint_prob_with_evidence = entry.second; // This is P(config, evidence)

        // If evidence was provided, normalize by P(evidence)
        if (!evidence.empty() && prob_evidence_sum > 1e-12) { // Avoid division by zero
            joint_prob_with_evidence /= prob_evidence_sum; // Now it's P(config | evidence)
        }

        int i = final_config.size()-1;
        const std::string &var_name = reordered_bn.id_to_name.at(i);
        const Variable &var = reordered_bn.variables.at(var_name); // Get variable definition
        int val_idx = final_config[i];
        std::string val_str = getValueString(var, val_idx);

        marginal_probabilities[var_name][val_str] += joint_prob_with_evidence;
    }

    return marginal_probabilities;
}