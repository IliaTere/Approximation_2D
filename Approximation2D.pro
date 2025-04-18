QT += core gui widgets

TARGET = gui_app
TEMPLATE = app
CONFIG += console
CONFIG += c++11

# Флаги компиляции из исходного Makefile
QMAKE_CXXFLAGS += -O3 -pthread -mfpmath=sse -fstack-protector-all -g -W -Wall -Wextra -Wunused -Wcast-align -Werror -pedantic -pedantic-errors -Wfloat-equal -Wpointer-arith -Wformat-security -Wmissing-format-attribute -Wformat=1 -Wwrite-strings -Wcast-align -Wno-long-long -Woverloaded-virtual -Wnon-virtual-dtor -Wcast-qual -Wno-suggest-attribute=format

# Подключаем pthread
LIBS += -lpthread

# Исходные файлы
SOURCES += \
    gui_main.cpp \
    algorithm.cpp \
    functions.cpp \
    solution.cpp \
    reduce_sum.cpp \
    residual.cpp \
    window.cpp \
    renderer.cpp

# Заголовочные файлы
HEADERS += \
    all_includes.h \
    matrix_operations.h \
    parallel_utils.h \
    function_types.h \
    common_types.h \
    window.hpp \
    renderer.hpp

# Создаем также цель a.out (консольная версия)
a_out.target = a.out
a_out.commands = g++ main.cpp algorithm.cpp functions.cpp solution.cpp reduce_sum.cpp residual.cpp $(QMAKE_CXXFLAGS) -o a.out
a_out.depends = main.cpp

QMAKE_EXTRA_TARGETS += a_out

# Добавляем цель для make clean
clean.commands = $(DEL_FILE) $(TARGET) a.out *.o moc_*.cpp moc_*.h
QMAKE_EXTRA_TARGETS += clean 