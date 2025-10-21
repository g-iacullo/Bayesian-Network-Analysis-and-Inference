#include "parser.h" 
#include <iostream>
#include <fstream>
#include <algorithm> 

// Helper function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) {
        return ""; // CORREZIONE: Restituisce stringa vuota se è solo whitespace
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

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


// Function to parse the BIF file
BayesianNetwork parseBIF(const std::string& filename) {
    BayesianNetwork bn;
    std::ifstream file(filename);
    std::string line;
    std::string current_block_type; // "variable" or "probability"
    std::string current_variable_name; // For parsing variable blocks
    std::string current_prob_target; // For parsing probability blocks

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return bn;
    }

    while (std::getline(file, line)) {
        line = trim(line); // Trim leading/trailing whitespace

        if (line.empty() || line.rfind("//", 0) == 0) { // Skip empty lines and comments
            continue;
        }

        if (line.rfind("network", 0) == 0) {
            // std::cout << "Parsing network block." << std::endl;
            // Handle network name if needed, for this project it's not strictly necessary.
        } else if (line.rfind("variable", 0) == 0) {
            current_block_type = "variable";
            // Extract variable name
            size_t start = line.find("variable") + 8;
            size_t end = line.find("{", start);
            current_variable_name = trim(line.substr(start, end - start));

            Variable new_var;
            new_var.name = current_variable_name;
            new_var.id = bn.next_id++;
            bn.name_to_id[new_var.name] = new_var.id;
            bn.id_to_name[new_var.id] = new_var.name;
            // Initialize adj list with enough space
            if (bn.adj.size() <= new_var.id) {
                bn.adj.resize(new_var.id + 1);
            }
            bn.variables[current_variable_name] = new_var;
            // std::cout << "Found variable: " << current_variable_name << std::endl;

        } else if (line.rfind("probability", 0) == 0) {
            current_block_type = "probability";
            // Extract target variable and parents
            size_t start = line.find("probability") + 11;
            size_t end_paren = line.find(")");
            std::string prob_decl = trim(line.substr(start + 2, end_paren - (start + 2))); // +1 for '('

            size_t pipe_pos = prob_decl.find("|");
            if (pipe_pos != std::string::npos) {
                // Has parents
                current_prob_target = trim(prob_decl.substr(0, pipe_pos));
                std::string parents_str = trim(prob_decl.substr(pipe_pos + 1));
                // std::cout << current_prob_target << "|||" ;

                std::stringstream ss(parents_str);
                std::string parent_name;
                while (std::getline(ss, parent_name, ',')) {
                    bn.variables[current_prob_target].parents.push_back(trim(parent_name));
                    // Add edge to DAG (parent -> target)
                    int parent_id = bn.name_to_id[trim(parent_name)];
                    int target_id = bn.name_to_id[current_prob_target];
                    bn.adj[parent_id].push_back(target_id);
                    // std::cout << "  Edge: " << trim(parent_name) << " -> " << current_prob_target << std::endl;
                }
            } else {
                // No parents (marginal probability)
                current_prob_target = trim(prob_decl);
                // std::cout << current_prob_target << "|||" ;
            }
            // std::cout << "Found probability for: " << current_prob_target << std::endl;

        } else if (line.rfind("type discrete", 0) == 0 && current_block_type == "variable") {
            // Extract values for the current variable
            size_t start_brace = line.find("{");
            size_t end_brace = line.find("}");
            std::string values_str = trim(line.substr(start_brace + 1, end_brace - start_brace - 1));

            std::stringstream ss(values_str);
            std::string value;
            while (std::getline(ss, value, ',')) {
                bn.variables[current_variable_name].values.push_back(trim(value));
            }
            // std::cout << "  Values: ";
            // for(const auto& val : bn.variables[current_variable_name].values) std::cout << val << " ";
            // std::cout << std::endl;

        } else if (line.rfind("table", 0) == 0 && current_block_type == "probability") {
            // Extract CPT values for the current probability block
            std::string table_line = line.substr(line.find("table") + 5);
            table_line = trim(table_line);
            
            // Remove semicolon if present at the end
            if (!table_line.empty() && table_line.back() == ';') {
                table_line.pop_back();
            }

            // This implies a simple marginal probability table (e.g., table 0.5, 0.5;)
            std::vector<double> marginal_cpt;
            std::stringstream ss_probs(table_line);
            std::string prob_str;
            while (std::getline(ss_probs, prob_str, ','))
            {
                try
                {
                    marginal_cpt.push_back(std::stod(trim(prob_str)));
                }
                catch (const std::invalid_argument &e)
                {
                    std::cerr << "Invalid argument for stod: " << prob_str << std::endl;
                }
                catch (const std::out_of_range &e)
                {
                    std::cerr << "Out of range for stod: " << prob_str << std::endl;
                }
            }
            if (!marginal_cpt.empty())
            {
                bn.variables[current_prob_target].cpt.push_back(marginal_cpt);
            }

            // std::cout << "  CPT for " << current_prob_target << ": ";
            // for(const auto& row : bn.variables[current_prob_target].cpt) {
            //     for(double val : row) std::cout << val << " ";
            //     std::cout << "| ";
            // }
            // std::cout << std::endl;
        }
        // If there are conditional values (e.g., (true) 0.8, 0.2;)
         else if (line.rfind("(", 0) == 0 && current_block_type == "probability") {
            // Questo blocco intercetta una riga che inizia direttamente con una tupla condizionale, 
            // e presuppone che il parsing della 'table' non sia ancora avvenuto sulla riga precedente.
            // (Molto raro nei formati BIF standard, ma implementato come richiesto)
            
            // Uniamo le righe fino alla chiusura '}' del blocco probability, se necessario.
            std::string raw_data = line;
            bool block_closed = (line.find("}") != std::string::npos);

            // Accumulo delle righe (simulando l'accumulo di parseCPT)
            while (!block_closed && std::getline(file, line)) {
                std::string next_cleaned_line = trim(line);
                if (next_cleaned_line.empty() || next_cleaned_line.rfind("//", 0) == 0) continue;
                
                raw_data += " " + next_cleaned_line;
                if (next_cleaned_line.find("}") != std::string::npos) {
                    block_closed = true;
                }
            }

            // --- Pulizia e Normalizzazione della stringa unica ---
            std::string normalized_data = raw_data;
            
            // 1. Sostituisci ; con ,
            std::replace(normalized_data.begin(), normalized_data.end(), ';', ',');
            
            // 2. Rimuovi tutte le sezioni comprese tra parentesi tonde (es. (true, false))
            size_t open_paren = normalized_data.find("(");
            while (open_paren != std::string::npos) {
                size_t close_paren = normalized_data.find(")", open_paren);
                if (close_paren != std::string::npos) {
                    // Rimuove la tupla condizionale e le parentesi
                    normalized_data.erase(open_paren, close_paren - open_paren + 1);
                }
                open_paren = normalized_data.find("(");
            }
            
            // 3. Rimuovi la '}' di chiusura del blocco
            size_t brace_pos = normalized_data.find("}");
            if (brace_pos != std::string::npos) {
                normalized_data.erase(brace_pos);
            }
            
            // 4. Estrazione finale dei valori (usando la virgola come separatore)
            std::stringstream ss_data(normalized_data);
            std::string prob_str;
            std::vector<double> all_cpt_values;
            
            while (std::getline(ss_data, prob_str, ',')) {
                std::string trimmed_prob = trim(prob_str);
                if (trimmed_prob.empty()) continue;
                
                try {
                    all_cpt_values.push_back(std::stod(trimmed_prob));
                } catch (const std::exception& e) {
                    // Ignora
                }
            }

            // 5. Aggiungi i valori alla CPT della variabile corrente.
            // NOTA: Con questo approccio, tutti i valori finiscono in UN'UNICA riga della CPT,
            // cosa non corretta per il BIF, che richiede una riga CPT per ogni configurazione dei genitori.

            const size_t PROB_COUNT_PER_ROW = 2;

            // Resetta la CPT per evitare di mescolare dati in caso di esecuzione multipla o parsing complesso.
            bn.variables[current_prob_target].cpt.clear();

            if (!all_cpt_values.empty())
            {

                // Il contatore 'i' scorre l'array appiattito 'all_cpt_values' a passi di 2
                for (size_t i = 0; i < all_cpt_values.size(); i += PROB_COUNT_PER_ROW)
                {

                    // La variabile 'cpt_row_index' tiene traccia della riga CPT che stai costruendo (i/2)
                    // size_t cpt_row_index = i / PROB_COUNT_PER_ROW;

                    // Verifica che ci siano almeno due elementi nel blocco
                    if (i + 1 < all_cpt_values.size())
                    {

                        // 1. Creiamo la nuova riga CPT usando l'inizializzazione uniforme (la più pulita)
                        std::vector<double> new_row = {
                            all_cpt_values[i],
                            all_cpt_values[i + 1]};

                        // 2. Aggiungiamo la riga alla matrice CPT. Questa operazione inserisce
                        // la riga all'indice 'i/2' in modo sicuro.
                        bn.variables[current_prob_target].cpt.push_back(new_row);

                        // Se volessi stampare l'indice in questo momento:
                        // std::cout << "Aggiunta riga CPT all'indice: " << i / PROB_COUNT_PER_ROW << std::endl;
                    }
                    else
                    {
                        std::cerr << "Attenzione: Ultima riga CPT incompleta per " << current_prob_target << std::endl;
                    }
                }
            }
            // Fine Codice Corretto
        } // Fine del nuovo else if

         else if (line.rfind("}", 0) == 0) {
            // Chiusura di un blocco. Resetta il tipo di blocco.
            current_block_type = "";
            current_variable_name = "";
            current_prob_target = ""; // Reset cruciale per la stampa
        }
    }

    file.close();
    return bn;
}
