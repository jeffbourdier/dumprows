DUMPROWS (Database Utility Map-Producing Read-Only Web Service) is a [CGI](https://en.wikipedia.org/wiki/Common_Gateway_Interface) program that allows a database to be queried using a web browser.

Home Page:  https://jeffbourdier.github.io/dumprows

### Building on Windows

It is easiest to build DUMPROWS on Windows from the Visual Studio solution (`dumprows.sln`) included with this repository.  If desired, DUMPROWS can be built from the **Developer Command Prompt** (Run as administrator!) as follows:

	cl dumprows.c html.c geojson.c text.c jb.c /link /OUT:"C:\Program Files (x86)\dumprows.exe"

The executable file `dumprows.exe` will be output into `C:\Program Files (x86)\`.  (If you want to run DUMPROWS without using the full path, `C:\Program Files (x86)\` can be added to the `PATH` environment variable.)

### Building on Linux

The following command should build DUMPROWS on Linux:

	sudo gcc -o /usr/local/bin/dumprows dumprows.c html.c geojson.c text.c jb.c

The executable file `dumprows` will be output into `/usr/local/bin/`.
