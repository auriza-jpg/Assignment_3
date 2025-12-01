#!/bin/bash

SOURCE="$1"
INPUT_ROOT="InputFiles"

declare -A PREFIXES=(
    ["IO"]="IO_"
    ["CPU"]="CPU_"
    ["Balanced"]="BAL_"
)

for cat in "${!PREFIXES[@]}"; do
    mkdir -p "$INPUT_ROOT/$cat"
done

current_name=""
current_cat=""
collecting=0
buffer=""

while IFS= read -r line; do
    if [[ "$line" =~ ^#TESTCASE[[:space:]]+(.+) ]]; then
        current_name="${BASH_REMATCH[1]}"
        collecting=1

        current_cat=""
        for cat in "${!PREFIXES[@]}"; do
            prefix="${PREFIXES[$cat]}"
            if [[ "$current_name" == ${prefix}* ]]; then
                current_cat="$cat"
                break
            fi
        done

        buffer=""
        continue
    fi

    if [[ "$line" =~ ^#END ]]; then
        if [ $collecting -eq 1 ] && [ -n "$current_cat" ]; then
            out_path="$INPUT_ROOT/$current_cat/${current_name}.txt"
            echo -n "$buffer" > "$out_path"
            echo "[OK] Wrote $out_path"
        fi
        collecting=0
        continue
    fi
    if [ $collecting -eq 1 ]; then
        trimmed="$(echo "$line" | sed 's/^[[:space:]]*//')"
        if [[ -n "$trimmed" && ! "$trimmed" =~ ^# ]]; then
            buffer+="$trimmed"$'\n'
        fi
    fi

done < "$SOURCE"

