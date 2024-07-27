# #!/bin/bash
set -e

FILENAME="unilibc.txt"
REPO_URL="git@github.com:LoZioo/UniLibC.git"

# Needed libraries to $libs.
mapfile -t libs < <(sed 's/\r$//' "$FILENAME")

git clone $REPO_URL tmp
mkdir -p lib

for i in "${!libs[@]}"; do
	rm -rf "lib/ul_${libs[$i]}"
	mv "tmp/lib/ul_${libs[$i]}" lib
done

rm -rf tmp
