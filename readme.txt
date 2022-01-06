Nume: Razvan-Andrei Matisan
Grupa: 324CA

                                Tema 2 PC - Aplicatie Client - Server


    I. Implementare server

    -> am folosit multiplexarea I/O pentru a manipula inputurile venite de la
    UDP, subscriberii TCP sau de la stdin;

    -> astfel, am creat socketi pentru:
        * conexiunea UDP
        * conexiunea TCP (este un socket pasiv, adica asculta cererile de 
        conectare venite din partea clientilor TCP).
    
    -> serverul ruleaza pana cand primeste de la stdin "exit". In acel
    moment, toate socket-urile se vor inchide si ele.   
    
    -> in ceea ce priveste conectarea clientilor TCP la server, dupa ce
    socketul TCP pasiv accepta cererea de conectare, serverul verifica
    daca clientul cu acelasi id este deja conectat sau nu. 
        * in cazul in care este deja conectat, se va afisa un mesaj
        care sa anunte acest lucru
        * in cazul in care nu este conectat (adica se conecteaza pt
        prima data SAU se reconecteaza) => se adauga in structurile
        de date care retin clientii TCP online. Acestea sunt:
            1. un unordered set cu toate id-urile clientilor online
            (O(1) timp mediu de cautare).
            2. un unordered_map care mapeaza un socket TCP cu id-ul sau.
            (pt cautare in O(1)).
            3. un unordered_map care mapeaza un id cu un socket TCP.
            (pt cautare in O(1)).
        In plus, se trimit mesajele salvate pt topicuri cu sf=1 la care
        s-a abonat clientul offline. Toate aceste mesaje au fost salvate
        intr-un unordered_map care mapeaza id-ul unui client OFFLINE cu
        un vector de mesaje ce trebuie trimise mai departe.

    -> in ceea ce priveste retinerea informatiilor despre abonatii la topicuri,
    este bine sa amintesc faptul ca aceasta se face intr-o structura ceva mai
    complexa: un unordered_map care mapeaza un topic la un alt unordered_map,
    care mapeaza, la randul lui, id-ul clientului dupa sf.

    -> astfel, ceea ce se intampla la abonare este de a adauga/actualiza
    mapul de la topicul cerut astfel:
        * daca am operatie de subscribe -> creez/updatez intrarea respectiva
        cu noul sf (O(1))
        * daca am operatie de unsubscribe -> sters acel id din map-ul asignat
        topicului de la care vrea sa se dezaboneze. (O(1))

    -> daca un client TCP vrea sa se deconecteze, el va primi 0 bytes de la 
    acel client (insemnand ca subscriber-ul a dat comanda exit, insa nu 
    a primit nimic server ul). Ceea ce se intampla aici este ca se va elimina
    id-ul/socketul coresp. din toate structurile de date care retin clientii
    online, pe care le am amintit ceva mai sus. Apoi, se va scoate din set-ul
    de socketi activi si se va da close, urmand sa se recalculeze si fdmax-ul.  

    -> daca primesc un mesaj UDP, atunci imi voi crea o structura statica
    de tip mesaj TCP (ambele definite in utils.h) pentru a trimite mai 
    departe clientilor TCP abonati la topicul dat de UDP.
    Dupa ce se efectueaza parsarea conform enuntului, distingem 2 cazuri la
    trimitere, in functie de client:
        1. clientul este online => send message
        2. clientul este offline, dar este abonat la topic cu sf = 1 => 
        store message (in map-ul despre care am vorbit anterior, care
        mapeaza id-ul clientului offline cu lista de mesaje pe care trebuie
        sa le primeasca la reconectare).

-------------------------------------------------------------------------------    

    II. Subscriber

    -> se creeaza un socket de comunicare cu serverul pentru care se
    dezactiveaza algoritmul lui Neagle.

    -> subscriber-ul ruleaza pana cand primeste comanda "exit" sau pana
    cand inchide serverul conexiunea.

    -> acesta primeste de la stdin 3 comenzi:
        * exit -> inchide conexiunea subscriber-ului.
        * subscribe <TOPIC> <SF> -> clientul TCP doreste sa se conecteze la
        la un topic, avand un anumit SF. Aceasta comanda se parseaza, iar
        daca respecta formatul din enunt, atunci va fi trimisa o structura
        speciala pentru server cu atributul packed, cu informatiile necesare.
        * unsubscribe <TOPIC> -> similar cu subscribe, doar ca respectivul
        client se va dezabona de la un topic anume. La fel, se va parsa
        comanda si se va trimite doar daca este valida.

        !!! Detalii despre cum se retin clientii abonati la anumite topicuri
        puteti sa urmariti in prima sectiune a README-ului.

    -> de asemenea, primeste de la server mesaje UDP (ce, asa cum am amintit
    sectiunea trecuta, au fost "copiate" intr-o structura de tip TCP definita
    de mine si care se afla in utils.h). Dupa ce se primesc informatiile, se
    printeaza un mesaj cu formatul din cerinta. Daca, insa, nu s-a primit 
    nimic, atunci inseamna ca server-ul a inchis conexiunea si, implicit,
    si subscriber-ul o va face.  


------------------------------------------------------------------------------- 


    3. Tratarea erorilor

    -> am folosit macro-ul din laboratoarele de PC -> DIE

    -> DIE a fost folosit in momentul in care o operatie foarte importanta nu 
    a avut succes sau daca apar erori de input (ex: id-ul clientului are mai
    mult de 10 caractere sau nu am primit un numar corespunzatori in linia de
    comanda). 

    -> de asemenea, atunci cand parsez mesajele venite de la stdin pentru
    subscriber, in momentul in care comanda nu este una valida, se afiseaza
    la stderr ce ar fi trebuit sa faca utilizatorul ca sa ajunga la una
    corecta.


-------------------------------------------------------------------------------

    IMPORTANT!
    
    Pentru fiecare entitate (server, client TCP, UDP), am creat cate o
    structura diferita cu atributul packed care contine toate informatiile de
    care are nevoie entitatea respectiva. Astfel, pentru a copia campurile din
    buffer-ul in care primesc informatiile intr-o structura, voi face direct
    cast la acea structura. 

    De asemenea, este important de mentionat ca, pentru a gestiona cum trebuie
    fluxul de date al protocolului TCP, a fost nevoie sa primesc si sa trimit
    EXACT cat aveam nevoie. Astfel, atunci cand am vrut, de exemplu, sa ma abonez
    la un canal, subscriber-ul a trimis o strcutura pt server de 59 de bytes, iar
    server-ul, la randul lui, va primi exact 59 de bytes.

    In plus, am dezactivat algoritmul lui Neagle atat pentru socketii TCP
    acceptati de socketul TCP pasiv (inainte de a primi id-ul subscriberului
    respectiv), cat si pentru socket-ul serverului in subscriber.cpp inainte
    de a trimite id-ul catre server.  
 