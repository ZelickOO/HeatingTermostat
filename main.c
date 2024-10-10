/*
 * termostat.c
 *
 * Created: 26.12.2017 17:02:57
 * Author : zelickoo
 */ 

#define F_CPU 1000000
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "lcd.h"
#include "lcd.c"
#include <avr/eeprom.h>
#include "1wire.c"
#include "1wire.h"
#include <avr/wdt.h>

char pomocne[5];
uint8_t klice[5][9];
volatile uint8_t pocet;
volatile uint8_t hodnota;
volatile int8_t polozka = 0; //promenna pro vyber polozky menu
volatile uint8_t stat = 0; //promenna pro blokovani vicenasobneho zaregostrovani stisknuti tlacitka
volatile float teplota_kotel; //promena pro aktualni teplotu kotle
volatile float teplota_bojler; //promenna pro aktualni teplotu bojleru
volatile float teplota_zpatecka;
volatile float teplota_system;
volatile int zapnuti;
volatile int vypnuti;
volatile int zpatecka;
float cidla;
uint8_t obraz = 0;
uint16_t minuta = 0;
//deklarace funkcí

void zmen_polozku(); //funkce pro vyber nastavovaneho parametru
void zobraz(); //funkce pro zobrazeni aktualnich teplot
void nastaveni(); //funkce nastaveni
void uloz(); //funkce pro ulozeni do eeprom
void rid(); //funkce pro rizeni spinani
int nacti(); //funkce pro nacteni z eeprom
void set();
void nacticidla();
//void initcidla();

/*void initcidla(uint8_t pole[], int adresa) {
	uint8_t prvek = 0;
	for(uint8_t i = 0; i != 8; i++) {
		eeprom_write_byte((uint8_t*)adresa , pole[prvek]);
		adresa++;
		prvek++;
	}
	return;
} */

void nacticidla() {
	uint8_t cidlo = 0;
	uint8_t prvek = 0;
	int adresa = 8;
	while(1) {
	while(1) {
		klice[cidlo][prvek] = eeprom_read_byte((uint8_t*)adresa);
		prvek++;
		adresa++;
		if(prvek == 8) { prvek = 0; break; }
	}
	cidlo++;
	if(cidlo == 4) break;
	}
	return;
} 

//funkce pro rizeni spinani
	void rid() {
	if(teplota_zpatecka > zpatecka && teplota_bojler < zapnuti) PORTD |= 0b01000000; //zapnuti cerpadla
	if(teplota_zpatecka < zpatecka || teplota_bojler > vypnuti) PORTD &= 0b10111111; //vypnuti cerpadla
	return;
	}


void zmen_polozku() {
	if((PIND&0b00000001) == 0 && stat == 0) { //pokud je stisknute tlacitko "nahoru" a stisk neprobehl v predchozim cyklu
//	stat = 1; //blokuj opetovne registrovani stisknuti tlacitka
	polozka++; //nastav vyssi polozku
	lcd_clrscr();
	minuta = 0;
	if(polozka > 2) polozka = 2; //pokud uz je ale nastavena posledni polozka menu,nic nemen
	}
	if((PIND&0b00000010) == 0 && stat == 0) { //pokud je stisknute tlacitko "dolu" a stisk neprobehl v predchozim cyklu
	//	stat = 1; //blokuj opetovne registrovani stisknuti tlacitka
		lcd_clrscr();
		minuta = 0;
		polozka--; //nastav nizsi polozku
		if(polozka < 0) polozka = 0; //pokud uz je ale nastavena prvni polozka menu,nic nemen
	}
	if((PIND&0b00001111) == 15) stat = 0; //pokud neni stisknute ani jedno tlacitko,povol registraci stisknuti
	return;
}

	//funkce zobrazeni namerenych hodnot
	
void zobraz() {
	pocet = 0;
	while(1) {
	teplota_kotel = (float)DS18B20_read_temp(klice[1]) / 10;
	pocet++;
	_delay_ms(100);
	wdt_reset();
	if(teplota_kotel != 0 || pocet == 10) break;
	}
	pocet = 0;
	while(1) {
	teplota_bojler = (float)DS18B20_read_temp(klice[2]) / 10;
	pocet++;
	_delay_ms(100);
	wdt_reset();
	if(teplota_bojler != 0 || pocet == 10) break;
	}
	pocet = 0;
	while(1) {
		teplota_zpatecka = (float)DS18B20_read_temp(klice[3]) / 10;
		pocet++;
		_delay_ms(100);
		wdt_reset();
		if(teplota_zpatecka != 0 || pocet == 10) break;
		}
	pocet = 0;
	while(1) {
	teplota_system = (float)DS18B20_read_temp(klice[0]) / 10;
	pocet++;
	_delay_ms(100);
	wdt_reset();
	if(teplota_system != 0 || pocet == 10) break;
}
	lcd_clrscr(); //vymaz displej
	lcd_gotoxy(0 , 0); //nastav prvni sloupec,prvni radek
	if(obraz == 0) {
	lcd_puts("Bojler  "); //na uvedenou pozici zapis retezec
	sprintf(pomocne , "%3.1f" , teplota_bojler);
	if(teplota_bojler < 100.0) lcd_putc(' ');
	}
	if(obraz == 1) {
		lcd_puts("Kotel   "); //na uvedenou pozici zapis retezec
		sprintf(pomocne , "%3.1f" , teplota_kotel);
		if(teplota_kotel < 100.0) lcd_putc(' ');
	}
	lcd_puts(pomocne);
	lcd_putc(' ');
	lcd_putc(0b11011111);
	lcd_putc('C');
	lcd_gotoxy(0 , 1); //nastav prvni sloupec,druhy radek
	if(obraz == 0) {
	lcd_puts("Topeni  "); //na uvedenou pozici zapis retezec
	sprintf(pomocne , "%3.1f" , teplota_system);
	if(teplota_system < 100.0) lcd_putc(' ');
	}
	if(obraz == 1) {
		lcd_puts("Zpatecka"); //na uvedenou pozici zapis retezec
		sprintf(pomocne , "%3.1f" , teplota_zpatecka);
		if(teplota_zpatecka < 100.0) lcd_putc(' ');
	}
	lcd_puts(pomocne);
	lcd_putc(' ');
	lcd_putc(0b11011111);
	lcd_putc('C');
	_delay_ms(500); //cekej 0,5s
	return;
}

//funkce menu nastaveni

void nastaveni() {
	lcd_clrscr();
	while((PIND&0b00000100) != 0) { //opakuj dokud neni stisknute tlacitko "zpet"
		minuta++;
		lcd_gotoxy(0,0);
		lcd_puts("Nastaveni");
		lcd_gotoxy(0,1);
		switch (polozka) {
			case 0: lcd_puts("Zapnuti"); break;
			case 1: lcd_puts("Vypnuti"); break;
			case 2: lcd_puts("Zpatecka"); break;
		}
		if((PIND&0b00001000) == 0 && stat == 0) set();
		if((PIND&0b00001111) == 15) stat = 0;
		_delay_ms(300);
		if(minuta == 200) {
			minuta = 0;
			return;
		}
		zmen_polozku();
		wdt_reset();
		GIFR |= 0b10000000; //kvuli zakazani vicenasobneho preruseni
			}
			return;
			}


void set() {
	_delay_ms(300);
	switch(polozka) {
		case 0: hodnota = zapnuti; break;
		case 1: hodnota = vypnuti; break;
		case 2: hodnota = zpatecka; break;
		}
	while((PIND&0b00000100) != 0) {
	lcd_clrscr();
	lcd_gotoxy(0,0);
	switch(polozka) {
		case 0: lcd_puts("Zapnuti"); break;
		case 1: lcd_puts("Vypnuti"); break;
		case 2: lcd_puts("Zpatecka"); break;
	}
	sprintf(pomocne , "%d" , hodnota);
	lcd_gotoxy(0,1);
	lcd_puts(pomocne);
	if((PIND&0b00000001) == 0) hodnota++;
	if((PIND&0b00000010) == 0) hodnota--;
	if((PIND&0b00001000) == 0) {
		switch(polozka) {
			case 0: { uloz(0 , hodnota); zapnuti = hodnota; break; }
			case 1: { uloz(2 , hodnota); vypnuti = hodnota; break; }
			case 2: { uloz(4 , hodnota); zpatecka = hodnota; break; }
		}
		}
		_delay_ms(200);
		wdt_reset();
		}
		_delay_ms(300);
		lcd_clrscr();
		wdt_reset();
		return;
}


//funkce pro ulozeni do eeprom
void uloz(int adresa ,int hodnota) {
	eeprom_write_byte((uint8_t*)adresa , hodnota);
	adresa += 1;
	eeprom_write_byte((uint8_t*)adresa , (hodnota >> 8));
	lcd_clrscr();
	lcd_gotoxy(0,0);
	lcd_puts("Ulozeno");
	_delay_ms(1500);
	wdt_reset();
	return;
}

//funkce pro nacteni z eeprom
int nacti(int adresa) {
	int hodnota;
	hodnota = eeprom_read_byte((uint8_t*)adresa);
	adresa += 1;
	hodnota |= (eeprom_read_byte((uint8_t*)adresa) << 8);
	return hodnota;
}


ISR(INT1_vect) {
	//stat = 1;
	nastaveni();
}


int main(void)
{   
	DDRC = 0b11111111;
	DDRD = 0b11110000;  //nastaveni podtu D jako vstupne vystupni
	PORTD = 0b00001111; //aktivace pull-up rezistoru
	GICR |= 0b10000000; //povoleni preruseni na INT1
	MCUCR |= 0b00001010; //nastaveni INT1 na sestupnou hranu
	wdt_reset();
	wdt_enable(WDTO_2S);
	sei(); //globalni povoleni preruseeni
	lcd_init(LCD_DISP_ON); //zapnuti displeje
	lcd_gotoxy(0,0);
	lcd_puts("** Dobry den ***");
	lcd_gotoxy(0,1);
	lcd_puts("** Zelenkovi ***");
	wdt_reset();
	_delay_ms(1500);
	wdt_reset();
	nacticidla();
/*	cidla = OWFirst(klice1 , 1 , 0);
	cidla = OWNext(klice2 , 1 , 0);
	cidla = OWNext(klice3 , 1 , 0);
	cidla = OWNext(klice4 , 1 , 0);
	initcidla(klice1 , 8);
	initcidla(klice2 , 16);
	initcidla(klice3 , 24);
	initcidla(klice4 , 32);
*/
	
	    /* Replace with your application code */

	while (1) {
	if((PIND&0b00000001) == 0) obraz = 1;
	if((PIND&0b00000010) == 0) obraz = 0;
	zapnuti = nacti(0);
	vypnuti = nacti(2);
	zpatecka = nacti(4);
	zobraz();
	wdt_reset();	//zvolani funkce pro zobrazeni namerenych hodnot
	rid();
	wdt_reset();
    }
	return 0;
}

