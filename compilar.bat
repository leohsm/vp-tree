@echo off
REM Compila a biblioteca, o exemplo e os testes no Windows (MinGW gcc), sem make.
gcc -std=c99 -Wall -Wextra -pedantic -O2 -Iinclude -c srcp_tree.c -o vp_tree.o || exit /b 1
ar rcs libvptree.a vp_tree.o || exit /b 1
gcc -std=c99 -Wall -Wextra -pedantic -O2 -Iinclude examplesxemplo.c -L. -lvptree -lm -o exemplo.exe || exit /b 1
gcc -std=c99 -Wall -Wextra -pedantic -O2 -Iinclude tests	este.c -L. -lvptree -lm -o teste_vp.exe || exit /b 1
echo Compilado: libvptree.a, exemplo.exe, teste_vp.exe
