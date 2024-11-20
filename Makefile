CC = gcc
CFLAGS = -Wall

SRC = src
INCLUDE = include
BIN = bin

SRC_FILES = $(wildcard $(SRC)/*.c)

# Targets
download: $(BIN)/download  

$(BIN)/download: $(SRC_FILES)
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCLUDE) -lm

install: $(BIN)/download
	@echo "Do you want to install, download as a global command? (y/n)"
	@read answer; \
	if [ "$$answer" = "y" ]; then \
		sudo cp $(BIN)/download /usr/local/bin; \
		echo "Installed."; \
	else \
		echo "Skipped installation."; \
	fi

uninstall:
	sudo rm -f /usr/local/bin/download           

clean:
	rm -f $(BIN)/download

