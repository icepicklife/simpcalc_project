CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -pedantic -O2
SRCDIR  = src
OBJDIR  = build
TARGET  = simpcalc

SOURCES = $(SRCDIR)/main.c \
          $(SRCDIR)/token.c \
          $(SRCDIR)/scanner.c \
          $(SRCDIR)/parser_stm.c \
          $(SRCDIR)/parser_exp.c

OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

test: $(TARGET)
	./$(TARGET) -d tests

clean:
	rm -rf $(OBJDIR) $(TARGET) tests/*_output_*.txt *_output_*.txt
