compile: visnake.c
	gcc visnake.c -o visnake -lncurses

run: visnake
	./visnake
