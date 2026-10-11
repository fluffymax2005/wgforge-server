#!/bin/bash

set -e

ENTITY_ARG="$1"
DBMS="${2:-sqlite}"
BASE_DIR=".."
OUTPUT_DIR="${BASE_DIR}/entities/generated"

if [[ -z "$ENTITY_ARG" ]]; then
    echo "Usage: bash preprocess_entities {entity|*} [DBMS] [FLAGS...]"
    exit 1
fi

ENTITIES=()
if [[ "$ENTITY_ARG" == "*" ]]; then
    ENTITIES=("allowedip" "peer")
else
    ENTITIES=("$ENTITY_ARG")
fi

if [[ -z "$3" ]]; then
    ODB_FLAGS=(
        --std c++17
        --generate-schema
        --generate-query
        --schema-format embedded
        --default-pointer std::shared_ptr
        --output-dir "$OUTPUT_DIR"
    )
else
    shift 2
    ODB_FLAGS=("$@")
fi

mkdir -p "$OUTPUT_DIR"

for ENTITY in "${ENTITIES[@]}"; do
    FILE_PATH="${BASE_DIR}/entities/${ENTITY}.hxx"

    if [[ ! -f "$FILE_PATH" ]]; then
        echo "Error: File $FILE_PATH does not exist!"
        exit 1
    fi

    echo "Processing entity '$ENTITY' for DBMS '$DBMS'..."

    odb -d "$DBMS" "${ODB_FLAGS[@]}" "$FILE_PATH"

    echo "Successfully generated code for $ENTITY"
done
