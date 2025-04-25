FROM ubuntu:20.04

# Устанавливаем необходимые зависимости
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt5-default \
    qtbase5-dev \
    qtdeclarative5-dev \
    libxcb-xinerama0 \
    git \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Создаем директорию приложения
WORKDIR /app

# Копируем файлы проекта
COPY . .

# Собираем приложение
RUN mkdir -p build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Настраиваем переменные среды для запуска GUI
ENV QT_QPA_PLATFORM=offscreen

# Запускаем приложение при старте контейнера
CMD ["/app/build/Approximation_2D", "-1", "1", "-1", "1", "50", "50", "5", "1e-6", "100", "1"] 