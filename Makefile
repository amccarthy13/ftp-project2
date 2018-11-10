pftp: pftp.o
		g++ pftp.o -o pftp

pftp.o: pftp.cpp
		g++ -c pftp.cpp

clean:
		rm *.o pftp
