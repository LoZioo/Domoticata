#!/bin/bash
set -e

FILENAME="unilibc.txt"
REPO_URL="git@github.com:LoZioo/UniLibC.git"

rm -rf tmp

# Needed libraries to $libs.
mapfile -t libs < <(sed 's/\r$//' "$FILENAME")

git clone $REPO_URL tmp
mkdir -p lib

for i in "${!libs[@]}"; do

	# Do not overwrite the `ul_configs` lib but, if does not exist, use the default one.
	if [[ "${libs[$i]}" != "configs" || ( "${libs[$i]}" == "configs" && ! -d "lib/ul_configs" ) ]]; then
		rm -rf "lib/ul_${libs[$i]}"
		mv "tmp/lib/ul_${libs[$i]}" lib
	fi
done

rm -rf tmp
