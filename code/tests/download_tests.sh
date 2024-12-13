#!/bin/bash

array=(
    "ftp://ftp.up.pt/pub/gnu/emacs/elisp-manual-21-2.8.tar.gz"
    "ftp://demo:password@test.rebex.net/readme.txt"
    "ftp://anonymous:anonymous@ftp.bit.nl/speedtest/100mb.bin"
    "ftp://anonymous:rcom%37@mirrors.up.pt/debian/README;type=d"
    "ftp://mirrors.up.pt/debian/README"
)

GREEN="\033[0;32m"
RED="\033[0;31m"
RESET="\033[0m"

TESTS=0

for url in "${array[@]}"; do

    download $url 

    result=$?  

    if [ $result -eq 0 ]; then
        TESTS=$((TESTS + 1)) 
        echo -e "${GREEN}TEST_SUCCESS for URL: $url${RESET}"
    else
        echo -e "${RED}TEST_FAILURE for URL: $url${RESET}"
    fi

    sleep 5

done

echo -e "${GREEN}Total tests passed: $TESTS/${#array[@]}${RESET}"
