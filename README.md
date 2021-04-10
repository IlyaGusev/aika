# Aika

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
git clone https://github.com/IlyaGusev/ChessEngineTemplate/
cd ChessEngineTemplate
git submodule update --init --recursive
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4 && cd ..
```

# Run
```
cp build/chess_engine chess_engine
./chess_engine
```

Link: http://127.0.0.1:8000/index.html
