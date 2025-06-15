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

bool save_network_weights(const neural_network& network, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Errore: impossibile aprire il file " << filename << " per salvare i pesi." << std::endl;
        return false;
    }

    // salva weights_ih
    outfile << "# weights_ih\n";
    outfile << network.get_weights_ih().rows() << " " << network.get_weights_ih().cols() << "\n";
    outfile << network.get_weights_ih() << "\n";

    // salva bias_h
    outfile << "# bias_h\n";
    outfile << network.get_bias_h().rows() << " " << network.get_bias_h().cols() << "\n";
    outfile << network.get_bias_h() << "\n";

    // salva weights_ho
    outfile << "# weights_ho\n";
    outfile << network.get_weights_ho().rows() << " " << network.get_weights_ho().cols() << "\n";
    outfile << network.get_weights_ho() << "\n";

    // salva bias_o
    outfile << "# bias_o\n";
    outfile << network.get_bias_o().rows() << " " << network.get_bias_o().cols() << "\n";
    outfile << network.get_bias_o() << "\n";

    outfile.close();
    std::cout << "Pesi della rete salvati in: " << filename << std::endl;
    return true;
}

// agente di rinforzo
class rl_agent {
public:
    rl_agent(int grid_width, int grid_height, double learning_rate, double discount_factor, double epsilon_start, double epsilon_end, double epsilon_decay)
        : grid_width_(grid_width), grid_height_(grid_height),
          learning_rate_(learning_rate), discount_factor_(discount_factor),
          epsilon_(epsilon_start), epsilon_end_(epsilon_end), epsilon_decay_(epsilon_decay),
          // dimensione input rete = width * height per la rappresentazione 1D
          q_network_(grid_width * grid_height, 20, 4) { // rete con 20 neuroni hidden, 4 output (azioni)
        
        std::random_device rd;
        rng_ = std::mt19937(rd());
    }

    // seleziono azione utilizzando la policy epsilon-greedy
    // capire altre tecniche 
    int select_action(const std::vector<double>& state_representation) {
        std::uniform_real_distribution<> dist(0.0, 1.0);
        if (dist(rng_) < epsilon_) {
            // esplorazione: azione casuale
            std::uniform_int_distribution<> action_dist(0, q_network_.output_size_ - 1);
            return action_dist(rng_);
        } else {
            // sfruttamento: azione con il massimo valore Q
            std::vector<double> q_values = q_network_.forward(state_representation);
            return std::distance(q_values.begin(), std::max_element(q_values.begin(), q_values.end()));
        }
    }

    // addestro rete Q con esperienza: (stato, azione, ricompensa, prossimo stato)
    void train(const std::vector<double>& state_representation, int action, double reward, const std::vector<double>& next_state_representation, bool done) {
        std::vector<double> target_q_values = q_network_.forward(state_representation);
        
        double max_next_q = 0.0;
        if (!done) {
            std::vector<double> next_q_values = q_network_.forward(next_state_representation);
            max_next_q = *std::max_element(next_q_values.begin(), next_q_values.end());
        }

        // aggiornamento Q-value target secondo l'equazione di Bellman:
        // Q(s, a) = r + gamma * max(Q(s', a'))
	// https://it.eitca.org/artificial-intelligence/eitc-ai-arl-advanced-reinforcement-learning/deep-reinforcement-learning/function-approximation-and-deep-reinforcement-learning/examination-review-function-approximation-and-deep-reinforcement-learning/what-is-the-bellman-equation-and-how-is-it-used-in-the-context-of-temporal-difference-td-learning-and-q-learning/
	// esplorare altre vie
        double target_q = reward + discount_factor_ * max_next_q;

        // aggiorno valore Q per azione che e' stata eseguita
        target_q_values[action] = target_q;

        // addestramento rete
        q_network_.train(state_representation, target_q_values, learning_rate_);
    }

    // aggiorna epsilon policy epsilon-greedy
    void update_epsilon() {
        epsilon_ = std::max(epsilon_end_, epsilon_ * epsilon_decay_);
    }

    // get valori Q per un dato stato
    std::vector<double> get_q_values(const std::vector<double>& state_representation) const {
        return q_network_.forward(state_representation);
    }

    // salva pesi rete dall'agente
    void save_weights(const std::string& filename) const {
        save_network_weights(q_network_, filename);
    }

    //carica pesi
    bool load_weights(const std::string& filename) {
        return q_network_.load_weights(filename);
    }

    //visibili 
    double epsilon_;
    double epsilon_end_;

private:
    int grid_width_;
    int grid_height_;
    double learning_rate_;
    double discount_factor_;
   
    double epsilon_decay_;

    neural_network q_network_;
    std::mt19937 rng_;
};
