all: adzip

adzip: adzip.c
	gcc adzip.c -o adzip -lm

clean:
	rm -f adzip
