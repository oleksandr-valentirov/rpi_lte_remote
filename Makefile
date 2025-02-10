# Вказуємо директорії, де розміщуються підкаталоги з програмами
SUBDIRS := servo_cam

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
