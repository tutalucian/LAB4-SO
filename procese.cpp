// Creeaza 3 procese separate (Linux si Windows):
//   procesul 1 asteapta 1 secunda, procesul 2 - 2 secunde, procesul 3 - 3 secunde.
// Procesul initial (parintele) asteapta sa se termine toate, apoi afiseaza mesajul final.
//
// Compilare:
//   Linux:   g++ procese.cpp -o procese
//   Windows: g++ procese.cpp -o procese.exe   (MinGW)   sau   cl /EHsc procese.cpp   (Visual Studio)

#include <iostream>
#include <string>
#include <cstdlib>     // atoi

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>    // fork, sleep, getpid
    #include <sys/wait.h>  // waitpid
#endif

const int NR_PROCESE = 3;

// Ce face fiecare proces copil
void lucreazaCopil(int nr)
{
    const char* unitate = (nr == 1) ? " secunda" : " secunde";

#ifdef _WIN32
    std::cout << "Procesul " << nr << " (PID " << GetCurrentProcessId()
              << ") a pornit, asteapta " << nr << unitate << "..." << std::endl;
    Sleep(nr * 1000);   // Sleep pe Windows primeste milisecunde
#else
    std::cout << "Procesul " << nr << " (PID " << getpid()
              << ") a pornit, asteapta " << nr << unitate << "..." << std::endl;
    sleep(nr);          // sleep pe Linux primeste secunde
#endif
    std::cout << "Procesul " << nr << " a terminat." << std::endl;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // Pe Windows nu exista fork(). Programul se porneste pe sine insusi de 3 ori,
    // cu un argument (1, 2 sau 3). Daca a primit argument, este un proces copil.
    if (argc > 1) {
        int nr = std::atoi(argv[1]);
        if (nr < 1 || nr > NR_PROCESE) {
            std::cerr << "Argument invalid: " << argv[1] << std::endl;
            return 1;
        }
        lucreazaCopil(nr);
        return 0;
    }

    std::cout << "Parintele (PID " << GetCurrentProcessId()
              << ") creeaza " << NR_PROCESE << " procese..." << std::endl;

    char caleExe[MAX_PATH];
    GetModuleFileNameA(NULL, caleExe, MAX_PATH);   // calea catre acest .exe

    PROCESS_INFORMATION pi[NR_PROCESE];
    HANDLE handles[NR_PROCESE];
    int create = 0;                                 // cate procese au pornit efectiv

    for (int i = 0; i < NR_PROCESE; i++) {
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        std::string comanda = "\"" + std::string(caleExe) + "\" " + std::to_string(i + 1);

        if (!CreateProcessA(NULL, &comanda[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi[i])) {
            std::cerr << "Eroare la CreateProcess: " << GetLastError() << std::endl;
            break;                                  // asteptam totusi procesele deja pornite
        }
        handles[i] = pi[i].hProcess;
        create++;
    }

    // Parintele asteapta toate procesele (TRUE = asteapta TOATE, nu doar unul)
    if (create > 0) {
        WaitForMultipleObjects(create, handles, TRUE, INFINITE);
    }

    for (int i = 0; i < create; i++) {
        CloseHandle(pi[i].hProcess);
        CloseHandle(pi[i].hThread);
    }

    if (create < NR_PROCESE) {
        return 1;
    }

#else
    (void)argc;                      // argumentele se folosesc doar pe Windows
    (void)argv;

    std::cout << "Parintele (PID " << getpid()
              << ") creeaza " << NR_PROCESE << " procese..." << std::endl;

    // Pe Linux folosim fork(): procesul se "cloneaza".
    // fork() intoarce 0 in copil si PID-ul copilului in parinte.
    pid_t pids[NR_PROCESE];
    int create = 0;                  // cate procese au pornit efectiv

    for (int i = 0; i < NR_PROCESE; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            std::cerr << "Eroare la fork()" << std::endl;
            break;                   // asteptam totusi copiii deja creati
        }
        if (pid == 0) {              // suntem in procesul copil
            lucreazaCopil(i + 1);
            _exit(0);                // copilul se opreste aici, nu continua bucla
        }
        pids[create++] = pid;        // suntem in parinte: retinem PID-ul copilului
    }

    // Parintele asteapta fiecare copil
    for (int i = 0; i < create; i++) {
        waitpid(pids[i], NULL, 0);
    }

    if (create < NR_PROCESE) {
        return 1;
    }
#endif

    std::cout << "Toate procesele au terminat!" << std::endl;
    return 0;
}
