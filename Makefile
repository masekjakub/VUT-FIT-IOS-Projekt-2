all:
	gcc -std=gnu99 -Wall -Wextra -Werror -pedantic proj2.c -o proj2 -pthread
clean:
	rm proj2.out
	rm proj2