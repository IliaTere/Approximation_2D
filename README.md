# 2D Function Approximation

This application provides a multithreaded implementation for approximating 2D functions with visualization capabilities.

## Features

- Multithreaded computation for improved performance
- Interactive control with keyboard shortcuts
- Three visualization modes:
  - Original function
  - Approximation function
  - Residual error
- Adjustable grid dimensions and visualization detail
- Zoom capabilities
- Support for multiple mathematical functions

## Building the Application

### Prerequisites

- C++ compiler (g++)
- Qt 5 development libraries
- pthread

### Build Commands

```bash
# Build both command-line and GUI versions
make

# Build only the command-line version
make a.out

# Build only the GUI version
make gui_app
```

### Cleaning

```bash
make clean
```

## Running the Application

### Command-line Version

```bash
./a.out a b c d nx ny k epsilon max_iterations threads
```

### GUI Version

```bash
./gui_app a b c d nx ny mx my k epsilon max_iterations threads
```

Or use the provided script with default parameters:

```bash
./run_gui.sh
```

### Parameters

- `a`, `b`: boundaries in x
- `c`, `d`: boundaries in y
- `nx`, `ny`: computational grid dimensions
- `mx`, `my`: visualization grid dimensions
- `k`: function number (0-7)
- `epsilon`: computation accuracy
- `max_iterations`: maximum number of iterations
- `threads`: number of parallel threads

## Keyboard Controls

- **0**: Switch between mathematical functions (0-7)
- **1**: Toggle visualization mode (function → approximation → error)
- **2**: Zoom in
- **3**: Zoom out (reset to original view)
- **4**: Increase grid dimensions (nx, ny) by 2x
- **5**: Decrease grid dimensions (nx, ny) by 2x (minimum 5)
- **6**: Increase accuracy parameter (epsilon)
- **7**: Decrease accuracy parameter (epsilon)
- **8**: Increase visualization detail (mx, my) by 2x
- **9**: Decrease visualization detail (mx, my) by 2x (minimum 5)

## Mathematical Functions

The application supports the following functions:

0. f(x, y) = 1
1. f(x, y) = x
2. f(x, y) = y
3. f(x, y) = x + y
4. f(x, y) = sqrt(x² + y²)
5. f(x, y) = x² + y²
6. f(x, y) = exp(x² - y²)
7. f(x, y) = 1/(25(x² + y²) + 1)

## Color Scheme

- Background: Light gray (#F0F0F0)
- Grid lines: Medium gray (#C0C0C0)
- Text and labels: Dark blue (#00008B)
- Borders and controls: Dark gray (#404040)
- Function visualization: Blue-Green-Red gradient
- Approximation visualization: Cyan-Orange gradient
- Residual error visualization: Green-Purple gradient 