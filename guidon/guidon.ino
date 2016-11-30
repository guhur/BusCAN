// Le code permet d'afficher la valeur du guidonn

#include <mcp_can.h> // Librairie du bus CAN
#include <SPI.h> // Librairie de la liaison série
#include <leOS2.h> // Librairie pour la gestion multi-tâches
#include <stdlib.h>
//#include <endian.h>

leOS2 myOS; // Création d'une nouvelle instance de multi-tâches

long unsigned int rxId; // Identifiant de la trame CAN reçue
const long unsigned int ID_GUIDON = 0x305; // Identifiant du guidon
unsigned char len = 0; // Longueur de la trame
unsigned char rxBuf[8]; // Trame reçue 
unsigned char trame_guidon[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; 
MCP_CAN CAN0(10); // Place Chip Select au pin 10


void setup()
{
  Serial.begin(115200); // Initialisation de la sortie série
  CAN0.begin(CAN_500KBPS);  // Initialise le bus CAN à un baudrate de 125 kbps 
  pinMode(2, INPUT);                            // Configure la pin 2 pour des entrée de 
  Serial.println("Gestion_CAN");
  
  // Déclaration des tâches
  myOS.begin(); // Activation de l'instance multi tâche
  myOS.addTask(reception, myOS.convertMs(16)); // La librairie leOS2 marche sur base temporelle multiple de 16ms
}



// Tâche de réception
void reception ()
{
 if(!digitalRead(2))                         // Si pin 2 est à l'état bas, On lis le buffer reçu
    {
      CAN0.readMsgBuf(&len, rxBuf);              // Lecture des données: len = longueur, buf = octets de donnée.
      rxId = CAN0.getCanId();                    // Récupèration de l'Id de la trame reçue
      if (rxId == ID_GUIDON) { // on filtre l'etat du tableau de bord
        Serial.print("Guidon:");
        for(int i = 0; i<len; i++)                // Impression sur le moniteur série des données reçues.
        {
          if(rxBuf[i] < 0x10)                     // Si l'octet de données est inférieur à 0x10, on ajoute un zéro.
          {
            Serial.print("0");
          }
          Serial.print(rxBuf[i], HEX);
          Serial.print(" ");
        }

      
        int value = rxBuf[0] << 8;
        value += rxBuf[1];
        Serial.print(" = ");    
        Serial.print(value);
        Serial.print("\n");
      }
    }

}

 
void loop() {
  }
