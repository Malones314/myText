kilo: kilo.c
	$(CC) $^ -o kilo -Wall -Wextra -pedantic -std=c99

clean:
	rm -f *.o
