import os, sys, csv, json, esptool

# User settings.
TARGET_PARTITION_INDEX = 0
PARTITIONS_CSV = "partitions.csv"
SETTINGS_JSON = ".vscode/settings.json"
PARTITION_BIN = "build/fs_r.bin"

# Derived settings.
TARGET_PARTITION_NAME = "fs_%u" % TARGET_PARTITION_INDEX

if __name__ == "__main__":
	assert os.path.exists(PARTITIONS_CSV)
	assert os.path.exists(SETTINGS_JSON)

	target_partition_offset = None
	target_partition_size = None

	with open(PARTITIONS_CSV, mode='r') as file:
		reader = csv.reader(file)

		for row in reader:
			if row and row[0].startswith('#'):
				continue

			if len(row) > 0 and row[0].strip() == TARGET_PARTITION_NAME:
				target_partition_offset = row[3].strip()
				target_partition_size = row[4].strip()
				break

	if(
		not target_partition_offset or
		not target_partition_size
	):
		exit(1)

	with open(SETTINGS_JSON, 'r') as file:
		settings = json.load(file)

	serial_port = settings["idf.portWin" if sys.platform == "win32" else "idf.port"]
	baud_rate = settings["idf.flashBaudRate"]

	exit(
		esptool.main([
			"-c", "esp32",
			"-p", serial_port,
			"-b", baud_rate,
			"read_flash",
			target_partition_offset,
			str(480 * 1024),
			PARTITION_BIN
		])
	)
