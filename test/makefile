CXX=g++5
CXXSTD=c++14
DEPTH=512
RANDOM=0xAC0

driver: driver.cxx
	$(CXX) -o driver driver.cxx -I../src --std=$(CXXSTD) \
        -L/opt/local/lib/ -lboost_iostreams -lboost_serialization \
        -ftemplate-depth=$(DEPTH) -DRANDOM_SEED=$(RANDOM)
