CFLAGS = -O3 -pthread -mfpmath=sse -fstack-protector-all -g -W -Wall -Wextra -Wunused -Wcast-align -Werror -pedantic -pedantic-errors -Wfloat-equal -Wpointer-arith -Wformat-security -Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long -Woverloaded-virtual -Wnon-virtual-dtor -Wcast-qual -Wno-suggest-attribute=format

# Qt configuration
QT_INCLUDES = $(shell pkg-config --cflags Qt5Widgets Qt5Core Qt5Gui)
QT_LIBS = $(shell pkg-config --libs Qt5Widgets Qt5Core Qt5Gui)
QT_CFLAGS = $(CFLAGS) -fPIC
MOC = /usr/bin/moc

all: a.out gui_app

a.out:	main.o algorithm.cpp functions.cpp solution.cpp reduce_sum.cpp residual.cpp
		g++ $^ $(CFLAGS) -o $@

main.o: main.cpp all_includes.h matrix_operations.h parallel_utils.h function_types.h common_types.h
		g++ -c $(CFLAGS) main.cpp

# Build Qt GUI application
gui_app: gui_main.o algorithm.cpp functions.cpp solution.cpp reduce_sum.cpp residual.cpp window.o renderer.o moc_window.o moc_renderer.o
		g++ $^ $(QT_CFLAGS) $(QT_LIBS) -o $@

gui_main.o: gui_main.cpp window.hpp renderer.hpp all_includes.h
		g++ -c $(QT_CFLAGS) $(QT_INCLUDES) gui_main.cpp

window.o: window.cpp window.hpp renderer.hpp
		g++ -c $(QT_CFLAGS) $(QT_INCLUDES) window.cpp

renderer.o: renderer.cpp renderer.hpp
		g++ -c $(QT_CFLAGS) $(QT_INCLUDES) renderer.cpp

# MOC files for Qt
moc_window.cpp: window.hpp
		$(MOC) $(QT_INCLUDES) window.hpp -o moc_window.cpp

moc_renderer.cpp: renderer.hpp
		$(MOC) $(QT_INCLUDES) renderer.hpp -o moc_renderer.cpp

moc_window.o: moc_window.cpp
		g++ -c $(QT_CFLAGS) $(QT_INCLUDES) moc_window.cpp

moc_renderer.o: moc_renderer.cpp
		g++ -c $(QT_CFLAGS) $(QT_INCLUDES) moc_renderer.cpp

clean:
		rm -f *.out *.o gui_app moc_*.cpp
		