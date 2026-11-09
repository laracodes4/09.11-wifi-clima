#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ezTime.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

Timezone brasil; // criação do fuso horário
unsigned long marcoZero = 0;

// API: sistema back-end (não exibe nada em front), todas tem documentação de como usar (/documentation)

// CÓDIGO WIFI -> faz conexão e reconexão, monitora se o sinal for perdido (avisa caso ocorra)

//=======================================WIFI=====================================================

void conexaoWifi();
void reconexaoWifi(); // avisar pro meu codigo que essas funções wifi existem

const char *SSID = "SENAI EDUCACIONAL"; // nome da rede e senha que vou utilizar
const char *SENHA = "info@134";

const unsigned long tempoEsperaConexao = 10000;  // variável pra controle de tempo de conexao e reconexao
const unsigned long tempoEsperaReconexao = 5000; // a cada 5 seg verifica se wifi ta conectado

//======================================= API ====================================================

const String enderecoApi = "https://wttr.in/Sao+Caetano+do+Sul?lang=pt&format="; // o "&format=" serve pra especificar algo que quero dentro da api

// float temperatura, sensacaoTermica;
// int umidade, pressao;
// String condicao, nascerSol;

String consultaApi(String codigo); // função string que vai receber um texto, codigo, pra inserir depois de format=

//=======================================SETUP================================================

void setup()
{ // void é uma função vazia, ela faz o que tem dentro e ponto
  Serial.begin(115200);

  conexaoWifi(); // mandando executar a função conexaoWifi

  // LCD -----------------------------------------------------------------

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 1);
  lcd.print("T:     ST:     U:   ");

  lcd.setCursor(0, 2);
  lcd.print("Sol:         P:");

  // EZTIME ----------------------------------------------------------------

  // OBS: essa parte vai no setup, mas ativei na conexao wifi (dá no mesmo)

  // // Aguarda a sincronização inicial via NTP
  // waitForSync();

  // // Define a localização (aplica fuso UTC e horário de verão automaticamente)
  // brasil.setLocation("America/Sao_Paulo");
}

//==========================================LOOP================================================
void loop()
{
  unsigned long marcoAtual = millis();

  if (marcoAtual - marcoZero >= 5000)
  {
    reconexaoWifi();

    events(); // processa os eventos internos do eztime

    // JSON --------------------------------------------------------------------

    // JsonDocument doc; // cria o objeto json

    StaticJsonDocument<512> doc;

    doc["Temp"] = consultaApi("%t").toFloat(); // adicionando dados ao objeto
    doc["Umi"] = consultaApi("%h").toInt();
    doc["Sensacao"] = consultaApi("%f").toInt();
    doc["P"] = consultaApi("%P").toInt();
    doc["Cond"] = consultaApi("%C");
    doc["Sol"] = consultaApi("%S");
    doc["data"] = brasil.dateTime("d/m/Y");
    doc["hora"] = brasil.dateTime("H:i:s");

    serializeJsonPretty(doc, Serial); // enviando um objeto presente em doc pra serial

    float temperatura = doc["Temp"];
    int umidade = doc["Umi"];
    int sensacaoTermica = doc["Sensacao"];
    int pressao = doc["P"];
    String condicao = doc["Cond"];
    String nascerSol = doc["Sol"]; // guardando os valores do json em variáveis pra poder trabalhar com eles
    String data = doc["data"];
    String hora = doc["hora"];

    //COMPARAÇÕES ---------------------------------------------------------

    static float temperaturaAnterior = -999;
    static int umidadeAnterior = -999;
    static int sensacaoAnterior = -999;
    static int pressaoAnterior = -999;
    static String condicaoAnterior = "";
    static String nascerSolAnterior = "";
    static String dataAnterior = "";
    static String horaAnterior = "";

    if (data != dataAnterior || hora != horaAnterior)
    {
      dataAnterior = data;
      horaAnterior = hora;
    }

    if (temperatura != temperaturaAnterior)
    {
      temperaturaAnterior = temperatura;
    }

    if (sensacaoTermica != sensacaoAnterior)
    {
      sensacaoAnterior = sensacaoTermica;
    }

    if (umidade != umidadeAnterior)
    {
      umidadeAnterior = umidade;
    }

    if (nascerSol != nascerSolAnterior)
    {
      nascerSolAnterior = nascerSol;
    }

    if (pressao != pressaoAnterior)
    {
      pressaoAnterior = pressao;
    }

    if (condicao != condicaoAnterior)
    {
      condicaoAnterior = condicao;
    }

    // LCD ----------------------------------------------------------------

    lcd.setCursor(0, 0);
    lcd.print(data + " " + hora);

    lcd.setCursor(2, 1);
    lcd.print(String(temperatura, 0));

    lcd.setCursor(10, 1);
    lcd.print(String(sensacaoTermica));

    lcd.setCursor(17, 1);
    lcd.print(String(umidade));

    lcd.setCursor(4, 2);
    lcd.print(nascerSol);

    lcd.setCursor(15, 2);
    lcd.print(String(pressao));

    lcd.setCursor(0, 3);
    lcd.print(condicao.substring(0, 12));

    marcoZero = marcoAtual;
  }

  // EXEMPLO ODIRLEI -------------------------------------------------------------------------

  //   temperatura = consultaApi("%t").toFloat();     //%t = texto dentro da consultaApi... ele vai para depois do format=
  //   sensacaoTermica = consultaApi("%f").toFloat(); // esse toFloat serve pra converter a variavel de texto em numero
  //   umidade = consultaApi("%h").toInt();
  //   pressao = consultaApi("%P").toInt();
  //   condicao = consultaApi("%C");
  //   nascerSol = consultaApi("%S");

  //   Serial.println("Temp: " + String(temperatura));
  //   Serial.println("Sensacao termica: " + String(sensacaoTermica));
  //   Serial.println("Umidade: " + String(umidade));
  //   Serial.println("Pressao: " + String(pressao));
  //   Serial.println("Condicao: " + condicao);
  //   Serial.println("Nascer do sol: " + nascerSol);
  //   Serial.println("--------------------------");

  //   delay(10000);
}

//=======================================FUNCÕES=============================================================

// FUNÇÃO DE CONEXÃO ---------------------------------------------------

void conexaoWifi()
{
  Serial.printf("Conectando ao Wifi: %s", SSID);

  WiFi.begin(SSID, SENHA);

  unsigned long marcoZeroWifi = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - marcoZeroWifi < tempoEsperaConexao) // enquanto não estiver conectado e estiver esperando os 10 seg, vai executar oq está dentro
  {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Wifi conectado com sucesso!");
    Serial.print("Endereço de IP: ");
    Serial.println(WiFi.localIP());

    // SINCRONIZAÇÃO DO EZTIME (É feita logo após conectar no Wi-Fi)
    Serial.println("Sincronizando relógio via NTP...");
    waitForSync();                           // Aguarda obter o horário da internet
    brasil.setLocation("America/Sao_Paulo"); // Aplica o fuso de São Paulo / Brasil
    Serial.println("Relógio sincronizado!");
  }
  else
  {
    Serial.println("Falha ao conectar");
  }
}

// FUNÇÃO RECONEXÃO WIFI -------------------------------------------------------

void reconexaoWifi()
{
  unsigned long marcoAtualWifi = millis();
  unsigned long ultimaConexao = 0;

  if (marcoAtualWifi - ultimaConexao > tempoEsperaConexao)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Conexao Wifi perdida");
      conexaoWifi(); // mandando executar de novo a conexao
    }
    ultimaConexao = marcoAtualWifi;
  }
}

// FUNÇÃO STRING CODIGO ----------------------------------------------------------

String consultaApi(String codigo)
{
  HTTPClient http;

  http.begin(enderecoApi + codigo);
  int codigoHTTP = http.GET(); // armazena codigo http
  String resposta = "";

  if (codigoHTTP == 200) // 200 = deu certo
  {
    resposta = http.getString(); // pega a consulta e salva como string
  }
  else
  {
    Serial.println("Deu erro. Codigo: " + String(codigoHTTP));
  }

  http.end();
  return resposta; // se não fizer isso, a resposta só vai existir dentro dessa funcao
}