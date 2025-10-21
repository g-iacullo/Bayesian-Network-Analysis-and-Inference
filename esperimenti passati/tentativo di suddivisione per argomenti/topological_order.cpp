#include "topological_order.h" 
#include <iostream>
#include <fstream>
#include <algorithm> 


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