gendist: gendist.o
	clang++ --std=c++1z -lm -o gendist gendist.o
gendist.o: gendist.cpp
	clang++ --std=c++1z -c gendist.cpp
clean:
	rm -f gendist gendist.o
