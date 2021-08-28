# Parallel-algorithms-in-distributed-systems

Homework from 'Parallel and Distributed Algorithms' course

                             Tema #3 - APD
                    Algoritmi paraleli in sisteme distribuite

  Am implementat un procesator de text distribuit care trebuie sa transforme
textul primit, aplicand diverse reguli pe cuvinte in functie de genul unui
paragraf. Pentru asta, am folosit un program bazat pe 5 noduri de MPI, un
nod MASTER si 4 noduri WORKER, pt fiecare gen in parte. Nodul MASTER ruleaza
exact 4 fire de executie Pthreads pt a citi paragrafele din fisierul de intrare
(asadar acesta e deschis de 4 ori in functia de thread si inca o data in main).
Acesta comunica cu cei 4 workeri in paralel prin intermediul celor 4
thread-uri. Cei 4 workeri primesc paragrafele corespunzatoare genului lor si
le trimit inapoi catre MASTER procesate. In cele din urma din MASTER se vor
scrie paragrafele procesate in fisierul de iesire.

  << Dovada scalabilitate solutie >>

  La o simpla rulare a checker-ului, timpii seriali vs timpii de rulare
  cod distribuit sunt: (timpii sunt mai mari pentru ca am rulat pe masina
  virtuala de linux; am modificat checker.py pt afisarea de mai jos):

  ~~~~~~~~~~~~~~~~~~~ DOES IT SCALE? ~~~~~~~~~~~~~~~~~~
Serial : 0.0015642642974853516 vs Distributed : 0.325702428817749
Serial : 0.33673882484436035 vs Distributed : 0.6296436786651611
Serial : 0.9984748363494873 vs Distributed : 1.1533401012420654
Serial : 17.93897557258606 vs Distributed : 13.616210699081421
Serial : 25.83383083343506 vs Distributed : 19.01471972465515
IT DOES :)
SCORE = 10


  Implementarea etapelor de send, receive si procesare, descrise mai sus,
s-a realizat in felul urmator:

  - am considerat pentru retinerea paragrafelor vectori de string-uri, fiecare
  string reprezentand o linie din paragraful respectiv, acestora fiindu-le
  atribuite dimensiuni specifice in functie de limitele fisierelor de intrare

  - in main consider un vector de 4 thread-uri si initializez bariera necesara
  sincronizarii acestora; in cazul nodului MASTER, acesta preia numele
  fisierului de intrare ca argument din linia de comanda, prelucreaza numele
  fisierului de iesire ce urmeaza a fi creat si deschide/creeaza aceste 2
  fisiere; programul se asigura ca cele 2 fisiere exista, iar in caz afirmativ,
  se parcurge fisierul de intrare pe linii si se contorizeaza numarul de
  paragrafe din fiecare gen in 4 variabile diferite, acestea fiind trimise
  workerilor corespunzatori pe rand

  - tot in main, pt nodul MASTER, creez si pornesc cele 4 thread-uri; dupa
  ce acestea termina de prelucrat citirea in paralel din fisierul de intrare
  si primirea paragrafelor procesate de la workeri, sunt afisate in fisierul
  de iesire din vectorul de stringuri considerat pentru stocarea acestora,
  in ordinea in care au aparut inital in fisierul de intrare

  - pt fiecare thread in parte am considerat o functie de thread in care
  se discuta in parte caracterul executiei fiecarui dintre cele 4 thread-uri;
  totusi fiecare thread va deschide fisierul de intrare exact 1 data

  - desi caracterul executiei thread-urilor este diferit, fiecarui thread
  fiindu-i asociat un anumit gen, logica este aceeasi, asadar este suficienta
  descrierea comportamentului unui singur thread, celelalte imitand acest
  tip de comportament, doar fiindu-i asociat alt gen

  - thread-ul cu id-ul 0 asociat genului horror se foloseste de o variabila
  careia i se poate atribui doar 0 sau 1 si care comunica faptul ca s-a gasit
  sfarsitul de fisier (1) sau nu (0); acesta parcurge fisierul pe linii si
  foloseste functia 'getline' pentru a citi cate o linie pe rand, dar si pentru
  a-i atribui valoarea corecta variabilei care marcheaza sau nu sfarsitul
  fisierului de intrare; verifica la fiecare citire a liniei daca aceasta
  coincide cu tipul paragrafului cautat (in acest caz 'horror') si retine
  faptul ca urmeaza a fi citite liniile unui astfel de paragraf pana la un
  anumit index de linie, retinandu-se numarul liniei din fisierul de intrare
  de la care incepe acesta pt pastrarea ordinii la afisarea paragrafelor in
  fisierul de iesire; se retine numarul de caractere al liniei fara caracterul
  terminator, iar daca acesta este 0 sau s-a gasit 'eof' inseamna ca s-a
  terminat de citit un paragraf, inclusiv ultimul din fisier (din cauza
  considerarii ca s-a gasit 'eof'); daca inca se mai citesc linii dintr-un
  paragraf de tip horror se adauga linia in vectorul de sring-uri asociat
  acestuia, linie careia i se sterge ultimul caracter, in cazul in care acesta
  este 'new line' (se vor afisa separat '\n' in fisierul de iesire dupa linii);
  in caz contrar, cand citirea paragrafului respectiv s-a terminat, acesta
  devine eligibil pentru trimiterea catre worker-ul corespondent, aici HORROR
  (rank = H), in cazul in care nr de linii al acestuia nu este vid (se trimite si
  acesta impreuna cu paragraful); apoi se primeste paragraful procesat din
  worker si se adauga in vectorul de stringuri considerat pentru tot textul
  procesat, de la linia de la care a inceput si in fisierul de intrare, salvata
  anterior in thread; la sfarsit pentru a asigura o buna gestionare a
  paragrafelor (pt a nu exista interferente de caractere/linii intre ele),
  acestea se vor goli; asemenea functioneaza si celelalte 3 thread-uri
  asociate genurilor comedy (id = 1), fantasy (id = 2) si sf (id = 3) numai
  ca se vor prelucra paragrafele conform genului respectiv (exemplu: in cazul
  comedy, se cauta paragrafele comedy, se trimit workerului COMEDY (rank = C)
  etc.)

  - intre thread-uri exista cate o bariera folosita pentru a astepta terminarea
  thread-ului anterior, astfel incat fiecare trebuie sa se asigure ca thread-ul
  de dinainte a terminat de trimis si primit inapoi toate paragrafele genului sau

  - in main, in cazul workerilor, acestia primesc intai numarul de paragrafe
  de genul sau din MASTER, dupa care primesc paragrafele din thread-urile
  corespunzatoare (H de la 0, C de la 1, F de la 2 si SF de la 3), le proceseaza
  in functie de gen pe linii, conform regulilor din cerinta (existand o procesare
  distincta pt fiecare in parte); dupa procesarea paragrafului respectiv,
  workerul trimite paragraful procesat catre MASTER

