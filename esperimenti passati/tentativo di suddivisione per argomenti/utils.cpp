#include "utils.h" 
#include <iostream>
#include <fstream>
#include <algorithm> 


// Function to parse evidence from a string like "a=true,c=false"
Evidence parseEvidenceString(const std::string& evidence_str) {
    Evidence evidence;
    std::stringstream ss(evidence_str);
    std::string pair_str;
    while (std::getline(ss, pair_str, ',')) {
        size_t eq_pos = pair_str.find("=");
        if (eq_pos != std::string::npos) {
            std::string var_name = trim(pair_str.substr(0, eq_pos));
            std::string var_value = trim(pair_str.substr(eq_pos + 1));
            evidence[var_name] = var_value;
        }
    }
    return evidence;
}

// Restituisce il valore (stringa) che la variabile assume, dato il suo indice
std::string getValueString(const Variable& var, int index) {
    if (index >= 0 && index < var.values.size()) {
        return var.values[index];
    }
    // Handle error
    return "";
}
