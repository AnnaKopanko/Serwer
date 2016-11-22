#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <dlfcn.h>

#define BUFLEN 64  				//maksymalna długość bufora
#define PORT 666    			//port, przez który wysyłamy dane

struct params {
	float a;
	float b;
	float (*fun)(float, float);
}params; //struktura z parametrami przekazywanymi przy wywolywaniu funkcji

void die(char *s)
{
	printf(s);
    exit(1);
}

int main(int argc, char *argv[])
{
	int i;
	uint8_t space = 0;
	char buf_temp[BUFLEN];

	//zmienne - biblioteka dynamiczna
	void* lib_handle;
	char* error_msg;

	//zmienne - gniazdo
	struct sockaddr_in si_server, si_client;
    int s, slen = sizeof(si_client), recv_len;

    //bufor na dane przychodzace
    char buf[BUFLEN];

    float (*operation)(float, float); //wskaznik do funkcji

    //otwarcie biblioteki dynamicznej
    lib_handle = dlopen("/home/sylwia/workspace/Serwer_C/libdlib.so", RTLD_LAZY);

    if (!lib_handle)
    {
    	fprintf(stderr, "Blad wykonywania funkcji dlopen(): %s\n", dlerror());
    	exit(1);
    }

    //tworzenie gniazda UDP
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    // wypełnianie struktury si_server zerami
    memset((char *) &si_server, 0, sizeof(si_server));

    si_server.sin_family = AF_INET;		//uzycie protokolu IP
    si_server.sin_port = htons(PORT);	//odwracanie bitow
    si_server.sin_addr.s_addr = htonl(INADDR_ANY);

    //nadanie gniazdu etykiety adresowej (zeby nie blokowac portu)
    if( bind(s , (struct sockaddr*)&si_server, sizeof(si_server) ) == -1)
    {
        die("bind");
    }

    //ciągłe nasłuchiwanie na dane od klienta
    while(1)
    {
    	printf("Oczekiwanie na dane...");
        fflush(stdout);

        //próba odbioru danych, oczekiwanie na zapytanie klienta
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_client, &slen)) == -1)
        {
            die("recvfrom()");
        }

        //wywietl szczegoly klienta/peer i otrzymanych danych
		printf("\nOtrzymany pakiet od %s: %d\n", inet_ntoa(si_client.sin_addr), ntohs(si_client.sin_port));
		printf("Otrzymane dane: %s\n" , buf);

		for(i=0; i < BUFLEN; i++)
		{
			//pierwszy parametr programu
			if(buf[i] == ' ')
			{
				space = i + 1;
				break;
			}

			buf_temp[i] = buf[i];
		}

		params.a = (float)atof(buf_temp); //konwersja do float

		for(i=0; i < BUFLEN;i++) buf_temp[i]='\0';

		//drugi parametr programu
		for(i = 0; i < BUFLEN; i++)
		{
			if(buf[space + i] == ' ')
			{
				space = space + i + 1;
				break;
			}

			buf_temp[i] = buf[space + i];
		}

		params.b = (float)atof(buf_temp); //konwersja do float

		for(i = 0; i < BUFLEN; i++) buf_temp[i]='\0';

		//trzeci parametr programu
		for(i = 0; i < BUFLEN; i++)
		{
			if(buf[space + i] == ' ')
			{
				break;
			}

			buf_temp[i] = buf[space + i];
		}

		////dlsym - funkcja zwraca adres
		operation = dlsym(lib_handle, buf_temp); //do wskaznika przypisujemy funkcje na którą wskazuje nazwe operacji

		error_msg = dlerror();
		if (error_msg)
		{
		    sprintf(buf, "Zly parametr operacji! Sprobuj ponownie.\n");

		    //odpowiedz do klienta z wynikiem
			if (sendto(s, buf, sizeof(buf), 0, (struct sockaddr*) &si_client, slen) == -1)
			{
				die("sendto()");
			}

		    exit(1);
		}

		memset((char *) &buf, 0, sizeof(buf));

		sprintf(buf, "Wynik: %.3f.\n", operation(params.a, params.b));
		printf("Wyslany pakiet: %s", buf);

		//odpowiedz do klienta z wynikiem
        if (sendto(s, buf, sizeof(buf), 0, (struct sockaddr*) &si_client, slen) == -1)
        {
            die("sendto()");
        }

        memset((char *) &buf, 0, sizeof(buf));
    }

    close(s);

	return 0;
}
