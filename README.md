# projekt TFTP Klient + Server
autor: Ondřej Bahounek
login: xbahou00
vytvořeno 17.října 2023

# Eng
The project assignment was to create two programs: a client and a server capable of sharing files using the TFTP protocol implemented over UDP. Both the client and the server log all incoming communication to stderr in the format specified in the assignment.

# Popis
 Zadáním projektu bylo vytvořit 2 programy. Klienta a server, kteří si jsou schopní sdílet soubory pomocí TFTP protokolu implementovaného na UDP.  Klient i server vypisují veškerou příchozí komunikaci na stderr ve formátu specifikovaném podle zadání.
 # Rozšíření
 Server implementuje rozšíření blksize, tsite a timeout.
 # Spuštění programu

 překlad pomocí `make`
 
 Klinet se spouší pomocí příkazu:

`./tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath`

-   **hostname** je jméno nebo IPv4 adresa serveru
    
-   **port** je číslo portu kde server bude očekávat komunikaci
    
-  **filepath** je cesta k souboru, který chci ze serveru stáhnout,
	  pokud není zadáno, klient bude nahrávat soubor na server, soubor který bude očekávat na stdin
-   **dest_path** je jméno pod kteým se má soubor uložit

Spuštění serveru:

`sudo ./tftp-server [-p port] root_dirpath`

-   **číslo** je portu na kterém bude server očekávat komunikaci
    
-   **root_dirpath** je adresář do kterého a ze kterého server nahrává soubory
Spuštění pro stažení souboru ze serveru může vypadat tedy třeba takto:
`sudo ./tfpt-server source_folder`
`./tftp-client -h localhost -p 69 -f soubor_ke_stažení -t novy_soubor`
 ### Odevzdané soubory
 - tftp-client.c
 - tftp-client.h
 - tftp-server.h
 - tftp-server.c
 - tftp-utils.c
 - tftp-utils.h
 - Makefile
 - README
 - manual.pdf