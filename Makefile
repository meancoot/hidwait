TARGET  := hidwait
CFLAGS  := -std=gnu99
LDFLAGS := -framework Foundation -framework IOKit
OBJECTS := src/hidwait.o

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) 

clean:
	rm -rf $(OBJECTS) $(TARGET)