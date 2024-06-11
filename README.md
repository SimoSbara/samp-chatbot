# samp-shared-memory


## Description

A GTA SAMP plugin for IPC shared memory functionality.

## Concept

### Explanation
This plugin is created for better performance in the creation of a SAMP gamemode.

With this plugin you can distribuite works across other process in the operating system by using shared memory segments.

This approach could save time to the gamemode developer and provide better performance in some tasks.

Example:
* Instead of getting data from the samp gamemode, you could do a C# program that does that in a simplier way with the shared memory segment writing.
* You could do any intense calculation (even a neural network) in a separate program (with GPUs) and comunicate with the samp gememode by reading/writing onto a shared memory segment.
* Much more...

### Scheme
![alt text](https://simosbara.s-ul.eu/Z9tPgEKU)

### Performance Charts

Work in progress...

## Getting Started

### Compiling

* If you want to compile the project on Windows, you will need Visual Studio 2019 or 2022.
* If you want to compile the project on Linux, you will need to execute "make" in the global folder.
* Otherwise, there are also pre-compiled binaries in [Releases](https://github.com/SimoSbara/samp-shared-memory/releases).

### Installing

* Put sharedmemory.dll or .so in plugins/ samp server folder.
* Put shmem.inc in pawno/include/ samp server folder.

### Example of use

* [Examples](https://github.com/SimoSbara/samp-shared-memory/tree/main/Examples) in Python, C/C++ and Pawno gamemode (for now).

### Documentation
* Documentation can be found in the [wiki](https://github.com/SimoSbara/samp-shared-memory/wiki) section.

## Authors

[@SimoSbara](https://github.com/SimoSbara)

## License

This project is licensed under the GNU General Public License v3.0 - see the LICENSE.md file for details

## Acknowledgments

Project structure inspired from:
* [samp-dns-plugin by samp-incognito](https://github.com/samp-incognito/samp-dns-plugin)
