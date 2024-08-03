#!/bin/bash
set -e

FILENAME="unilibc.txt"
REPO_URL="git@github.com:LoZioo/UniLibC.git"
TARGET="components/unilibc"

rm -rf tmp

# Check if the "unilibc.txt" file exists.
if [[ ! -e "unilibc.txt" ]]; then
	echo "\"unilibc.txt\" does not exist."
	exit 1
fi

# Needed libraries to $libs.
mapfile -t libs < <(sed 's/\r$//' "$FILENAME")

git clone $REPO_URL tmp

mkdir -p "$TARGET/include"
mkdir -p "$TARGET/src"

mv "tmp/lib/CMakeLists.txt" "$TARGET"

for i in "${!libs[@]}"; do

	# Do not overwrite the `ul_configs` lib but, if does not exist, use the default one.
	if [[ "${libs[$i]}" != "configs" || ( "${libs[$i]}" == "configs" && ! -e "$TARGET/include/ul_configs.h" ) ]]; then
		rm -rf "$TARGET/include/ul_${libs[$i]}.h"
		mv "tmp/lib/ul_${libs[$i]}/ul_${libs[$i]}.h" "$TARGET/include"

		if [[ "${libs[$i]}" != "configs" ]]; then
			rm -rf "$TARGET/src/ul_${libs[$i]}.c"
			mv "tmp/lib/ul_${libs[$i]}/ul_${libs[$i]}.c" "$TARGET/src"
		fi
	fi
done

rm -rf tmp
