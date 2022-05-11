M68K: always
	make -C src M68K
	gcc -o M68K src/M68K.a

clean:
	make -C src clean
	rm -fR M68K

always: ;
