#include "tela.h"
#include <U8g2_for_Adafruit_GFX.h>
#include <GxEPD2_BW.h>

GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16); 
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
U8G2_FOR_ADAFRUIT_GFX fontes; 

const float PESO_MAXIMO = 200.0f;

void telaSetup(){
    tela.init(); 
    tela.setRotation(3); 
    tela.fillScreen(GxEPD_WHITE); 
    tela.display(true);

    fontes.begin(tela); 
    fontes.setForegroundColor(GxEPD_BLACK);
}

void telaInicial(){
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvB24_te );
    fontes.setFontMode(1);
    fontes.setCursor(50, 50);
    fontes.print("Bem vindo!");

    fontes.setFont( u8g2_font_helvB12_te );
    fontes.setFontMode(1);
    fontes.setCursor(60, 80);
    fontes.print("Aproxime sua tag no");
    fontes.setCursor(90, 100);
    fontes.print("local indicado");

    tela.display(true);
}

void usuarioInvalido(){
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvB24_te );
    fontes.setFontMode(1);
    fontes.setCursor(70, 50);
    fontes.print("Inválido!");

    fontes.setFont( u8g2_font_helvB12_te );
    fontes.setFontMode(1);
    fontes.setCursor(70, 90);
    fontes.print("Tente novamente");
    tela.display(true);
}

void telaProdutos(String nomeUsuario, float saldoUsuario){
    String saldoFormatado = String(saldoUsuario, 2); 
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvR10_te );
    fontes.setFontMode(1);
    fontes.setCursor(160, 20);
    fontes.print(nomeUsuario + ": R$" + saldoFormatado);

    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(10, 50);
    fontes.print("Selecione o produto:");

    fontes.setFont( u8g2_font_helvB12_te );
    fontes.setFontMode(1);
    fontes.setCursor(10, 90);
    fontes.print("Amendoim");
    fontes.setFont( u8g2_font_helvR12_te );
    fontes.setFontMode(1);
    fontes.setCursor(10, 110);
    fontes.print("R$0,50");

    fontes.setFont( u8g2_font_helvB12_te );
    fontes.setFontMode(1);
    fontes.setCursor(200, 90);
    fontes.print("M&M");
    fontes.setFont( u8g2_font_helvR12_te );
    fontes.setFontMode(1);
    fontes.setCursor(200, 110);
    fontes.print("R$0,80");
    tela.display(true);
}

void telaSegure(){
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(20, 50);
    fontes.print("Segure o botão do produto");

    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(70, 90);
    fontes.print("para despejá-lo");
    tela.display(true);
}

void telaSaldoFinal(float preco, float saldoUser){
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(90, 50);
    fontes.print("Saldo atual:");

    float saldoAtual = saldoUser - preco;

    fontes.setFont( u8g2_font_helvR14_te );
    fontes.setFontMode(1);
    fontes.setCursor(120, 90);
    fontes.print("R$" + String(saldoAtual, 2));
    tela.display(true);
}

void telaSaldoNegativo(float saldoUser){
    tela.fillScreen(GxEPD_WHITE);
    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(90, 30);
    fontes.print("Saldo negativo");

    fontes.setFont( u8g2_font_helvR14_te );
    fontes.setFontMode(1);
    fontes.setCursor(120, 70);
    String saldoFormatado = "R$"+String(saldoUser, 2);
    fontes.print(saldoFormatado);

    fontes.setFont( u8g2_font_helvR12_te );
    fontes.setFontMode(1);
    fontes.setCursor(100, 110);
    fontes.print("Faça a recarga!");
    tela.display(true);
}

void telaSemEstoque(){
    tela.fillScreen(GxEPD_WHITE);
    fontes.setFont( u8g2_font_helvB24_te );
    fontes.setFontMode(1);
    fontes.setCursor(45, 50);
    fontes.print("Sem estoque");

    fontes.setFont( u8g2_font_helvB12_te );
    fontes.setFontMode(1);
    fontes.setCursor(65, 90);
    fontes.print("Escolha outro produto");
    tela.display(true);
}

int telaPesagem(float peso, float preco){
    tela.fillScreen(GxEPD_WHITE); 
    tela.drawLine(50, 30, 50, 110, GxEPD_BLACK);
    tela.drawLine(50, 110, 150, 110, GxEPD_BLACK);
    tela.drawLine(150, 30, 150, 110, GxEPD_BLACK);

    if (peso >= PESO_MAXIMO * 0.995f) {
        printf("Peso máximo atingido: %.2f g\n", peso);
        return 1;
    }

    float nivel = constrain(peso / PESO_MAXIMO, 0.0, 1.0);

    int altura = nivel * 80; 
    int yInicio = 110 - altura;

    tela.fillRect(50, yInicio, 100, altura, GxEPD_BLACK);

    fontes.setFont(u8g2_font_helvB14_te);
    fontes.setFontMode(1);
    fontes.setCursor(200, 70);
    fontes.print("R$ " + String(preco, 2));

    tela.display(true);
    return 0;
}

void telaCadastraRFID(){
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(20, 70);
    fontes.print("Aproxime o novo");
    fontes.setCursor(70, 90);
    fontes.print("cartão");
    tela.display(true);

}

void telaCadastroRealizado(){
    tela.fillScreen(GxEPD_WHITE); 
    fontes.setFont( u8g2_font_helvB14_te );
    fontes.setFontMode(1);
    fontes.setCursor(50, 70);
    fontes.print("Cadastro realizado!");
    tela.display(true);
}