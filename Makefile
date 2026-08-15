CXX = g++
CXXFLAGS = -g -w -std=c++17 -Iinclude
LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
CPP_FILES = src/main.cpp
HEADER_FILES = include/Circuit.hpp include/Component.hpp include/LogicGates.hpp include/PointKey.hpp include/Solver.hpp include/Wire.hpp include/Oscilloscope.hpp include/Utils.hpp include/ToolMode.hpp include/AppState.hpp include/InputHandler.hpp include/Sidebar.hpp
TARGET = circuit_sim
all: $(TARGET)
$(TARGET) : $(CPP_FILES) $(HEADER_FILES)
	$(CXX) $(CPP_FILES) $(CXXFLAGS) $(LIBS) -o $(TARGET)
clean:
	rm -f $(TARGET)
