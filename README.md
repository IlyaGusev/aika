# Aika
[![Tests Status](https://github.com/IlyaGusev/aika/actions/workflows/cpp.yml/badge.svg)](https://github.com/IlyaGusev/aika/actions/workflows/cpp.yml)

Amateur level C++ chess engine with web GUI on top of lc0 board representation

Based on:
* https://github.com/maksimKorzh/uci-gui/
* https://github.com/LeelaChessZero/lc0

# Build

Prerequisites: CMake, Boost
```
sudo apt-get install cmake libboost-all-dev build-essential libjsoncpp-dev uuid-dev
```

For MacOS:
```
brew install boost jsoncpp ossp-uuid
```

Build commands:
```
git clone https://github.com/IlyaGusev/aika/
cd aika
git submodule update --init --recursive
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..
```

# Run
```
cd build
./aika --root ../gui --search_config ../search_config.json
```

* Link (after running): http://127.0.0.1:8000/index.html
* Demo: http://ilyagusev.dev/aika/
