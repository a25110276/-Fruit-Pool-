CXX = g++
CXXFLAGS = -std=c++17 -Wall -Iinclude -IC:/msys64/mingw64/include
LDFLAGS = -LC:/msys64/mingw64/lib -lsfml-graphics -lsfml-window -lsfml-system -lbox2d

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

# Encuentra todos los .cpp en src/
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
# Genera los nombres de los .o
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SOURCES))
EXECUTABLE = $(BIN_DIR)/FruitPool.exe

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	$(EXECUTABLE)

clean:
	rm -rf $(OBJ_DIR) $(EXECUTABLE)