import os, sys, csv, json, esptool

PARTITIONS_CSV = "partitions.csv"
SETTINGS_JSON = ".vscode/settings.json"
PARTITION_NAME = "fs"
PARTITION_BIN = "build/%s.bin" % PARTITION_NAME

if __name__ == "__main__":
	assert os.path.exists(PARTITIONS_CSV)
	assert os.path.exists(SETTINGS_JSON)
	assert os.path.exists(PARTITION_BIN)

	target_partition_offset = None
	with open(PARTITIONS_CSV, mode='r') as file:
		reader = csv.reader(file)

		for row in reader:
			if row and row[0].startswith('#'):
				continue

			if len(row) > 0 and row[0].strip() == PARTITION_NAME:
				target_partition_offset = row[3].strip()
				break

	if(not target_partition_offset):
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
			"write_flash",
			target_partition_offset,
			PARTITION_BIN
		])
	)
