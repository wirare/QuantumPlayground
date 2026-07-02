# Compiler and flags
CXX := clang++

OPT_FLAGS := -O3 -march=native -mtune=native -funroll-loops -fvectorize \
             -freciprocal-math -ffp-contract=fast \
             -fstrict-aliasing -flto=full -mprefer-vector-width=256 \
             -fomit-frame-pointer

CXXFLAGS := -std=c++23 -Wall -Werror -Wextra $(OPT_FLAGS)

# Directories
SRCDIR := srcs
OBJDIR := .objs

# Target binary
TARGET := Quantum

# VTK local install
VTK_PREFIX := $(HOME)/vtk/install
VTK_LIBDIR := $(VTK_PREFIX)/lib

# Auto-detect VTK major.minor suffix, e.g. 9.5
VTK_VERSION := $(shell ls $(VTK_LIBDIR)/libvtkCommonCore-*.so* 2>/dev/null | head -n 1 | sed -E 's|.*libvtkCommonCore-([0-9]+\.[0-9]+).*|\1|')

VTK_INCLUDEDIR := $(VTK_PREFIX)/include/vtk-$(VTK_VERSION)

# Includes
CPPFLAGS := -Iincludes -I$(VTK_INCLUDEDIR)

# Runtime path avoids needing LD_LIBRARY_PATH
LDFLAGS := -L$(VTK_LIBDIR) -Wl,-rpath,$(VTK_LIBDIR)

# Minimal VTK modules for:
# vtkRenderer, vtkRenderWindow, vtkRenderWindowInteractor,
# vtkActor, vtkPolyDataMapper, vtkSphereSource, vtkArrowSource,
# vtkPoints, vtkCellArray, vtkPolyData, vtkProperty, interactor style.
VTK_LIBS := \
	-lvtkInteractionStyle-$(VTK_VERSION) \
	-lvtkRenderingOpenGL2-$(VTK_VERSION) \
	-lvtkRenderingFreeType-$(VTK_VERSION) \
	-lvtkRenderingCore-$(VTK_VERSION) \
	-lvtkFiltersSources-$(VTK_VERSION) \
	-lvtkFiltersCore-$(VTK_VERSION) \
	-lvtkCommonColor-$(VTK_VERSION) \
	-lvtkCommonExecutionModel-$(VTK_VERSION) \
	-lvtkCommonDataModel-$(VTK_VERSION) \
	-lvtkCommonTransforms-$(VTK_VERSION) \
	-lvtkCommonMath-$(VTK_VERSION) \
	-lvtkCommonCore-$(VTK_VERSION) \
	-lvtksys-$(VTK_VERSION)

LDLIBS := $(VTK_LIBS)

# Source and object files
SRCFILES := $(wildcard $(SRCDIR)/*.cpp)
OBJFILES := $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCFILES))

# Default target
all: $(TARGET)

# Link object files into final binary
$(TARGET): $(OBJFILES)
	@echo "Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) $(OBJFILES) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

# Compile .cpp to .o
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Debug helper
print-vtk:
	@echo "VTK_PREFIX     = $(VTK_PREFIX)"
	@echo "VTK_VERSION    = $(VTK_VERSION)"
	@echo "VTK_INCLUDEDIR = $(VTK_INCLUDEDIR)"
	@echo "VTK_LIBDIR     = $(VTK_LIBDIR)"

# Clean build files
clean:
	rm -rf $(OBJDIR)

# Clean everything including binary
fclean: clean
	rm -f $(TARGET)

# Rebuild from scratch
re: fclean all

.PHONY: all clean fclean re print-vtk
