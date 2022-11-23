bin=httpserver
cc=g++
LD_FLAGS=-std=c++11 -lpthread #-DDEBUG=1
src=main.cc

ALL:$(bin) $(cgi)
.PHONY:ALL

$(bin):$(src)
	$(cc) -o $@ $^ $(LD_FLAGS)

.PHONY:clean
clean:
	rm -f $(bin) $(cgi)
	rm -rf output

.PHONY:output
output:
	mkdir -p output
	cp $(bin) output
	cp -rf wwwroot output
	cp -rf cgi/test_cgi output/wwwroot
