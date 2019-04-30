n = main.o



run: $(n)
	gcc $(n) -o waski_most -pthread

clean:
	rm -f *.o*
	rm waski_most
