PARSER=parser
SEARCHER=searcher
HTTP=httpserver
cc=g++

.PHONY:all
all:$(PARSER) $(SEARCHER) $(HTTP)
$(PARSER):parser.cc
	$(cc) -o $@ $^ -std=c++11 -lboost_filesystem -lboost_system

$(SEARCHER):server.cc
	$(cc) -o $@ $^ -std=c++11 -ljsoncpp

$(HTTP):http_server.cc
	$(cc) -o $@ $^ -std=c++11 -ljsoncpp -lpthread

.PHONY:clean
clean:
	rm -f $(PARSER) $(SEARCHER) $(HTTP)