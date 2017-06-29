gendist: gendist.o
	clang -lstdc++ -lm -o gendist gendist.o
gendist.o: gendist.cpp
	clang -std=c++1z -c gendist.cpp
clean:
	rm gendist gendist.o
