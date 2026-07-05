# Builds src/FlyingToastersSaver.cpp (+ src/xpm.c) into a Haiku screen
# saver add-on. Run from the project root.

SRCDIR   = src
NAME     = FlyingToasters
TARGET   = $(SRCDIR)/$(NAME)
CXX      = g++
CXXFLAGS = -O2 -Wall -Wno-write-strings -fPIC $(shell pkg-config --cflags sdl2)
LIBS     = -lbe $(shell pkg-config --libs sdl2)

SRCS = $(SRCDIR)/FlyingToastersSaver.cpp $(SRCDIR)/xpm.c
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -shared -o $(TARGET) $(OBJS) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

install: $(TARGET)
	mkdir -p "$(HOME)/config/non-packaged/add-ons/Screen Savers"
	cp $(TARGET) "$(HOME)/config/non-packaged/add-ons/Screen Savers/$(NAME)"

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all install clean