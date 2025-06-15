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
#include <linux/limits.h>
#include <getopt.h>
#include <Eigen/Dense>

//#include "util.h"
#include "micro_nn.h"
#include "enviroment.h"
#include "agent.h"
#include "func.h"

std::string grid_debug_select = "6x6";
std::string path_file = "";
int g_num_epoch = 5000;
bool no_print = true;
bool stop_end = false;
double g_learning_rate   = 0.01;
double g_discount_factor = 0.95;
double g_epsilon_start   = 1.0;
double g_epsilon_end     = 0.05;
double g_epsilon_decay   = 0.998;

void print_help() {
      std::cerr << "Training robot 2d map obstacle" << std::endl;
      std::cerr << "\t--noprint    disable print grid" << std::endl;
      std::cerr << "\t--save-file  save file weights" << std::endl;
      std::cerr << "\t--load-grid  load file grid map" << std::endl;
      std::cerr << "\t--num-epochs num epochs training" << std::endl;
      std::cerr << "\t--learn-rate" << std::endl;
      std::cerr << "\t--epsilon-start" << std::endl;
      std::cerr << "\t--epsilon-end" << std::endl;
      std::cerr << "\t--decacy" << std::endl;
      std::cerr << "\t--factor" << std::endl;
      std::cerr << "\t--stop-end" << std::endl; 
      //std::cerr << "\t" << std::endl;
}

void cmd_parser_argv(int argc, char ** argv) {
   const char* const short_opts = "bhnf:g:e:l:a:s:d:y:k:";
   const option long_opts[] = {
      {"help",     no_argument, nullptr, 'h'},
      {"noprint",     no_argument, nullptr, 'n'},
      {"save-file", required_argument, nullptr, 'f'},
      {"load-grid", required_argument, nullptr, 'g'},
      {"num-epochs", required_argument, nullptr, 'e'},
      {"learn-rate", required_argument, nullptr, 'l'},
      {"epsilon-start", required_argument, nullptr, 's'},
      {"epsilon-end", required_argument, nullptr, 'd'},
      {"decacy", required_argument, nullptr, 'y'},
      {"factor", required_argument, nullptr, 'k'},
      {"stop-end", no_argument, nullptr, 'b'}
   };

   int opt;
   while((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
      switch (opt) {
       case 'h':
	  print_help();
	  exit(0);
          break;
       case 'b':
            stop_end = true;
	  break; 
       case 'k':
	  if(optarg!=NULL)
            g_discount_factor = atof(optarg);
	  break; 
       case 'l':
	  if(optarg!=NULL)
            g_learning_rate = atof(optarg);
	  break;
       case 's':
	  if(optarg!=NULL)
            g_epsilon_start = atof(optarg);
	  break;
       case 'd':
	  if(optarg!=NULL)
            g_epsilon_end = atof(optarg);
	  break;
       case 'y':
	  if(optarg!=NULL)
            g_epsilon_decay = atof(optarg);
	  break;
       case 'n':
	  no_print = false;
	  break;
       case 'f':
          if(optarg!=NULL)
             path_file = std::string(optarg);
          break;
       case 'g':
          if(optarg!=NULL)
             grid_debug_select = std::string(optarg);
          break;
       case 'e':
          if(optarg!=NULL)
             g_num_epoch = atoi(optarg);
          break;
 
       default:
	  print_help();
          break;
      }
   }
}

int main(int argc, char ** argv) {
    // definizione della mappa con ostacoli (esempio 5x5)
    // 0: vuota, 1: ostacolo, 2: obiettivo, 3: partenza
    cmd_parser_argv(argc,argv);
    if(grid_debug_select=="") {
      std::cerr << "errore seleziona griglia" << std::endl;
      return 0;
      }

    std::vector<std::vector<int>> grid_map = load_grid_from_numeric_file(grid_debug_select);
 
    std::cout << "params: " << std::endl;
    std::cout << "\tlearning_rate:   " << g_learning_rate << std::endl;
    std::cout << "\tdiscount_factor: " << g_discount_factor << std::endl;
    std::cout << "\tepsilon_start:   " << g_epsilon_start << std::endl;
    std::cout << "\tepsilon_end:     " << g_epsilon_end << std::endl;
    std::cout << "\tepsilon_decacy:  " << g_epsilon_decay << std::endl;
    std::cout << "\tmappa selezionata: " << grid_debug_select << " size(" << grid_map.size() << "x" << grid_map[0].size() << ")" << std::endl;

    if(path_file == "")
      path_file = "rl_weights_"+grid_debug_select+"map.csv";

    environment env(grid_map);

    const int grid_width         = env.get_width();
    const int grid_height        = env.get_height();
    const double learning_rate   = g_learning_rate;
    const double discount_factor = g_discount_factor;
    const double epsilon_start   = g_epsilon_start;
    const double epsilon_end     = g_epsilon_end;
    const double epsilon_decay   = g_epsilon_decay;
    const int num_episodes       = g_num_epoch; // aumentiamo episodi per imparare a evitare gli ostacoli
    const int print_interval     = 500;
    const int save_weights_ep    = 200; 
    const std::string weights_filename = "network_weights.txt";
    int ep_weigths_save          = 0; 

    rl_agent agent(grid_width, grid_height, learning_rate, discount_factor, epsilon_start, epsilon_end, epsilon_decay);
    int episode = 0;

    // inutile ora.. da finre
    std::cout << "tentativo di caricare pesi da: " << weights_filename << std::endl;
    if (agent.load_weights(weights_filename)) {
        std::cout << "pesi caricati con successo, ripresa addestramento." << std::endl;
        agent.epsilon_ = epsilon_end; // da finire
    } else {
        std::cerr << "impossibile caricare i pesi, inizio addestramento da zero." << std::endl;
    }

    std::cout << "inizio addestramento con ostacoli" << std::endl;
    while ((stop_end && agent.epsilon_ > agent.epsilon_end_ ) || episode < num_episodes) {
        std::pair<int, int> state = env.reset();
        std::vector<double> current_state_representation = state_to_one_hot_flattened(state.first, state.second, grid_width, grid_height);
        bool done = false;
        double total_reward = 0.0;
        int steps = 0;

        std::pair<int, int> final_episode_pos = state;
        const int max_steps_per_episode = grid_width * grid_height * 4;

        while (!done && steps < max_steps_per_episode) {
            // seleziona un'azione
            int action = agent.select_action(current_state_representation);
            // esegui azione nell'ambiente
            auto [next_state, reward, is_done] = env.step(action);
            // converti il prossimo stato in 1D
            std::vector<double> next_state_representation = state_to_one_hot_flattened(next_state.first, next_state.second, grid_width, grid_height);
            // addestra agente
            agent.train(current_state_representation, action, reward, next_state_representation, is_done);
            // aggiorna stato corrente
            current_state_representation = next_state_representation;
            state = next_state;
            done = is_done;
            total_reward += reward;
            steps++;
            final_episode_pos = state;
        }

        // aggiorna epsilon dopo ogni episodio
        agent.update_epsilon();
	episode++;

	//salvo pesi
        ep_weigths_save++;
        if (ep_weigths_save >= save_weights_ep) {
             agent.save_weights(weights_filename);
	     ep_weigths_save = 0; 
             std::cout << "\n--- salvataggio pesi al termine dell'episodio " << episode << " ---" << std::endl;
        }

        // stampa le informazioni episodio
        if ((episode + 1) % print_interval == 0 || episode == 0 || episode == num_episodes - 1) {
            std::cout << "\n--- fine episodio " << episode + 1 << " ---" << std::endl;
            std::cout << "passi: " << steps << ", ricompensa Totale: " << total_reward << ", epsilon: " << agent.epsilon_ << std::endl;
            std::cout << "posizione finale in questo episodio: (" << final_episode_pos.first << ", " << final_episode_pos.second << ")" << std::endl;
            // passo vettore vuoto per il percorso durante l'addestramento per non visualizzare il percorso completo
            if(no_print) print_grid(grid_width, grid_height, final_episode_pos, env.get_goal_pos(), env.get_start_pos(), env.get_grid_map(), {});
            std::cout << "------------------------" << std::endl;
        }
    }

    std::cout << "addestramento completato, auto-end su episodio: " << episode << std::endl;
    std::cout << "inizio test agente addestrato" << std::endl;

    // test agent
    agent.epsilon_                                   = 0.0;// disattiva esplorazione per test
    std::pair<int, int> state                        = env.reset();
    std::vector<double> current_state_representation = state_to_one_hot_flattened(state.first, state.second, grid_width, grid_height);
    bool done                                        = false;
    double test_reward                               = 0.0;
    int steps                                        = 0;

    // vettore per memorizzare il percorso durante il test
    std::vector<std::pair<int, int>> test_path;
    test_path.push_back(state); // pos iniziale 
    std::cout << "percorso durante il test:" << std::endl;
    print_grid(grid_width, grid_height, state, env.get_goal_pos(), env.get_start_pos(), env.get_grid_map(), test_path); // Stampa posizione iniziale del test
    const int max_test_steps = grid_width * grid_height * 4;
    while (!done && steps < max_test_steps) {
        int action = agent.select_action(current_state_representation);
        auto [next_state, reward, is_done] = env.step(action);
        current_state_representation = state_to_one_hot_flattened(next_state.first, next_state.second, grid_width, grid_height);
        state = next_state;
        done = is_done;
        test_reward += reward;
        steps++;

        test_path.push_back(state);
        if(no_print) print_grid(grid_width, grid_height, state, env.get_goal_pos(), env.get_start_pos(), env.get_grid_map(), test_path);
    }

    std::cout << "test completato in " << steps << " passi. ricompensa totale: " << test_reward << std::endl;
    std::cout << "posizione finale: (" << state.first << ", " << state.second << ")" << std::endl;
    std::cout << "posizione dell'obiettivo: (" << env.get_goal_pos().first << ", " << env.get_goal_pos().second << ")" << std::endl;
    std::cout << "posizione di partenza: (" << env.get_start_pos().first << ", " << env.get_start_pos().second << ")" << std::endl;

    // salvo lo stato della policy
    save_learned_policy(agent, env, path_file);

    return 0;
}
