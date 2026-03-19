compile: parser.o scanner.o driver.o ast.o ast-print.o tac.o codegen.o
	gcc -o compile parser.o scanner.o driver.o ast.o ast-print.o tac.o codegen.o

parser.o: parser.c parser.h scanner.h ast.h
	gcc -Wall -g -c parser.c

scanner.o: scanner.c scanner.h
	gcc -Wall -g -c scanner.c

driver.o: driver.c scanner.h ast.h
	gcc -Wall -g -c driver.c

ast.o: ast.c ast.h
	gcc -Wall -g -c ast.c

ast-print.o: ast-print.c ast.h
	gcc -Wall -g -c ast-print.c

tac.o: tac.c tac.h ast.h
	gcc -Wall -g -c tac.c

codegen.o: codegen.c codegen.h tac.h ast.h parser.h
	gcc -Wall -g -c codegen.c

clean:
	rm -f compile *.o