#include <8051.h>
#include <stdio.h>
#include <serial.h>

// Definição da velocidade do som em cm por segundo
#define velocidade_som 34300

// Definição dos pinos do display LCD
#define LCD_Data P1
__sbit __at (0xB5) trig;
__sbit __at (0xB2) echo;
__sbit __at (0xA0) LCDrs;
__sbit __at (0xA1) LCDrw;
__sbit __at (0xA2) LCDen;

// Função para inicializar a comunicação serial
void inicia_serial() {
    TMOD |= 0x20;    // Configura o Timer 1 em modo 2 (8-bit auto-reload)
    TH1 = 0xFD;      // Define a taxa de transmissão para 9600 bps
    TL1 = 0xFD;
    SCON = 0x50;     // Configura o modo de operação serial (modo 1)
    TR1 = 1;         // Inicia o Timer 1
}

// Função para enviar um caractere via comunicação serial
void send_char_serial(char c) {
    SBUF = c;         // Carrega o caractere no buffer serial
    while (!TI);      // Espera até que a transmissão esteja concluída
    TI = 0;           // Reseta a flag de transmissão
}

// Função para enviar uma string via comunicação serial
void send_string_serial(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        send_char_serial(str[i]);
        i++;
    }
}

// Função para gerar um atraso de 1 µs
void Delay_us() {
    TL0 = 0xF5;
    TH0 = 0xFF;
    TR0 = 1;           // Inicia o Timer 0
    while (!TF0);      // Aguarda a interrupção do Timer 0
    TR0 = 0;           // Para o Timer 0
    TF0 = 0;           // Reseta o flag de overflow do Timer 0
}

// Função para inicializar o Timer 0
void inicia_timer() {
    TMOD |= 0x01;      // Configura o Timer 0 em modo 1 (16-bit)
    TF0 = 0;
    TR0 = 0;
}

// Função para enviar um pulso de trigger de 10 µs ao sensor ultrassônico
void send_trigger_pulse() {
    trig = 1;          // Define o pino de trigger como HIGH
    Delay_us();        // Gera um atraso de 10 µs
    trig = 0;          // Define o pino de trigger como LOW
}

// Função para gerar um atraso em milissegundos
void delay(unsigned int k) {
    int i, j;
    for (i = 0; i < k; i++)
        for (j = 0; j < 112; j++);
}

// Função para enviar um comando para o display LCD
void LCD_Command(char Command) {
    LCD_Data = Command;
    LCDrs = 0;
    LCDrw = 0;
    delay(2);
    LCDen = 1;
    delay(2);
    LCDen = 0;
    delay(3);
}

// Função para enviar um caractere para o display LCD
void LCD_Char(char Data) {
    LCD_Data = Data;
    LCDrs = 1;
    LCDrw = 0;
    delay(2);
    LCDen = 1;
    delay(2);
    LCDen = 0;
    delay(3);
}

// Função para exibir uma string no display LCD
void LCD_String(unsigned char *str) {
    int i;
    for (i = 0; str[i] != 0; i++) {
        LCD_Char(str[i]);
    }
}

// Função para exibir uma string em uma posição específica no display LCD
void LCD_String_xy(unsigned char row, unsigned char pos, unsigned char *str) {
    if (row == 1)
        LCD_Command((pos & 0x0F) | 0x80);
    else if (row == 2)
        LCD_Command((pos & 0x0F) | 0xC0);
    LCD_String(str);
}

// Função para inicializar o display LCD
void inicia_LCD() {
    delay(1000);
    LCD_Command(0x30);  // Configura o display para modo de operação
    LCD_Command(0x30);
    LCD_Command(0x30);
    LCD_Command(0x38);  // Define o modo de operação: 8 bits, 2 linhas
    LCD_Command(0x0C);  // Liga o display e desliga o cursor
    LCD_Command(0x01);  // Limpa o display
    LCD_Command(0x06);  // Define o modo de entrada
    LCD_Command(0x80);  // Posiciona o cursor na primeira linha
}

// Função para medir a distância usando o sensor ultrassônico
int medir_distancia() {
    int range = 0;
    long int timerval;
    float fator_correcao = 1.02; // Ajuste para calibrar o sensor

    send_trigger_pulse();       // Envia um pulso de 10 µs
    while (!echo);              // Aguarda o pino de echo ser ativado
    TR0 = 1;                    // Inicia o Timer 0
    while (echo && !TF0);        // Aguarda o pino de echo retornar LOW
    TR0 = 0;                    // Para o Timer 0

    timerval = TH0;
    timerval = ((timerval << 8) | TL0); // Lê o valor do Timer 0

    if (timerval < velocidade_som)
        range = (int)((timerval * velocidade_som * fator_correcao) / (2 * 1000000));
    else
        range = 0;

    return range;
}

// Função principal do programa
void main() {
    int range = 0;
    int i = 0, j;
    int soma_distancias = 0;
    int num_leituras = 10;      // Número de leituras para média
    unsigned char str[8] = "0000"; // String para exibir a distância

    inicia_LCD();              // Inicializa o display LCD
    LCD_String_xy(1, 1, (unsigned char *)"Distancia:"); // Exibe "Distancia:" na primeira linha
    inicia_timer();
    inicia_serial();           // Inicializa a comunicação serial

    while (1) {
        soma_distancias = 0;
        for (j = 0; j < num_leituras; j++) {
            soma_distancias += medir_distancia(); // Mede a distância e acumula
            delay(10); // Pequena pausa entre as leituras
        }
        range = soma_distancias / num_leituras;   // Calcula a média das leituras

        // Converte o número para uma string de 4 dígitos
        for (i = 3; i >= 0; i--) {
            str[i] = 0x30 | (range % 10);
            range = range / 10;
        }

        LCD_String_xy(2, 1, str); // Exibe a distância no LCD
        send_string_serial(str);  // Envia a distância pela comunicação serial
        send_char_serial('\n');   // Envia uma nova linha para finalizar a mensagem

        delay(100);
    }
}