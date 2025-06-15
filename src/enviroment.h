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

// costanti per la mappa
const int CELL_EMPTY = 0;
const int CELL_OBSTACLE = 1;
const int CELL_GOAL = 2;
const int CELL_START = 3;

// ambiente di rinforzo semplice 
// (griglia 2D con ostacoli)
//
class environment {
public:
    environment(const std::vector<std::vector<int>>& grid_map) : grid_map_(grid_map) {
        height_ = grid_map_.size();
        if (height_ > 0) {
            width_ = grid_map_[0].size();
        } else {
            width_ = 0;
        }

        // trova posizione iniziale e obiettivo sulla mappa
        bool start_found = false;
        bool goal_found = false;
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                if (grid_map_[y][x] == CELL_START) {
                    start_pos_ = {x, y};
                    current_pos_ = start_pos_; // posizione iniziale
                    start_found = true;
                } else if (grid_map_[y][x] == CELL_GOAL) {
                    goal_pos_ = {x, y}; // arrivo
                    goal_found = true;
                }
            }
        }

        if (!start_found || !goal_found) {
             throw std::runtime_error("mappa non valida: ci deve essere una (CELL_START) e una cella obiettivo (CELL_GOAL).");
        }
         // assicurati che la cella di partenza non sia anche un ostacolo o l'obiettivo 
	 // (anche se la logica sopra gia lo previene in pratica..)
         if (grid_map_[start_pos_.second][start_pos_.first] != CELL_START) {
              throw std::runtime_error("cella di partenza specificata non valida.");
         }
          if (grid_map_[goal_pos_.second][goal_pos_.first] != CELL_GOAL) {
              throw std::runtime_error("cella obiettivo specificata non valida.");
         }
    }

    // eesetta ambiente restituisce lo stato iniziale
    std::pair<int, int> reset() {
        current_pos_ = start_pos_;
        return current_pos_;
    }

    // esegue azione e restituisce il prossimo stato, la ricompensa e se l'episodio è finito
    // action: 0: Su, 1: Giù, 2: Sinistra, 3: Destra
    std::tuple<std::pair<int, int>, double, bool> step(int action) {
        std::pair<int, int> next_pos = current_pos_;
        double reward = 0.0;
        bool done = false;

        // calcola la prossima posizione candidata
        if (action == 0) { // su
            next_pos.second -= 1;
        } else if (action == 1) { // giu 
            next_pos.second += 1;
        } else if (action == 2) { // sinistra
            next_pos.first -= 1;
        } else if (action == 3) { // destra
            next_pos.first += 1;
        } else {
            // azione non valida
            reward = -1.0; // penalita' per azione non valida
        }

        // controlla i confini e gli ostacoli
        if (next_pos.first < 0 || next_pos.first >= width_ || next_pos.second < 0 || next_pos.second >= height_ ||
            grid_map_[next_pos.second][next_pos.first] == CELL_OBSTACLE) {
            
            // se la prossima posizione e' fuori dai confini o un ostacolo, 
	    // l'agente non si muove e riceve una penalità.
            reward = -1.0; // penalita per collisione
            next_pos = current_pos_; // la posizione rimane la stessa
        } else {
            // movimento valido
            current_pos_ = next_pos;

            // controlla se e' stata raggiunta la destinazione
            if (current_pos_ == goal_pos_) {
                reward = 10.0; // ricompensa per raggiungere l'obiettivo
                done = true;
            } else {
                reward = -0.01; // piccola penalita' per ogni passo valido (per incoraggiare percorsi brevi)
            }
        }
	//std::cerr << "------------" << std::endl;
	//std::cerr << current_pos_.first << " " << current_pos_.second << std::endl;
	//std::cerr << reward << std::endl;
	//std::cerr << done << std::endl;
	//std::cerr << "------------" << std::endl;

        return {current_pos_, reward, done};
    }

    std::pair<int, int> get_state() const {
        return current_pos_;
    }

    std::pair<int, int> get_goal_pos() const {
        return goal_pos_;
    }

     std::pair<int, int> get_start_pos() const { // ottenere la posizione di partenza
        return start_pos_;
    }


    int get_width() const { return width_; }
    int get_height() const { return height_; }
    int get_num_actions() const { return 4; } // su, giu sinistra, destra
    const std::vector<std::vector<int>>& get_grid_map() const { return grid_map_; }


private:
    int width_;
    int height_;
    std::pair<int, int> current_pos_;
    std::pair<int, int> start_pos_; // aggiunto punto di partenza per il reset
    std::pair<int, int> goal_pos_;
    std::vector<std::vector<int>> grid_map_; //mappa ambiente
};
