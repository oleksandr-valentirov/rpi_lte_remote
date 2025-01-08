.DEFAULT_GOAL := all

# Вказуємо компілятор
CXX = g++

# Вказуємо флаги компілятора
CXXFLAGS = -Wall -Werror -g -std=c++11 `pkg-config --cflags opencv`

# Вказуємо директорії для включень
INCLUDE_DIRS = -I./inc/

# Вказуємо бібліотеки для лінкування
LIBS = `pkg-config --libs opencv`

# Вказуємо директорії для артефактів
BUILD_DIR = ../build/cam_subprocess

# Вказуємо вихідний файл
SRC_DIR = ./src
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)

# Описуємо результат компіляції
TARGET = $(BUILD_DIR)/cam_subprocess

# Команда для створення директорії для артефактів
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Компіляція всіх вихідних файлів
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

# Лінкування всіх об'єктних файлів у фінальний виконуваний файл
$(TARGET): $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES)) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

# Очистка об'єктних файлів та виконуваного файлу
clean:
	rm -rf $(BUILD_DIR)/*.o $(TARGET)

# За замовчуванням компілюємо програму
all: $(TARGET)
