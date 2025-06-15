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

// funzione di attivazione ReLU (comunemente usata negli strati nascosti)
double relu(double x) {
    return std::max(0.0, x);
}

// derivata della funzione di attivazione ReLU
double relu_derivative(double x) {
    return (x > 0.0) ? 1.0 : 0.0;
}

// funzione di attivazione lineare (usata nello strato di output per i valori Q)
double linear(double x) {
    return x;
}

// derivata della funzione di attivazione Lineare
double linear_derivative(double x) {
    return 1.0;
}

// una piccolissima
// rete neurale feedforward
// D.L 2025
class neural_network {
public:
    neural_network(int input_size, int hidden_size, int output_size)
        : input_size_(input_size), hidden_size_(hidden_size), output_size_(output_size) {
        
        // inizializzazione casuale dei pesi
        weights_ih_.resize(input_size_, hidden_size_);
        weights_ho_.resize(hidden_size_, output_size_);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dist(-0.5, 0.5); // inizializzazione piccola

        for (int i = 0; i < input_size_; ++i) {
            for (int j = 0; j < hidden_size_; ++j) {
                weights_ih_(i, j) = dist(gen);
            }
        }

        for (int i = 0; i < hidden_size_; ++i) {
            for (int j = 0; j < output_size_; ++j) {
                weights_ho_(i, j) = dist(gen);
            }
        }

        // inizializzazione dei bias
        bias_h_.resize(1, hidden_size_);
        bias_o_.resize(1, output_size_);

        for (int i = 0; i < hidden_size_; ++i) {
            bias_h_(0, i) = dist(gen);
        }
         for (int i = 0; i < output_size_; ++i) {
            bias_o_(0, i) = dist(gen);
        }
    }

    // forward pass
    std::vector<double> forward(const std::vector<double>& inputs) const {
        if (inputs.size() != input_size_) {
            throw std::runtime_error("Dimensione input non corrispondente!");
        }

        // input layer
        Eigen::VectorXd input_layer = Eigen::VectorXd::Map(inputs.data(), inputs.size());

        // hidden layer
        Eigen::VectorXd hidden_inputs = input_layer.transpose() * weights_ih_ + bias_h_;
        Eigen::VectorXd hidden_outputs = hidden_inputs.unaryExpr(&relu); // ReLU

        // output layer (valori Q)
        Eigen::VectorXd final_inputs = hidden_outputs.transpose() * weights_ho_ + bias_o_;
        Eigen::VectorXd final_outputs = final_inputs.unaryExpr(&linear); // lineare per i valori Q

        std::vector<double> outputs(output_size_);
        Eigen::VectorXd::Map(&outputs[0], output_size_) = final_outputs;
        
        return outputs;
    }

    // backward pass (per l'addestramento)
    void train(const std::vector<double>& inputs, const std::vector<double>& targets, double learning_rate) {
         if (inputs.size() != input_size_ || targets.size() != output_size_) {
            throw std::runtime_error("Dimensioni input/target non corrispondenti per il training!");
        }

        // forward pass (memorizzando gli output intermedi per il backward pass)
        Eigen::VectorXd input_layer = Eigen::VectorXd::Map(inputs.data(), inputs.size());
        
        Eigen::VectorXd hidden_inputs = input_layer.transpose() * weights_ih_ + bias_h_;
        Eigen::VectorXd hidden_outputs = hidden_inputs.unaryExpr(&relu);

        Eigen::VectorXd final_inputs = hidden_outputs.transpose() * weights_ho_ + bias_o_;
        Eigen::VectorXd final_outputs = final_inputs.unaryExpr(&linear); // Valori Q non attivati (linear)

        Eigen::VectorXd targets_eigen = Eigen::VectorXd::Map(targets.data(), targets.size());

        // calcolo errore output layer
        Eigen::VectorXd output_errors = targets_eigen - final_outputs;
        // gradiente output layer (derivata della funzione lineare e' 1)
        Eigen::VectorXd output_delta = output_errors.cwiseProduct(final_outputs.unaryExpr(&linear_derivative));

        // calcolo errore shidden layer
        Eigen::VectorXd hidden_errors = weights_ho_ * output_delta;
        // gradiente hidden layer (derivata della funzione ReLU)
        Eigen::VectorXd hidden_delta = hidden_errors.cwiseProduct(hidden_outputs.unaryExpr(&relu_derivative));

        // aggiornamento pesi bias
        weights_ho_ += learning_rate * hidden_outputs * output_delta.transpose();
        bias_o_ += learning_rate * output_delta.transpose();

        weights_ih_ += learning_rate * input_layer * hidden_delta.transpose();
        bias_h_ += learning_rate * hidden_delta.transpose();
    }

    // da terminare.
    bool load_weights(const std::string& filename) {
        std::ifstream infile(filename);
        if (!infile.is_open()) {
            return false; 
        }

        std::string line;
        std::string header;
        int rows, cols;
        double value; 

        // leggi weights_ih
        if(!std::getline(infile, header) || header != "# weights_ih") { 
	  std::cerr << "Errore formato file (weights_ih header) in " << filename << std::endl;
	  infile.close(); 
	  return false; 
	}

        if(!(infile >> rows >> cols) || rows != weights_ih_.rows() || cols != weights_ih_.cols()) { 
	  std::cerr << "Errore dimensioni (weights_ih) in " << filename << ". File: " << rows << "x" << cols << ", Rete: " << weights_ih_.rows() << "x" << weights_ih_.cols() << std::endl;
	  infile.close(); return false; 
	}
        std::getline(infile, line); 

        // leggi i valori elemento per elemento
        for(int i = 0; i < rows; ++i) {
            for(int j = 0; j < cols; ++j) {
                if (!(infile >> weights_ih_(i, j))) {
                    std::cerr << "Errore lettura dato elemento (" << i << "," << j << ") per weights_ih in " << filename << std::endl;
                    infile.close(); return false;
                }
            }
        }

        // leggi bias_h
        if(!std::getline(infile, header) || header != "# bias_h") { 
	  std::cerr << "Errore formato file (bias_h header) in " << filename << std::endl; 
	  infile.close();
	  return false; 
	}

        if(!(infile >> rows >> cols) || rows != bias_h_.rows() || cols != bias_h_.cols()) { 
          std::cerr << "Errore dimensioni (bias_h) in " << filename << ". File: " << rows << "x" << cols << ", Rete: " << bias_h_.rows() << "x" << bias_h_.cols() << std::endl; 
	  infile.close(); 
	  return false; 
	}
        std::getline(infile, line);

         for(int i = 0; i < rows; ++i) {
            for(int j = 0; j < cols; ++j) {
                if (!(infile >> bias_h_(i, j))) {
                     std::cerr << "Errore lettura dato elemento (" << i << "," << j << ") per bias_h in " << filename << std::endl;
                     infile.close(); return false;
                }
            }
        }

        // leggi weights_ho
        if(!std::getline(infile, header) || header != "# weights_ho") { 
          std::cerr << "Errore formato file (weights_ho header) in " << filename << std::endl; 
	  infile.close(); 
	  return false; 
	}
        if(!(infile >> rows >> cols) || rows != weights_ho_.rows() || cols != weights_ho_.cols()) { 
	  std::cerr << "Errore dimensioni (weights_ho) in " << filename << ". File: " << rows << "x" << cols << ", Rete: " << weights_ho_.rows() << "x" << weights_ho_.cols() << std::endl;
	 infile.close();
	 return false; 
	}
        std::getline(infile, line);

         for(int i = 0; i < rows; ++i) {
            for(int j = 0; j < cols; ++j) {
                if (!(infile >> weights_ho_(i, j))) {
                     std::cerr << "Errore lettura dato elemento (" << i << "," << j << ") per weights_ho in " << filename << std::endl;
                     infile.close(); return false;
                }
            }
        }


        // leggi bias_o
        if(!std::getline(infile, header) || header != "# bias_o") { 
	   std::cerr << "Errore formato file (bias_o header) in " << filename << std::endl; 
	   infile.close(); 
	   return false; 
	}
        if(!(infile >> rows >> cols) || rows != bias_o_.rows() || cols != bias_o_.cols()) { 
	  std::cerr << "Errore dimensioni (bias_o) in " << filename << ". File: " << rows << "x" << cols << ", Rete: " << bias_o_.rows() << "x" << bias_o_.cols() << std::endl; 
	  infile.close();
	  return false; 
	}
        std::getline(infile, line);

         for(int i = 0; i < rows; ++i) {
            for(int j = 0; j < cols; ++j) {
                if (!(infile >> bias_o_(i, j))) {
                     std::cerr << "Errore lettura dato elemento (" << i << "," << j << ") per bias_o in " << filename << std::endl;
                     infile.close(); return false;
                }
            }
        }

        infile.close();
        std::cout << "Pesi caricati con successo da: " << filename << std::endl;
        return true;
    }

    const Eigen::MatrixXd& get_weights_ih() const { return weights_ih_; }
    const Eigen::MatrixXd& get_weights_ho() const { return weights_ho_; }
    const Eigen::MatrixXd& get_bias_h() const { return bias_h_; }
    const Eigen::MatrixXd& get_bias_o() const { return bias_o_; }

    // output_size_ accessibile per l'agente
    int output_size_;


private:
    int input_size_;
    int hidden_size_;

    Eigen::MatrixXd weights_ih_; // pesi input a hidden
    Eigen::MatrixXd weights_ho_; // pesi hidden a output
    Eigen::MatrixXd bias_h_;     // bias hidden layer
    Eigen::MatrixXd bias_o_;     // bias output layer
};

