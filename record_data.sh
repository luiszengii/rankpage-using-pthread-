#!/bin/bash
echo "recorded data(added 5ms in each calculation): " > data.txt

FILES=$(ls tests)
for FILE in $FILES
do
    if [[ "$FILE" =~ ".in" ]]; then
        filename=${FILE%.*}
        echo -n "$filename: " >> data.txt
        (./pagerank < tests/$FILE) > /dev/null
        # ./pagerank < tests/$FILE
    fi
done
