src_path := src
lib_path := lib

CXX := g++
CXXFLAGS := -ggdb -O0

libDNSResolver.so: $(src_path)/DNSMessage.cc $(src_path)/connection/connection.cc
	(mkdir -p $(lib_path) && $(CXX) $(CXXFLAGS) $^ -shared -fPIC -o $(lib_path)/$@ -I $(src_path))

resolver: $(src_path)/DNSResolver.cc libDNSResolver.so
	$(CXX) $(CXXFLAGS) $< -o $@ -I $(src_path) -I $(src_path)/connection -L $(lib_path) -lDNSResolver

client: $(src_path)/DNSClient.cc libDNSResolver.so
	$(CXX) $(CXXFLAGS) $< -o $@ -I $(src_path) -I $(src_path)/connection -L $(lib_path) -lDNSResolver

run_resolver: resolver
	LD_LIBRARY_PATH=$(lib_path) ./$<

run_client: client
	LD_LIBRARY_PATH=$(lib_path) ./$< $(ARGS)

tests: tests.cc libDNSResolver.so
	$(CXX) $(CXXFLAGS) $< -o $@ -I $(src_path) -I $(src_path)/connection -L $(lib_path) -lDNSResolver

run_tests: tests
	LD_LIBRARY_PATH=$(lib_path) ./tests

clean:
	rm -rf resolver client tests