#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm> // For std::remove

// Define a structure for a variable
struct Variable {
    std::string name;
    std::vector<std::string> values;
    std::vector<std::string> parents; // Parents for probability tables
    std::vector<std::vector<double>> cpt; // Conditional Probability Table
    // Add an ID for topological sort mapping
    int id;
};

// A simple structure to hold the entire Bayesian Network
struct BayesianNetwork {
    std::map<std::string, Variable> variables;
    std::vector<std::vector<int>> adj; // Adjacency list for DAG (using variable IDs)
    std::map<std::string, int> name_to_id; // Map variable names to their IDs
    std::map<int, std::string> id_to_name;
    int next_id = 0; // Counter for assigning unique IDs to variables
};

// Helper function to trim whitespace from a string
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) {
        return ""; // CORREZIONE: Restituisce stringa vuota se è solo whitespace
    }
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
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

// Funzione helper ricorsiva per la DFS per l'ordinamento topologico
// `u`: l'ID del nodo corrente da visitare
// `bn`: la rete bayesiana che contiene il grafo
// `visited`: un vettore per tenere traccia dei nodi già visitati
// `recursion_stack`: un vettore per rilevare cicli (non strettamente necessario per DAG garantiti, ma buona pratica)
// `result`: il vettore dove verrà accumulato l'ordinamento topologico
void dfs_topological_sort_helper(int u, const BayesianNetwork &bn,
                                 std::vector<bool> &visited,
                                 std::vector<bool> &recursion_stack, // Aggiunto per rilevamento cicli
                                 std::vector<int> &result)
{
    visited[u] = true;
    recursion_stack[u] = true; // Marca il nodo come parte dello stack di ricorsione attuale

    // Itera sui vicini (figli) del nodo corrente
    // bn.adj[u] contiene gli ID dei nodi figli di u
    for (int v : bn.adj[u])
    {
        if (recursion_stack[v])
        {
            // Ciclo rilevato!
            std::cerr << "Error: Cycle detected! Edge from " << bn.id_to_name.at(u)
                      << " (ID " << u << ") to " << bn.id_to_name.at(v)
                      << " (ID " << v << "). " << std::endl;        
        }
        if (!visited[v])
        { // Se il figlio non è stato visitato, chiama ricorsivamente
            dfs_topological_sort_helper(v, bn, visited, recursion_stack, result);
        }
        // else if (recursion_stack[v]) {
        //     // Se il figlio è nello stack di ricorsione attuale, signi fica che abbiamo trovato un ciclo.
        //     // Per le reti bayesiane, questo non dovrebbe accadere (sono DAG), ma è un buon controllo.
        //     // std::cerr << "Error: Cycle detected in Bayesian Network! Node " << bn.id_to_name.at(u)
        //     //           << " points to " << bn.id_to_name.at(v) << " which is an ancestor." << std::endl;
        //     // Qui potresti voler lanciare un'eccezione o gestire l'errore.
        // }
    }

    // Una volta visitati tutti i figli di 'u', e 'u' stesso,
    // aggiungilo all'inizio della lista dei risultati.
    // Questo garantisce che i nodi senza dipendenze vengano aggiunti per primi,
    // e i nodi che dipendono da altri vengano aggiunti dopo le loro dipendenze.
    result.insert(result.begin(), u);
    recursion_stack[u] = false; // Rimuovi il nodo dallo stack di ricorsione
}

// Funzione principale per eseguire l'ordinamento topologico
// Restituisce un vettore di ID di variabili nell'ordine topologico
std::vector<int> topological_sort(const BayesianNetwork &bn)
{
    std::vector<int> result; // Il vettore che conterrà l'ordinamento finale

    // Trova l'ID massimo per dimensionare correttamente i vettori visited e recursion_stack
    int max_id = -1;
    for (const auto &pair : bn.id_to_name)
    { // Usiamo id_to_name che è già popolata
        if (pair.first > max_id)
        {
            max_id = pair.first;
        }
    }
    // Se non ci sono variabili, restituisci un vettore vuoto
    if (max_id == -1)
    {
        return result;
    }

    // Vettori per tenere traccia dello stato di visita dei nodi
    // La dimensione è max_id + 1 perché gli ID sono 0-based
    std::vector<bool> visited(max_id + 1, false);
    std::vector<bool> recursion_stack(max_id + 1, false);
    bool cycle=false;

    // Itera su tutte le variabili della rete (assicurati di coprire tutti i nodi, anche quelli isolati)
    // Non c'è bisogno di un ordine specifico qui, la DFS esplorerà il grafo
    for (const auto &pair : bn.id_to_name)
    {                            // Itera su tutti gli ID validi
        int var_id = pair.first; // Ottieni l'ID della variabile
        if (!visited[var_id])
        { // Se il nodo non è ancora stato visitato
            dfs_topological_sort_helper(var_id, bn, visited, recursion_stack, result);
        }
    }
    return result;
}


// Funzione per creare una nuova BayesianNetwork con ID riassegnati in ordine topologico
// Prende la rete originale e l'ordinamento topologico (vecchi ID)
BayesianNetwork reorder_network_topologically(const BayesianNetwork& original_bn, const std::vector<int>& topological_order) {
    BayesianNetwork reordered_bn;
    reordered_bn.next_id = 0; // Inizializza il contatore per i nuovi ID

    // Mappa i vecchi ID ai nuovi ID (topologici)
    // praticamente manda topological orider in 0:(n-1)
    std::map<int, int> old_to_new_id_map;

    // 1. Popola le variabili nella nuova rete e aggiorna gli ID
    for (int old_id : topological_order) {
        const Variable& original_var = original_bn.variables.at(original_bn.id_to_name.at(old_id));
        
        Variable new_var = original_var; // Copia la variabile originale
        int new_id = reordered_bn.next_id++;
        new_var.id = new_id; // Assegna il nuovo ID topologico

        reordered_bn.variables[new_var.name] = new_var;
        reordered_bn.name_to_id[new_var.name] = new_id;
        reordered_bn.id_to_name[new_id] = new_var.name;
        
        old_to_new_id_map[old_id] = new_id; // Memorizza la mappatura ID
    }

    // 2. Costruisci la nuova lista di adiacenza (adj) con i nuovi ID
    reordered_bn.adj.resize(reordered_bn.next_id); // Dimensione basata sul numero totale di variabili

    for (int old_id_source : topological_order) {
        int new_id_source = old_to_new_id_map.at(old_id_source);

        // Assicurati che l'ID originale sia valido per accedere a original_bn.adj
        if (old_id_source < original_bn.adj.size()) {
            for (int old_id_target : original_bn.adj[old_id_source]) {
                int new_id_target = old_to_new_id_map.at(old_id_target);
                reordered_bn.adj[new_id_source].push_back(new_id_target);
            }
        }
    }

    return reordered_bn;
}


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



// Funzione principale per calcolare le probabilità marginali
// Accetta la rete riordinata topologicamente
std::map<std::string, std::map<std::string, double>> calculateMarginalProbabilities(
    const BayesianNetwork& reordered_bn
) {
    // current_joint_probabilities: mappa configurazioni parziali a probabilità congiunte
    // Chiave: std::vector<int> dove l'indice 'i' è il new_id topologico e il valore è l'indice del valore della variabile i
    std::map<std::vector<int>, double> current_joint_probabilities;
    std::map<std::vector<int>, double> current_partial_probabilities;

    // memorizza le probabilità marginali finali per ogni variabile
    std::map<std::string, std::map<std::string, double>> marginal_probabilities;

        // Itera attraverso le variabili nell'ordine topologico (dagli ID 0 a N-1)
    for (int var_topo_id = 0; var_topo_id < reordered_bn.next_id; ++var_topo_id) {
        const std::string& var_name = reordered_bn.id_to_name.at(var_topo_id);
        const Variable& current_var = reordered_bn.variables.at(var_name); // La variabile corrente (con new_id)

        // Prepara la mappa per la prossima iterazione
        std::map<std::vector<int>, double> next_joint_probabilities;

        // Se è la prima variabile (radice)
        if (var_topo_id == 0) {
            for (size_t val_idx = 0; val_idx < current_var.values.size(); ++val_idx) {
                std::vector<int> config = {static_cast<int>(val_idx)};
                double prob = getConditionalProbabilityFromCPT(current_var, {}, static_cast<int>(val_idx), reordered_bn); // Nessun antenato per la radice
                next_joint_probabilities[config] = prob;
                current_partial_probabilities[config] = prob;
            }
        } else {
            // Per le variabili successive, itera sulle configurazioni precedenti
            for (const auto& entry : current_joint_probabilities) {
                const std::vector<int>& prev_config = entry.first; // Configurazione [val_idx_0, ..., val_idx_k-1]
                double prev_joint_prob = entry.second; // P(X_0=v_0, ..., X_{k-1}=v_{k-1})

                for (size_t val_idx = 0; val_idx < current_var.values.size(); ++val_idx) {
                    std::vector<int> current_config = prev_config;
                    current_config.push_back(static_cast<int>(val_idx)); // Aggiungi il valore della variabile corrente per ultimo

                    // Ottieni P(X_k=x_k | parents of X_k)
                    // La funzione getConditionalProbabilityFromCPT deve usare prev_config
                    // per determinare i valori dei genitori di current_var
                    double conditional_prob = getConditionalProbabilityFromCPT(
                        current_var, 
                        prev_config, // Contiene i valori degli antenati fino a k-1
                        static_cast<int>(val_idx),
                        reordered_bn // Per accedere alle definizioni delle variabili genitore e CPT
                    );

                    next_joint_probabilities[current_config] = prev_joint_prob * conditional_prob;
                    current_partial_probabilities[current_config] = prev_joint_prob * conditional_prob;
                }
            }
        }
        current_joint_probabilities = next_joint_probabilities;
    }


    // --- Calcolo delle Probabilità Marginali Finali ---
    // Dopo aver costruito tutte le probabilità congiunte parziali,
    // possiamo sommarle per ottenere le marginali.
 

    // Inizializza le marginali a zero
    for (const auto& pair : reordered_bn.variables) {
        const Variable& var = pair.second;
        for (const std::string& val_str : var.values) {
            marginal_probabilities[var.name][val_str] = 0.0;
        }
    }

   
    // Somma le probabilità congiunte parziali
    for (const auto& entry : current_partial_probabilities) {
        const std::vector<int>& final_config = entry.first; // Configurazione [val_idx_0, ..., val_idx_N-1]
        double final_joint_prob = entry.second;

        int i = final_config.size()-1;
        const std::string &var_name = reordered_bn.id_to_name.at(i);
        const Variable &var = reordered_bn.variables.at(var_name); // Get variable definition
        int val_idx = final_config[i];
        std::string val_str = getValueString(var, val_idx);

        marginal_probabilities[var_name][val_str] += final_joint_prob;
    }

    // Stampa e verifica
    for (const auto& var_entry : marginal_probabilities) {
        const std::string& var_name = var_entry.first;
        double sum_probs = 0.0;
        std::cout << "P(" << var_name << "):" << std::endl;
        for (const auto& val_entry : var_entry.second) {
            std::cout << "  " << val_entry.first << " -> " << val_entry.second << std::endl;
            sum_probs += val_entry.second;
        }
        std::cout << "  (Sum: " << sum_probs << ")" << std::endl;
        if (std::abs(sum_probs - 1.0) > 1e-9) {
            std::cerr << "Warning: Probabilities for " << var_name << " do not sum to 1.0" << std::endl;
        }
    }

    return marginal_probabilities;
} 


// A new struct or map to hold evidence
using Evidence = std::map<std::string, std::string>;

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