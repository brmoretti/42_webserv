# Compiler
CXX := c++

# Directories
SRCDIR := src
INCDIR := include
BUILDDIR := build
NAMEDIR := bin

# NAME executable name
NAME := webserv

# Source files
SRCEXT := cpp
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))

# Object files
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))

# Includes
INC := -I$(INCDIR)

# Flags
CXXFLAGS = -std=c++98 -Wall -Wextra -Werror

# Final executable
EXECUTABLE := $(NAMEDIR)/$(NAME)

# Default make NAME
all: directories $(EXECUTABLE)

# Ensure build directories exist
directories:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(NAMEDIR)

# Rule for linking object files into executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $(EXECUTABLE)

# Rule for compiling source files into object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	$(CXX) $(CXXFLAGS) $(INC) -c -o $@ $<

# Clean build artifacts
clean:
	@rm -rf $(BUILDDIR)
	@rm -rf $(NAMEDIR)

# Run the executable
run: clean all
	@./$(EXECUTABLE) $(ARGS)

fclean: clean
	@rm -rf $(NAMEDIR)/$(NAME)

re: fclean all

test:
	@ echo "\nRunning test with no argument"
	@./$(EXECUTABLE)
	@ echo "\nRunning test with no file argument"
	@./$(EXECUTABLE) "nan"
	@ echo "\nRunning test with a valid file argument"
	@./$(EXECUTABLE) "data.csv"

val: clean all
	@valgrind ./$(EXECUTABLE) $(ARGS)

git: fclean
	@read -p "Enter commit message: " commit_message; \
    git add *; \
    git commit -m "$$commit_message"
	git push

# Phony NAMEs
.PHONY: all clean run fclean re git val
.DEFAULT_GOAL := all
