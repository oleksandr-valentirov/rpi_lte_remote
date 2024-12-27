# Вказуємо директорії, де розміщуються підкаталоги з програмами
SUBDIRS := servo_cam

# Загальні флаги компілятора
CFLAGS = -Wall -Werror -g
LIBS = -lwiringPi

# Правило для компіляції всіх програм
all: $(SUBDIRS)

# Компіляція кожної програми
$(SUBDIRS):
	$(MAKE) -C $@

# Правило для очищення всіх програм
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
