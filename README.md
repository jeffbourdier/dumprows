# dumprows
Database Utility Map-Producing Read-Only Web Service

### Building on Linux

The following command should build DUMPROWS on Linux:

	sudo gcc -o /usr/local/bin/dumprows dumprows.c html.c geojson.c text.c jb.c

The executable file `dumprows` will be output into `/usr/local/bin/`.
