.DEFAULT_GOAL := all

# Вказуємо компілятор
CC = gcc

# Вказуємо флаги компілятора
CFLAGS = -Wall -Werror -g

# Вказуємо директорії для включень
INCLUDE_DIRS = -I./inc/

# Вказуємо бібліотеки для лінкування
LIBS = -lwiringPi -lconfuse

# Вказуємо директорії для артефактів
BUILD_DIR = ../build/servo_cam

# Вказуємо вихідний файл
SRC_DIR = ./src
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

# Описуємо результат компіляції
TARGET = $(BUILD_DIR)/servo_cam

# Команда для створення директорії для артефактів
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Компіляція всіх вихідних файлів
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Лінкування всіх об'єктних файлів у фінальний виконуваний файл
$(TARGET): $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES)) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Очистка об'єктних файлів та виконуваного файлу
clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET)

# За замовчуванням компілюємо програму
all: $(TARGET)
