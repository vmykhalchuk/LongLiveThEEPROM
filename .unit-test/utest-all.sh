#!/bin/bash

# "nano" "nano-old" "mega" "mega-old"
AVR_BOARDS=("nano" "mega" "mega-old")

for ITEM in "${AVR_BOARDS[@]}"; do
    echo "-----------------------------------------------"
    echo "Running Unit Testing for Board: $ITEM"

    ./utest.sh $ITEM

    STATUS=$?

    if [ $STATUS -ne 0 ]; then
        echo "Failed. Aborting."
        exit $STATUS
    fi
done

echo "-----------------------------------------------"
echo "All Unit Tests Completed Successfully!"
