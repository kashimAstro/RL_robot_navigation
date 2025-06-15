#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <numeric>
#include <algorithm>
#include <tuple>
#include <fstream>
#include <set>
#include <map>
#include <unistd.h>
#include <Eigen/Dense>
#ifdef CURL_SAMPLE
	#include <curl/curl.h>
#endif

// short policy
using loaded_policy = std::map<std::pair<int, int>, int>;

// costanti per la mappa
//const int CELL_EMPTY = 0;
//const int CELL_OBSTACLE = 1;
//const int CELL_GOAL = 2;
//const int CELL_START = 3;
// 0: vuoto, 1: ostacolo, 2: obie

// converto stato (x, y) in una rappresentazione 1D
// appiattita della griglia
std::vector<double> state_to_one_hot_flattened(int x, int y, int grid_width, int grid_height) {
    std::vector<double> one_hot(grid_width * grid_height, 0.0);
    if (x >= 0 && x < grid_width && y >= 0 && y < grid_height) {
        int index = y * grid_width + x;
        one_hot[index] = 1.0;
    }
    return one_hot;
}

// stringa azione in un indice numerico
int action_string_to_int(const std::string& action_str) {
    if (action_str == "Su") return 0;
    if (action_str == "Giu") return 1;
    if (action_str == "Sinistra") return 2;
    if (action_str == "Destra") return 3;
    return -1; // Azione non riconosciuta
}

#ifdef CURL_SAMPLE
size_t xxcurl_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
#endif

// per un esempio con piccolo roboto basato su esp8266 
bool curl_request_esp8266(const std::string ip, const std::string cmd, int istep_lin = 2500, int istep_rot = 1000) {
#ifdef CURL_SAMPLE
    std::string url;
    std::string response_data;
    CURL* curl;
    CURLcode res;
    std::string step_l = std::to_string(istep_lin);
    std::string step_r = std::to_string(istep_rot);


    if(cmd == "SU") 
      url ="http://"+ip+"/forward?mot1=1&mot2=1&step="+step_l; 
    else if(cmd == "GIU") 
      url ="http://"+ip+"/backward?mot1=1&mot2=1&step="+step_l;
    else if(cmd == "SINISTRA") 
      url ="http://"+ip+"/left?mot1=1&mot2=1&step="+step_r;
    else if(cmd == "DESTRA") 
      url ="http://"+ip+"/right?mot1=1&mot2=1&step="+step_r;

    std::cerr <<"uri: "<< url << std::endl;
 
/*
  <a href="#" onclick="fetch('/forward?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">su</a><br>
  <a href="#" onclick="fetch('/backward?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">giu</a><br>
  <a href="#" onclick="fetch('/left?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">sinistra</a><br>
  <a href="#" onclick="fetch('/right?mot1=1&mot2=1').then(response => response.json()).then(data => console.log(data));">destra</a><br>
*/

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, xxcurl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

	std::cerr <<"response:"<< response_data << std::endl;

        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Errore curl_easy_perform(): " << curl_easy_strerror(res) << std::endl;
            response_data.clear();
            return false;
        }

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        std::cout << "Codice di stato HTTP: " << http_code << std::endl;
        curl_easy_cleanup(curl);
    } else {
        std::cerr << "Errore: Impossibile inizializzare libcurl easy handle." << std::endl;
        return false;
    }
    curl_global_cleanup();
#endif
    return true;
}

//std::map<std::pair<int, int>, std::string> 
std::vector<std::string> one_shot_navy_cmd(int width, int height, const std::pair<int, int>& agent_pos, const std::pair<int, int>& goal_pos, const std::pair<int, int>& start_pos, const std::vector<std::vector<int>>& grid_map, const std::vector<std::pair<int, int>>& agent_path, const std::map<std::pair<int, int>, int>* optimal_actions = nullptr) {
    
    std::cout << "◽";
    for (int i = 0; i < width; ++i)
        std::cout << "▪▪▪◽";
    std::cout << std::endl;

    std::set<std::pair<int, int>> path_set;
    if (!agent_path.empty())
        path_set.insert(agent_path.begin(), agent_path.end()); 

    std::map<std::pair<int, int>, std::string> path_symbols;
    std::vector<std::string> command_esp;
    //std::map<std::pair<int, int>, std::string> command_esp;

    if (optimal_actions != nullptr) {
        std::vector<std::pair<int, int>> current_optimal_path;
        std::pair<int, int> current_cell = start_pos;
        int max_path_length = width * height * 2; 
        int steps = 0;
        while (current_cell != goal_pos && steps < max_path_length) {
            current_optimal_path.push_back(current_cell);
            if (optimal_actions->count(current_cell)) {
                int action = optimal_actions->at(current_cell);
                std::pair<int, int> next_cell = current_cell;
                if (action == 0) next_cell.second -= 1;
                else if (action == 1) next_cell.second += 1;
                else if (action == 2) next_cell.first -= 1;
                else if (action == 3) next_cell.first += 1;

                if (next_cell.first < 0 || next_cell.first >= width ||
                    next_cell.second < 0 || next_cell.second >= height ||
                    grid_map[next_cell.second][next_cell.first] == CELL_OBSTACLE) {
                    break;
                }
                current_cell = next_cell;
            } else {
                break; 
            }
            steps++;
        }
        if (current_cell == goal_pos) 
             current_optimal_path.push_back(goal_pos);

        for (size_t i = 0; i < current_optimal_path.size(); ++i) {
            std::pair<int, int> cell = current_optimal_path[i];
            if (cell == start_pos || cell == goal_pos) continue;
            if (i > 0 && i < current_optimal_path.size() -1) {
                std::pair<int, int> prev = current_optimal_path[i - 1];
                std::pair<int, int> next = current_optimal_path[i + 1];
                int dx_prev = cell.first - prev.first;
                int dy_prev = cell.second - prev.second;
                int dx_next = next.first - cell.first;
                int dy_next = next.second - cell.second;
                int action_in = -1;
                if (dx_prev == 0 && dy_prev == -1) action_in = 0;
                else if (dx_prev == 0 && dy_prev == 1) action_in = 1;
                else if (dx_prev == -1 && dy_prev == 0) action_in = 2;
                else if (dx_prev == 1 && dy_prev == 0) action_in = 3;

                int action_out = -1;
                if (dx_next == 0 && dy_next == -1) action_out = 0;
                else if (dx_next == 0 && dy_next == 1) action_out = 1;
                else if (dx_next == -1 && dy_next == 0) action_out = 2;
                else if (dx_next == 1 && dy_next == 0) action_out = 3;
                if (action_in != action_out) {
		    //svolta
                    if ((action_in == 0 && action_out == 2) ||
                        (action_in == 1 && action_out == 3) ||
                        (action_in == 3 && action_out == 0) ||
                        (action_in == 2 && action_out == 1)) {
                        path_symbols[cell] = " ↩  |"; 
		        //command_esp[cell]  = "SU,SINISTRA,SU";
		        command_esp.push_back("SU");
                        command_esp.push_back("SINISTRA");
                        command_esp.push_back("SU"); 
                    } else if ((action_in == 0 && action_out == 3) ||
                               (action_in == 1 && action_out == 2) ||
                               (action_in == 3 && action_out == 1) ||
                               (action_in == 2 && action_out == 0)) {
                        path_symbols[cell] = " ↪  |";
			//command_esp[cell]  = "SU,DESTRA,SU";
		        command_esp.push_back("SU");
                        command_esp.push_back("DESTRA");
                        command_esp.push_back("SU"); 
                    } else {
                        path_symbols[cell] = " ❓ |";
                    }
                } else {
                    //retta
                    if (action_out == 0) path_symbols[cell] = " ⬆  |";
                    else if (action_out == 1) path_symbols[cell] = " ⬇  |";
                    else if (action_out == 2) path_symbols[cell] = " ⬅  |";
                    else if (action_out == 3) path_symbols[cell] = " ➡  |";
                    else path_symbols[cell] = " 🪙 |";
                    //command_esp[cell] = "SU";
		    command_esp.push_back("SU");
                }
            } /*else if (optimal_actions->count(cell)) {
                 int action = optimal_actions->at(cell);
                 if (action == 0) path_symbols[cell] = " ⬆  |";
                 else if (action == 1) path_symbols[cell] = " ⬇  |";
                 else if (action == 2) path_symbols[cell] = " ⬅  |";
                 else if (action == 3) path_symbols[cell] = " ➡  |";
                 else path_symbols[cell] = " 🪙 |";
                 command_esp[cell] = "AVANTI";
            }*/
        }
    }

    for (int y = 0; y < height; ++y) {
        std::cout << "|";
        for (int x = 0; x < width; ++x) {
            std::pair<int, int> current_cell = {x, y};
            if (agent_pos.first == x && agent_pos.second == y)
                std::cout << " 🤖 |"; // robot 
            else if (goal_pos.first == x && goal_pos.second == y)
                std::cout << " ⛳ |"; // obiettivo
            else if (start_pos.first == x && start_pos.second == y) 
                 std::cout << " 🏠 |"; // partenza
            else if (grid_map[y][x] == CELL_OBSTACLE) 
                 std::cout << " 🧱 |"; // ostacolo
            else if (path_symbols.count(current_cell)) // mostra le frecce di svolta o direzione
                 std::cout << path_symbols.at(current_cell);
            else if (path_set.count(current_cell)) 
                std::cout << " 🪙  |"; // parte del percorso (generica)
            else 
                std::cout << " 🔹 |"; // cella vuota
        }
        std::cout << std::endl;

        std::cout << "◽";
        for (int i = 0; i < width; ++i) 
            std::cout << "▪▪▪◽";
        std::cout << std::endl;
    }
    
    return command_esp;
}

// stampa griglia con ostacoli e il percorso
void print_grid(int width, int height, const std::pair<int, int>& agent_pos, const std::pair<int, int>& goal_pos, const std::pair<int, int>& start_pos, const std::vector<std::vector<int>>& grid_map, const std::vector<std::pair<int, int>>& agent_path, const std::map<std::pair<int, int>, int>* optimal_actions = nullptr) {
    //std::cout << "\n\n\n";
    std::cout << "\033[H\033[2J";
    std::cout << std::flush;
    //
    std::cout << "◽";
    for (int i = 0; i < width; ++i) {
        std::cout << "▪▪▪◽";
    }
    std::cout << std::endl;

    std::set<std::pair<int, int>> path_set;
    // no ultima pos
    if (!agent_path.empty()) {
        path_set.insert(agent_path.begin(), agent_path.end() - 1);
    }

    /**/
    for (int y = 0; y < height; ++y) {
        std::cout << "|";
        for (int x = 0; x < width; ++x) {
            std::pair<int, int> current_cell = {x, y};
            if (agent_pos.first == x && agent_pos.second == y) {
                std::cout << " 🤖 |"; // agente nella posizione corrente
            } 
	    else if (goal_pos.first == x && goal_pos.second == y) {
                std::cout << " ⛳ |"; // obiettivo
            } 
	    else if (start_pos.first == x && start_pos.second == y) { // controlla posizione di partenza
                 std::cout << " 🏠 |"; // partenza
            }
            else if (grid_map[y][x] == CELL_OBSTACLE) {
                 std::cout << " 🧱 |"; // ostacolo
            } 
	    else if (path_set.count(current_cell) && optimal_actions == nullptr) { // controlla se cella e' nel percorso (escluso la posizione corrente)
                 std::cout << " 🪙  |"; // parte del percorso
            }
	    else if (optimal_actions != nullptr && optimal_actions->count(current_cell) && path_set.count(current_cell)) { // mostra azione ottimale
                 int action = optimal_actions->at(current_cell);
                 if (action == 0) std::cout << " ⬆  |"; // su
                 else if (action == 1) std::cout << " ⬇  |"; // giu
                 else if (action == 2) std::cout << " ⬅  |"; // sinistra
                 else if (action == 3) std::cout << " ➡  |"; // destra
                 else std::cout << " ❓  |"; // azione sconosciuta
            }
            else {
                std::cout << " 🔹 |"; // cella vuota
            }
        }
        std::cout << std::endl;

        std::cout << "◽";
        for (int i = 0; i < width; ++i) {
            std::cout << "▪▪▪◽";
        }
        std::cout << std::endl;
    }
    std::cout << std::flush;
}

// save policy e valori Q
void save_learned_policy(const rl_agent& agent, const environment& env, const std::string& filename) {
    std::ofstream outfile(filename);

    if (!outfile.is_open()) {
        std::cerr << "errore apertura file " << filename << std::endl;
        return;
    }

    outfile << "apprendimento policy e Q\n";
    outfile << "----------------------------------\n";

    int width = env.get_width();
    int height = env.get_height();
    const auto& grid_map = env.get_grid_map();

    outfile << "griglia (" << width << "x" << height << "):\n";
    // rappresentazione semplice della mappa nel file
     for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid_map[y][x] == CELL_EMPTY) outfile << ".";
            else if (grid_map[y][x] == CELL_OBSTACLE) outfile << "#";
            else if (grid_map[y][x] == CELL_GOAL) outfile << "G";
            else if (grid_map[y][x] == CELL_START) outfile << "S"; // Aggiunto START
        }
        outfile << "\n";
     }
     outfile << "\n";


    // intestazione dati policy
    outfile << "X,Y,Azione Ottimale,Valore Q Ottimale,Q_Su,Q_Giu,Q_Sinistra,Q_Destra\n";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // non salvare la policy per le celle ostacolo
            if (grid_map[y][x] == CELL_OBSTACLE) {
                // DA RIVEDERE forse meglio indicare ostacolo
                // outfile << x << "," << y << ",OST.", ",,,,,\n"; // opzionale: marca gli ostacoli
                continue; // salta ostacolo
            }

            // stato 1D
            std::vector<double> state_representation = state_to_one_hot_flattened(x, y, width, height);

            // valori Q predetti
            std::vector<double> q_values = agent.get_q_values(state_representation);

            // trova azione con il valore Q massimo (politica appresa)
            int optimal_action = std::distance(q_values.begin(), std::max_element(q_values.begin(), q_values.end()));
            double optimal_q_value = q_values[optimal_action];

            // converti indice azione in una stringa
            std::string optimal_action_str;
            if (optimal_action == 0) optimal_action_str = "Su";
            else if (optimal_action == 1) optimal_action_str = "Giu";
            else if (optimal_action == 2) optimal_action_str = "Sinistra";
            else if (optimal_action == 3) optimal_action_str = "Destra";
            else optimal_action_str = "Sconosciuta";

            // salva i dati nel file
            outfile << x << "," << y << "," << optimal_action_str << "," << optimal_q_value;
            
            // salva anche tutti i valori Q per quella cella (opzionale ma utile)
            for (double qv : q_values) {
                outfile << "," << qv;
            }
            outfile << "\n";
        }
    }

    outfile.close();
    std::cout << "policy appresa e valori Q salvati in: " << filename << std::endl;
}

// caricare la policy da CSV
bool load_policy_from_csv(const std::string& filename, loaded_policy& policy_map, int& grid_width, int& grid_height) {
    std::ifstream infile(filename);

    if (!infile.is_open()) {
        std::cerr << "errore aprire file " << filename << std::endl;
        return false;
    }

    std::string line;

    // salta prime righe informative (intestazione, mappa ASCII)
    for(int i = 0; i < 4; ++i) { // salta le prime 4 righe (intestazione e bordo mappa)
        if (!std::getline(infile, line)) {
             std::cerr << "errore lettura intestazione o della mappa CSV" << std::endl;
             return false;
        }
         if (i == 2) { // dalla riga 3, leggi la mappa per determinare le dimensioni
            // questo fa schifo; un parsing migliore sarebbe leggere la mappa carattere per carattere
            grid_height = 0; 
            while(std::getline(infile, line) && (line.find('.') != std::string::npos || line.find('#') != std::string::npos || line.find('G') != std::string::npos || line.find('S') != std::string::npos)) {
                 if (grid_height == 0) grid_width = line.length(); 
                 grid_height++;
            }
            if (!std::getline(infile, line)) {
                 std::cerr << "errore lettura intestazione CSV." << std::endl;
                 return false;
            }
            break; 
        }
    }
    // leggo policy
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string segment;
        std::vector<std::string> segments;

        while(std::getline(ss, segment, ',')) {
            segments.push_back(segment);
        }

        // almeno 3 segmenti: X, Y, Azione Ottimale
        if (segments.size() >= 3) {
            try {
                int x = std::stoi(segments[0]);
                int y = std::stoi(segments[1]);
                std::string action_str = segments[2];
                int action_int = action_string_to_int(action_str);

                if (action_int != -1) {
                    policy_map[{x, y}] = action_int;
                } else {
                     std::cerr << "avviso: azione non riconosciuta '" << action_str << "'  posizione (" << x << "," << y << "). saltata." << std::endl;
                }

            } catch (const std::invalid_argument& e) {
                std::cerr << "avviso: impossibile convertire coordinate da riga: " << line << ". saltata." << std::endl;
            } catch (const std::out_of_range& e) {
                 std::cerr << "avviso: coordinate fuori range da riga: " << line << ". saltata." << std::endl;
            }
        } else {
             std::cerr << "avviso: riga CSV formato inatteso: " << line << ". saltata." << std::endl;
        }
    }

    infile.close();
    std::cout << "policy caricata da: " << filename << std::endl;
    return true;
}

/*std::map<std::pair<int, int>, std::string>*/
std::vector<std::string> execute_policy_test2(environment& env, loaded_policy& policy_map, std::string ip_esp = "") {
    std::cout << "esecuzione del test" << std::endl;
    /*std::map<std::pair<int, int>, std::string>*/
    std::vector<std::string> out;
    std::pair<int, int> state = env.reset();
    bool done = false;
    double test_reward = 0.0;
    int steps = 0;

    std::vector<std::pair<int, int>> test_path;
    test_path.push_back(state); 
    out = one_shot_navy_cmd(env.get_width(), env.get_height(), state, env.get_goal_pos(), env.get_start_pos(), env.get_grid_map(), test_path, &policy_map);
    const int max_test_steps = env.get_width() * env.get_height() * 4;

    std::cout << "posizione finale: (" << state.first << ", " << state.second << ")" << std::endl;
    std::cout << "posizione dell'obiettivo: (" << env.get_goal_pos().first << ", " << env.get_goal_pos().second << ")" << std::endl;
    std::cout << "posizione di partenza: (" << env.get_start_pos().first << ", " << env.get_start_pos().second << ")" << std::endl;
    return out;
}

// test policy load 
std::map<std::pair<int, int>, std::string> execute_policy_test(environment& env, loaded_policy& policy_map, std::string ip_esp = "") {
    std::cout << "esecuzione del test" << std::endl;
    std::map<std::pair<int, int>, std::string> out;
    std::pair<int, int> state = env.reset();
    bool done = false;
    double test_reward = 0.0;
    int steps = 0;

    std::vector<std::pair<int, int>> test_path;
    test_path.push_back(state); 
    /*out = */print_grid(env.get_width(), env.get_height(), state, env.get_goal_pos(), env.get_start_pos(), env.get_grid_map(), test_path, &policy_map);
    const int max_test_steps = env.get_width() * env.get_height() * 4;

    while (!done && steps < max_test_steps) {
        int action = -1; 
        auto it = policy_map.find(state);
        if (it != policy_map.end()) {
            action = it->second; 
        } else {
            std::cerr << "errore stato (" << state.first << "," << state.second << ") non trovato" << std::endl;
            break; 
        }
        // esegui azione ambiente
        auto [next_state, reward, is_done] = env.step(action);

        state = next_state; // aggiorna stato corrente
        done = is_done;
        test_reward += reward;
        steps++;

        // aggiungi nuova posizione al percorso
        test_path.push_back(state);
        /*out = */print_grid(env.get_width(), env.get_height(), state, env.get_goal_pos(), env.get_start_pos(), env.get_grid_map(), test_path, &policy_map);
 
	usleep(5*80000);
    }

    std::cout << "test con policy caricata completato in " << steps << " passi. ricompensa totale: " << test_reward << std::endl;
    std::cout << "posizione finale: (" << state.first << ", " << state.second << ")" << std::endl;
    std::cout << "posizione dell'obiettivo: (" << env.get_goal_pos().first << ", " << env.get_goal_pos().second << ")" << std::endl;
    std::cout << "posizione di partenza: (" << env.get_start_pos().first << ", " << env.get_start_pos().second << ")" << std::endl;
    return out;
}

std::vector<std::vector<int>> load_grid_from_numeric_file(const std::string& filename) {
    std::ifstream infile(filename);
    std::vector<std::vector<int>> grid_map;
    if (!infile.is_open()) {
        std::cerr << "errore file: " << filename << std::endl;
        return {};
    }
    std::string line;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::vector<int> current_row;
        int cell_value;
        // salta linee vuote
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) continue;
        while (ss >> cell_value) {
            current_row.push_back(cell_value);
        }
        if (!current_row.empty()) {
             grid_map.push_back(current_row);
        }
    }
    infile.close();
    if (grid_map.empty()) {
         std::cerr << "errore formato file" << std::endl;
         return {};
    }

    size_t expected_width = grid_map[0].size();
    for(size_t i = 1; i < grid_map.size(); ++i) {
        if (grid_map[i].size() != expected_width) {
            std::cerr << "errore len righe diverso nel file (riga 0: " << expected_width << ", riga " << i << ": " << grid_map[i].size() << ")" << std::endl;
            return {};
        }
    }
    std::cout << "griglia: " << filename << " ("<< grid_map[0].size() << "x" << grid_map.size() << ")"<< std::endl;
    return grid_map;
}
