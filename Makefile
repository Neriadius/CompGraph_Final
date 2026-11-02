# Snow Globe - Makefile (MSYS2 UCRT64)
#   pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-freeglut
#
#   make        build snowglobe.exe
#   make run    build and run
#   make clean  remove build files
# Run from this folder: shaders are loaded by relative path.

ifeq ($(origin CC),default)
CC := gcc
endif
ifeq ($(origin CXX),default)
CXX := g++
endif
CFLAGS   ?= -O2
CXXFLAGS ?= -O2 -Wall
CXXFLAGS += -std=c++17
CPPFLAGS += -Iglad/include -I.

EXE := .exe
FREEGLUT_STATIC_LIB := $(shell $(CXX) -print-file-name=libfreeglut_static.a)
ifneq ($(FREEGLUT_STATIC_LIB),libfreeglut_static.a)
CPPFLAGS += -DFREEGLUT_STATIC
LDFLAGS  += -static
LDLIBS   += -lfreeglut_static -lopengl32 -lwinmm -lgdi32
else
LDFLAGS  += -static-libgcc -static-libstdc++
LDLIBS   += -lfreeglut -lopengl32
endif

TARGET  := snowglobe$(EXE)
BUILD   := build
HEADERS := $(wildcard src/*.h)

OBJS := $(BUILD)/main.o $(BUILD)/glad.o \
        $(BUILD)/shaderClass.o $(BUILD)/VAO.o $(BUILD)/VBO.o $(BUILD)/EBO.o $(BUILD)/Camera.o

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD)/glad.o: glad/src/glad.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD)/main.o: main.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD)/%.o: src/%.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

run: $(TARGET)
	@./$(TARGET)

clean:
	rm -rf $(BUILD) snowglobe snowglobe.exe
