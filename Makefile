COMMON_FLAGS = -std=c++17 -Wall -I./include -DWINVER=0x0601 -D_WIN32_WINNT=0x0601
COMMON_LDFLAGS = -static -static-libgcc -static-libstdc++ -lurlmon -lAdvapi32 -lcomdlg32 -lshell32 -lole32 -loleaut32

CXX = g++
CXXFLAGS = $(COMMON_FLAGS)
LDFLAGS = $(COMMON_LDFLAGS)

SRC_DIR = src
BUILD_DIR = build/x64
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(SRC_DIR)/miniz.c
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.cpp)) $(BUILD_DIR)/miniz.o
TARGET = AndroidUtility_x64.exe

.PHONY: all clean

all: $(TARGET)
	@echo "64-bit build complete. Generated: $(TARGET)"

$(BUILD_DIR):
	if not exist build mkdir build
	if not exist build\x64 mkdir build\x64

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/miniz.o: $(SRC_DIR)/miniz.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(BUILD_DIR) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

clean:
	if exist build rmdir /s /q build
	if exist $(TARGET) del /f /q $(TARGET)
	if exist AndroidUtility_x86.exe del /f /q AndroidUtility_x86.exe