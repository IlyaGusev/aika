# Chess engine GUI and tools

Based on:
* https://github.com/maksimKorzh/uci-gui/
* https://github.com/LeelaChessZero/lc0

# Build

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
