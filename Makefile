CXX = clang++
CXXFLAGS = -std=c++17 -O2 -Wall
PKG_CONFIG = /opt/homebrew/bin/pkg-config
OPENCV_CFLAGS = $(shell $(PKG_CONFIG) --cflags opencv4)
OPENCV_LIBS = $(shell $(PKG_CONFIG) --libs opencv4)
TARGET = terminalcam

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) -o $(TARGET) main.cpp $(OPENCV_LIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
