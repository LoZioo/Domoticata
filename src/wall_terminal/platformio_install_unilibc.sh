# #!/bin/bash
set -e

FILENAME="unilibc.txt"
REPO_URL="git@github.com:LoZioo/UniLibC.git"

rm -rf tmp

# Needed libraries to $libs.
mapfile -t libs < <(sed 's/\r$//' "$FILENAME")

git clone $REPO_URL tmp
mkdir -p lib

for i in "${!libs[@]}"; do

	# Copy everything but the `ul_configs` lib.
	if [[ "${libs[$i]}" != "configs" ]]; then
		rm -rf "lib/ul_${libs[$i]}"
		mv "tmp/lib/ul_${libs[$i]}" lib
	fi
done

# Do not overwrite the `ul_configs` lib but, if does not exist, use the default one.
if [[ ! -e "include/ul_configs.h" ]]; then
	mv "tmp/lib/ul_configs/ul_configs.h" include
fi

rm -rf tmp
