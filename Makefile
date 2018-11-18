pftp: pftp.o
		g++ pftp.o -o pftp -pthread

pftp.o: pftp.cpp
		g++ -c pftp.cpp -pthread

clean:
		rm *.o pftp
