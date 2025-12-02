#include <Arduino.h>

void telaSetup();
void telaInicial();
void usuarioInvalido();
void telaProdutos(String nomeUsuario, float saldoUsuario);
void telaSegure();
void telaSaldoFinal(float preco, float saldoUser);
void telaSaldoNegativo(float saldoUser);
void telaSemEstoque();
int telaPesagem(float peso, float preco);
void telaCadastraRFID();
void telaCadastroRealizado();