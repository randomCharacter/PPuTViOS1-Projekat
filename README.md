# Projekat iz predmeta PPuTViOS 1

## Osnovni zadatak
1. Prilikom pokretanja aplikacije, učitava se konfiguraciona datoteka čija lokacija je
određena ulaznim parametrom komandne linije. Struktura konfiguracione datoteke
je proizvoljna, kao i podaci koji će se nalaziti u njoj, dok je minimalan skup
parametara:
a) Freq
b) Bandwidth
c) Module
d) Inicijalni kanal
i. Audio PID
ii. Video PID
iii. Audio Type
iv. Video Type
e) Program number
2. Na osnovu pakonfiguracione datoteke, pri pokretanju aplikacije podesiti ulazni
stepen digitalnog TV prijemnika i otpočeti prijem signala;
3. Pri pokretanju aplikacije pokrenuti video i audio PID iz konfiguracione datoteke –
slika postaje vidljiva na ekranu;
VITeorijske osnove
4. Na osnovu parsiranja PAT i PMT tabela, aplikacija dobavlja I memoriše spisak
servisa;
a) Napomena: PMT tabelu parsirati na zahtev (na promenu kanala)
5. Implementirati modul za korišćenje daljinskog upravljača koji se izvršava u
posebnoj niti i koristeći callback mehanizam, javlja koje je dugme stisnito modulu
koji ga koristi;
6. Omogućiti ručni unos broja kanala;
7. Implementirati grafički korisnički interfejs, koji se izvršava u render niti.
Napomena: Obratiti pažnju da dve grafičke informacije mogu da se pojave
istovremeno na ekranu.
a) Pri svakoj promeni kanala, ispisuje se dijalog sa podacima o broju trenutnog
kanala, informacijama o postojanju teleteksta, audio i video PID-u;
b) Info dijalog automatski nestaje nakon 3 sekunde;
c) Nakon automatskog ispisivanja i nestajanja info dijaloga, potrebno je omogućiti
ponovno prikazivanje pritiskom na ***INFO*** dugme. Ukoliko se ***INFO*** dugme
pritisne tokom prikazivanja info dijaloga, automatsko nestajenje treba da se
prolongira za novih 3 sekunde.
d) Pri promeni jačine zvuka, aplikacije prikazuje grafički interfejs sa skalom
trenutne jačine zvuka. Prilikom pritiska na dugme ***MUTE*** potrebno je
maksimalno utišati zvuk.
Programski kod treba da bude napisan u skladu sa dobrom praksom opisanom u
dokumentu „Strukturiranje koda – konvencije i preporuke”. Dokumentacija se piše u skladu
sa zahtevima navedenim u dokumentu “Dokumentacija – pravila i preporuke”.

## Dodatni zadatak
U info dijalog dodati informacije o nazivu trenutnog i narednog događaja. Informacije o
trenutnom i narednom događaju su dostupne u EIT tabeli.
