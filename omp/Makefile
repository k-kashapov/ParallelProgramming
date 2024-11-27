CC=g++
CCFLAGS=-O3 -fopenmp -ggdb3 -fno-omit-frame-pointer
SRC=omp.c
BIN=omp.elf
ASANFLAGS=-fsanitize=address,undefined
TSANFLAGS=-fsanitize=thread,undefined

.PHONY: test

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(SRC) $(CCFLAGS) -o $(BIN)

asan:
	$(CC) $(SRC) $(CCFLAGS) -o $(BIN) $(ASANFLAGS)

tsan:
	$(CC) $(SRC) $(CCFLAGS) -o $(BIN) $(TSANFLAGS)

test: all
	@printf "Running with $(OMP_NUM_THREADS) threads\n"
	@time ./omp.elf 5000 5000 res$(OMP_NUM_THREADS).txt

clean:
	rm -rf $(BIN)
