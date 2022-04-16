all:
	gcc -std=gnu99 -Wall -Wextra -Werror -pedantic proj2.c -pthread -o proj2
er:
	gcc -std=gnu99 proj2.c -pthread -o proj2