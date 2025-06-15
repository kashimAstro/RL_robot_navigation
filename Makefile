train:
	g++ -Isrc/ src/train.cpp -o rl_train -I /usr/include/eigen3 -std=c++17 -O3 -lm -fopenmp
inference:
	g++ -DCURL_SAMPLE -Isrc/ src/inference.cpp -o rl_inference -I /usr/include/eigen3 -std=c++17 -O3 -lm -fopenmp -lcurl
map:
	g++ -Isrc/ src/create_map.cpp -o create_map -std=c++17 -O3 -lm
clean:
	rm rl_train -f
	rm rl_inference -f
	rm create_map -f
