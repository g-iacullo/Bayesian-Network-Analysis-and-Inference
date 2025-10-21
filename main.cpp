// main.cpp
#include <iostream>
#include <fstream>
#include <string>
#include "BayesianNetwork.h"

// --- Main function for testing ---
int main(int argc, char* argv[]) {

    std::cout << std::endl;
    // Default: no evidence
    std::string filename = "";
    Evidence evidence;
    std::string query_variable_name = ""; // Optional: if you want to query a specific variable P(X|E)

    // Parse command line arguments for evidence and filename
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            filename = argv[++i];
        }
        else if (arg == "-e" && i + 1 < argc) {
            evidence = parseEvidenceString(argv[++i]);
            std::cout << "Evidence provided: " << std::endl;
            for(const auto& e_pair : evidence) {
                std::cout << "  " << e_pair.first << " = " << e_pair.second << std::endl;
            }
        } else if (arg == "-q" && i + 1 < argc) { // Example for a query variable
            query_variable_name = trim(argv[++i]);
            std::cout << "Query variable: " << query_variable_name << std::endl;
        } 
        // Add other argument parsing as needed (e.g., for different BIF files)
    }
    if (argc > 1) {
        std::cout << std::endl;
    }

    // Create a dummy BIF file for testing
    std::ofstream outfile("gradient.bif");
    outfile << R"(network "GradientBN" {}

variable a {
  type discrete [2] { true, false };
}

variable b {
  type discrete [2] { true, false };
}

variable c {
  type discrete [2] { true, false };
}

variable d {
  type discrete [2] { true, false };
}

variable e {
  type discrete [2] { true, false };
}

probability (a) {
  table 0.5, 0.5;
}

probability (b | a) {
  (true) 0.8, 0.2;
  (false) 0.3, 0.7;
}

probability (c | a) {
  (true) 0.6, 0.4;
  (false) 0.2, 0.8;
}

probability (d | b, c) {
  (true, true) 0.9, 0.1;
  (true, false) 0.7, 0.3;
  (false, true) 0.6, 0.4;
  (false, false) 0.1, 0.9;
}

probability (e | a, c, d) {
  (true, true, true) 0.95, 0.05;
  (true, true, false) 0.85, 0.15;
  (true, false, true) 0.75, 0.25;
  (true, false, false) 0.5, 0.5;
  (false, true, true) 0.8, 0.2;
  (false, true, false) 0.6, 0.4;
  (false, false, true) 0.3, 0.7;
  (false, false, false) 0.1, 0.9;
})";
    outfile.close();

    BayesianNetwork bn; // Dichiara bn prima del blocco if/else per renderla accessibile dopo

    if (!filename.empty()) { // Controlla se il nome del file NON è vuoto
        bn = parseBIF(filename);
    } else { // Se il nome del file è vuoto
        std::cout << "No BIF filename provided. Using default 'gradient.bif'." << std::endl << std::endl;
        filename = "gradient.bif"; // Assegna il nome del file di default
        bn = parseBIF(filename); // Parsa il file di default
    }

    // Print parsed data to verify
    std::cout << "--- Parsed Bayesian Network ---" << std::endl;
    for (const auto& pair : bn.variables) {
        const Variable& var = pair.second;
        std::cout << "Variable: " << var.name << " (ID: " << var.id << ")" << std::endl;
        std::cout << "  Values: ";
        for (const auto& val : var.values) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
        if (!var.parents.empty()) {
            std::cout << "  Parents: ";
            for (const auto& parent : var.parents) {
                std::cout << parent << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "  CPT:" << std::endl;
        for (const auto& row : var.cpt) {
            std::cout << "    ";
            for (double val : row) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }

    std::cout << "--- Adjacency List (DAG) ---" << std::endl;
    for (int i = 0; i < bn.adj.size(); ++i) {
        
        std::string var_name = bn.id_to_name[i];

        if (!var_name.empty()) {
            std::cout << var_name << " (ID " << i << ") -> ";
            for (int neighbor_id : bn.adj[i]) {
                std::string neighbor_name = bn.id_to_name[neighbor_id];
                std::cout << neighbor_name << " (ID " << neighbor_id << ") ";
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;


    // riassegnamo alle variabili dei nuovi ID corrispondenti all'ordinamento topologico
    std::cout << "--- Topological Order (original IDs) ---" << std::endl;
    std::vector<int> topo_order_original_ids = topological_sort(bn);
    for (int id : topo_order_original_ids)
    {
        std::cout << bn.id_to_name.at(id) << " (Original ID " << id << ") ";
    }
    std::cout << std::endl
              << std::endl;

    // Ora riordina la rete
    BayesianNetwork reordered_bn = reorder_network_topologically(bn, topo_order_original_ids);

    std::cout << "--- Reordered Adjacency List (Topological IDs) ---" << std::endl;
    for (int i = 0; i < reordered_bn.adj.size(); ++i)
    {
        if (reordered_bn.id_to_name.count(i))
        {
            std::string var_name = reordered_bn.id_to_name.at(i);
            std::cout << var_name << " (NEW ID " << i << ") -> ";
            for (int neighbor_id : reordered_bn.adj[i])
            {
                if (reordered_bn.id_to_name.count(neighbor_id))
                {
                    std::string neighbor_name = reordered_bn.id_to_name.at(neighbor_id);
                    std::cout << neighbor_name << " (NEW ID " << neighbor_id << ") ";
                }
                else
                {
                    std::cout << "[Unknown NEW ID " << neighbor_id << "] ";
                }
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;


    // Call a new or modified function to calculate probabilities with evidence
    auto marginal_probabilities = calculateProbabilitiesWithEvidence(reordered_bn, evidence);

    // --- Print Results ---
    std::cout << "\n--- Calculated Probabilities ---" << std::endl;
    for (const auto& var_entry : marginal_probabilities) {
        const std::string& var_name = var_entry.first;
        // Only print if it's the query variable or no specific query was asked, and not an evidence variable
        if ((query_variable_name.empty() || var_name == query_variable_name) && !evidence.count(var_name)) {
            double sum_probs = 0.0;
            std::cout << "P(" << var_name;
            if (!evidence.empty()) {
                std::cout << " | E";
            }
            std::cout << "):" << std::endl;

            for (const auto& val_entry : var_entry.second) {
                std::cout << "  " << val_entry.first << " -> " << val_entry.second << std::endl;
                sum_probs += val_entry.second;
            }
            std::cout << "  (Sum: " << sum_probs << ")" << std::endl;
            if (std::abs(sum_probs - 1.0) > 1e-9) {
                std::cerr << "Warning: Probabilities for " << var_name << " do not sum to 1.0" << std::endl;
            }
        } else if (evidence.count(var_name)) {
             // For evidence variables, just confirm their state
            std::cout << "P(" << var_name << " = " << evidence.at(var_name) << ") is fixed by evidence." << std::endl;
        }
    }

    return 0;
}