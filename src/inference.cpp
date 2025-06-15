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

std::string path_file = "learned_policy.csv";
std::string grid_debug_select = "6x6";
std::string ip_esp8266 = "";
int step_lin = 2500;
int step_rot = 1000;

//cmd argv
void cmd_parser_argv(int argc, char ** argv) {
   const char* const short_opts = "hf:g:e:s:r:";
   const option long_opts[] = {
      {"help",     no_argument, nullptr, 'h'},
      {"load-file", required_argument, nullptr, 'f'},
      {"load-grid", required_argument, nullptr, 'g'},
      {"ip-esp",    required_argument, nullptr, 'e'},
      {"step-lin",  required_argument, nullptr, 's'},
      {"step-rot",  required_argument, nullptr, 'r'}
 
   };
   int opt;
   while((opt = getopt_long(argc, argv, short_opts, long_opts, nullptr)) != -1) {
      switch (opt) {
       case 'h':
	  std::cerr << "help: ./rl_inference --load-file file.csv --load-grid \"8x8\" --ip-esp 192.168.1.132 --step-lin 2500 --step-rot 1000" << std::endl;
	  exit(0);
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
             ip_esp8266 = std::string(optarg);
          break;
       case 's':
          if(optarg!=NULL)
             step_lin = atoi(optarg); 
	  break;
       case 'r':
          if(optarg!=NULL)
             step_rot = atoi(optarg); 
	  break;
       default:
	  std::cerr << "errore" << std::endl;
	  break;
      }
   } 
}

int main(int argc, char ** argv) {
    // mappe di debug con ostacoli 
    // 0: vuoto, 1: ostacolo, 2: obiettivo, 3: partenza
    cmd_parser_argv(argc, argv);
    std::vector<std::vector<int>> grid_map = load_grid_from_numeric_file(grid_debug_select);
    // carico griglia mappa
    environment env(grid_map);
    const int grid_width         = env.get_width();
    const int grid_height        = env.get_height();

    loaded_policy ldpolicy;
    int loaded_width = 0;
    int loaded_height = 0;

    // carico pesi, da terminare inutile ora.. 
    if (load_policy_from_csv(path_file, ldpolicy, loaded_width, loaded_height)) {
        // check grandezza mappa file 
        if (loaded_width == grid_width && loaded_height == grid_height) {
            // esegui test
            std::vector<std::string> out_esp = execute_policy_test2(env, ldpolicy, ip_esp8266);

            std::cerr << "comandi[ ";
    	    for (int y = 0; y < out_esp.size(); ++y) {
		std::cerr <<"("<<y<<") "<< out_esp[y] << " ";
		if(ip_esp8266!="")
		   curl_request_esp8266(ip_esp8266, out_esp[y], step_lin, step_rot);
	    }
            std::cerr << " ]\n\n\n\n\n";
        } else {
            std::cerr << "errore dimensione griglia file (" << loaded_width << "x" << loaded_height << ") " << 
		         "diversa da griglia ambiente (" << grid_width << "x" << grid_height << ")." << std::endl;
        }
    } else {
        std::cerr << "errore nel caricamento di: " << path_file << std::endl;
    }
    return 0;
}
