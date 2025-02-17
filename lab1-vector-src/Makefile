.PHONY: all clean

CXX = g++
CXXFLAGS = -Wall -pedantic -O2 -mavx2 -lpthread
OBJDIR = obj

all: $(OBJDIR)/datagen $(OBJDIR)/traverse

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/datagen: datagen.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ datagen.cpp

$(OBJDIR)/traverse: main.cpp tree.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ main.cpp tree.cpp

clean:
	rm -rf $(OBJDIR)
