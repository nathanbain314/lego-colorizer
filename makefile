CXX += -std=c++11 -stdlib=libc++ -O3

EXECUTABLE = lego_parse build_data build_circle color_image better_sphere

SRC = lib
INCLUDE = include
OBJDIR = build

CFLAGS += -I$(INCLUDE) -Wimplicit-function-declaration -Wall -Wextra -pedantic

LDLIBS = -lncurses
MAGICK_FLAGS = `Magick++-config --cxxflags --cppflags`
MAGICK_LIBS = `Magick++-config --ldflags --libs`
VIPS_FLAGS = `pkg-config vips-cpp --cflags --libs`

all: $(EXECUTABLE)

lego_parse: $(OBJDIR)/lego_parse.o $(OBJDIR)/progressbar.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDLIBS) $(MAGICK_FLAGS) $(MAGICK_LIBS) $(OBJDIR)/lego_parse.o $(OBJDIR)/progressbar.o -o lego_parse

build_data: $(OBJDIR)/build_data.o $(OBJDIR)/progressbar.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDLIBS) $(VIPS_FLAGS) $(OBJDIR)/build_data.o $(OBJDIR)/progressbar.o -o build_data	

color_image: $(OBJDIR)/color_image.o $(OBJDIR)/progressbar.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(LDLIBS) $(VIPS_FLAGS) $(OBJDIR)/color_image.o $(OBJDIR)/progressbar.o -o color_image	

build_circle: $(OBJDIR)/build_circle.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(MAGICK_FLAGS) $(MAGICK_LIBS) $(OBJDIR)/build_circle.o -o build_circle

better_sphere: $(OBJDIR)/better_sphere.o
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(OBJDIR)/better_sphere.o -o better_sphere

$(OBJDIR)/%.o: $(SRC)/%.c $(INCLUDE)/%.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJDIR)/lego_parse.o: $(SRC)/parse.c++
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(MAGICK_FLAGS) $< -o $(OBJDIR)/lego_parse.o
	
$(OBJDIR)/build_data.o: $(SRC)/color_data.c++
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(VIPS_FLAGS) $< -o $(OBJDIR)/build_data.o

$(OBJDIR)/color_image.o: $(SRC)/color_image.cpp
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(VIPS_FLAGS) $< -o $(OBJDIR)/color_image.o

$(OBJDIR)/build_circle.o: $(SRC)/circle.cpp
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $(MAGICK_FLAGS) $< -o $(OBJDIR)/build_circle.o

$(OBJDIR)/better_sphere.o: $(SRC)/better_sphere.c++
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CXXFLAGS) $< -o $(OBJDIR)/better_sphere.o

clean:
	rm -f $(OBJDIR)/*.o $(EXECUTABLE)
