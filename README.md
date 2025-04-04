# Umgebung-Arduino

Board definition to run [Umgebung](https://github.com/dennisppaul/umgebung) as a board from Arduino IDE or CLI.

*Umgebung* is a lightweight C++ environment for small audio and graphics based applications. It is inspired by Processing.org and the like.

==THIS PROJECT IS VERY EXPERIMENTAL==

please use the [issue tracking system](https://github.com/dennisppaul/umgebung-arduino/issues) to file feature requests and report issues.

currently this only works on macOS and has only been tested on:

```
Chipset Model:      Apple M3 Pro
ProductName:        macOS
ProductVersion:     15.3.2
```

## installation

### macOS

Make sure [Umgebung](https://github.com/dennisppaul/umgebung) is up and running.

navigate to *Arduino* folder:

```sh
cd $HOME/Documents/Arduino/ # on macOS, on Ubuntu Linux it might be `/home/dennisppaul/Arduino/`
```

check if `hardware` folder exists. the *Arduino* folder should look something like this:

```
├── hardware
└── libraries
```

if it does not exist, create it and enter it.

```sh
mkdir hardware
```

enter the `hardware` folder and check out the repository ( with submodules ):

```sh
cd hardware
git clone --recurse-submodules https://github.com/dennisppaul/umgebung-arduino
```

the *Arduino* folder should look something like this:

```
.
├── hardware
│   └── umgebung-arduino ( <<< cloned repository )
│       ├── LICENSE
│       ├── README.md
│       └── umgebung
└── libraries
```

if you have the Ardunio Command Line tool installed you can verifiy that umgebung is properly installed with:

```sh
arduino-cli board listall
```

this should produce an output including something similar to this:

```sh
Umgebung   umgebung-arduino:umgebung:UMG
```

now either restart Arduino IDE to use *Umgebung* or compile and run sketches with `arduino-cli` ( see *Installing Arduino CLI* below ) with e.g:

```sh
arduino-cli compile -u -b umgebung-arduino:umgebung:UMG ./umgebung-arduino/umgebung/examples/test
```

### Installing Arduino CLI

either install the Arduino Command Line Interface `arduino-cli` with a package manage e.g with `snap` with `sudo snap install arduino-cli` ( failed on Ubuntu 22.04.3 with UTM ) or follow the steps below:

```sh
cd $HOME
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
export PATH="$HOME/bin:$PATH" # temporary add  and should be added to startup file
```

now run the following command to test if everthing works:

```sh
arduino-cli --version 
```
