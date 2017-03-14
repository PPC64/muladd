.PHONY: all muladd debug clean perf

CC=gcc-6
MAIN_C=main.c
C_FILES=muladd.c muladd.h
DBG_FLAGS=-O0 -g
FLAGS=-O3 -Wall
OUT=main
ASM=asm
DBG=dbg
ASM_FLAG=-D_ASM
TST=test
ASM_TST=asm.$(TST)
NOASM_TST=noasm.$(TST)
TST_C=perf_test.c

all: muladd debug

muladd: $(ASM).$(OUT) $(OUT)

$(OUT): $(C_FILES) $(MAIN_C)
		$(CC) $(MAIN_C) $(C_FILES) -o $(OUT) $(FLAGS)

$(ASM).$(OUT): $(C_FILES) $(MAIN_C)
		$(CC) $(MAIN_C) $(C_FILES) -o $(ASM).$(OUT) $(FLAGS) $(ASM_FLAG)

debug: $(DBG).$(OUT) $(ASM).$(DBG).$(OUT)

$(DBG).$(OUT):$(C_FILES) $(MAIN_C)
		$(CC) $(MAIN_C) $(C_FILES) -o $(DBG).$(OUT) $(DBG_FLAGS)

$(ASM).$(DBG).$(OUT):$(C_FILES) $(MAIN_C)
		$(CC) $(MAIN_C) $(C_FILES) -o $(ASM).$(DBG).$(OUT) $(DBG_FLAGS) $(ASM_FLAG)

perf: $(NOASM_TST) $(ASM_TST)
	sh plot_results.sh "Muladd test" $(NOASM_TST) $(ASM_TST)

$(ASM_TST): $(C_FILES) $(TST_C)
		$(CC) $(C_FILES) $(TST_C) -o $(ASM_TST) $(FLAGS) $(ASM_FLAG)

$(NOASM_TST): $(C_FILES) $(TST_C)
		$(CC) $(C_FILES) $(TST_C) -o $(NOASM_TST) $(FLAGS)

clean:
		rm -f $(OUT) $(ASM).$(OUT) $(DBG).$(OUT) $(ASM).$(DBG).$(OUT) $(NOASM_TST) $(ASM_TST)
