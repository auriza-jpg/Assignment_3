#!/bin/bash

#this is updated from assingment 1 code 
TEST_NAME="$1"

INPUT_ROOT="InputFiles"
OUTPUT_ROOT="OutputFiles/$TEST_NAME"

SIMS=("interrupts_EP" "interrupts_RR" "interrupts_EP_RR")
CATEGORIES=("IO" "CPU" "Balanced")

mkdir -p "$OUTPUT_ROOT"

# for each sim
for sim in "${SIMS[@]}"; do
    SIM_PATH="bin/$sim"

    if [ ! -f "$SIM_PATH" ]; then
        echo "$SIM_PATH not found" 
        continue
    fi

    echo "testing: $sim"

    # for each category 
    for category in "${CATEGORIES[@]}"; do
        
        INPUT_DIR="$INPUT_ROOT/$category"
        OUTPUT_DIR="$OUTPUT_ROOT/$sim/$category"

        mkdir -p "$OUTPUT_DIR"

        echo "Processing category: $category"

        # loop through each test per catagory 
        for inputfile in "$INPUT_DIR"/*; do
            
            filename=$(basename "$inputfile")
            echo "  - Running $filename"

            cp "$inputfile" test_input_file.txt

            # run the simulator with the test 
            "./$SIM_PATH" test_input_file.txt

            # check for execution.txt 
            if [ ! -f execution.txt ]; then
                echo "  execution.txt not found for $filename"
                rm -f test_input_file.txt
                continue
            fi

            output_path="$OUTPUT_DIR/${TEST_NAME}_${filename}_execution.txt"
            cp execution.txt "$output_path"
            rm test_input_file.txt
        done
    done
done


