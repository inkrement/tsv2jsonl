CXX = c++
CXXFLAGS = -pthread -std=c++17 -march=native -Wall -O3
OBJS = 
INCLUDES = -I.

tsv2jsonl: $(OBJS) src/main.cc
	$(CXX) $(CXXFLAGS) $(OBJS) src/main.cc -o tsv2jsonl

clean:
	rm -rf *.o tsv2jsonl

install:
	install tsv2jsonl /usr/local/bin

test: clean tsv2jsonl
	rm -f inc/example.json
	./tsv2jsonl -a inc/example.tsv inc/example.jsonl

.PHONY : clean install test