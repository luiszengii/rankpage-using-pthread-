#!/bin/bash
echo "failed tests: " > failed_tests.txt
FILES=$(ls tests)
for FILE in $FILES
do
    if [[ "$FILE" =~ ".in" ]]; then
        filename=${FILE%.*}
        # diff <(./pagerank < tests/$FILE) tests/${filename}".out" -y -W 50
        # (./pagerank < tests/$FILE) > /dev/null
        DIFF=$(diff <(./pagerank < tests/$FILE) tests/${filename}".out")

        if [ "$DIFF" != "" ]
        then
            echo "$filename: " >> failed_tests.txt 
            echo "$DIFF" >> failed_tests.txt
        fi
    fi
done
