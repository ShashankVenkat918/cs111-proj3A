build:
	gcc -o lab3a -g -Wall -Wextra lab3a.c

dist:
	tar -czvf lab3a-705303381.tar.gz README lab3a.c Makefile ext2_fs.h

clean:
	rm -f lab3a lab3a-705303381.tar.gz