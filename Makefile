bin=httpserver
cgi=test_cgi
cc=g++
LD_FLAGS=-std=c++11 -lpthread #-DDEBUG=1
curr=$(shell pwd)
src=main.cc

ALL:$(bin) $(cgi)
.PHONY:ALL

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

$(cgi):cgi/test_cgi.cc
	$(cc) -o $@ $^ -std=c++11

.PHONY:clean
clean:
	rm -f $(bin) $(cgi)
	rm -rf output

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp $(cgi) output/wwwroot
	cp ./cgi/shell_cgi.sh output/wwwroot
	cp ./cgi/python_cgi.py output/wwwroot
	cp ./cgi/test.html output/wwwroot
