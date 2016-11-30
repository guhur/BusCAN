#include <mcp_can_dfs.h>
#include <mcp_can.h> // Librairie du bus CAN
#include <SPI.h> // Librairie de la liaison série
#include <leOS2.h> // Librairie pour la gestion multi-tâches


// CAN.sendMsgBuf(INT32U id, INT8U ext, INT8U len, INT8U *buf);
//id represents where the data come from.
//ext represents the status of the frame. '0' means standard frame. '1' means extended frame.
//len represents the length of this frame.
//buf is the content of this message.


leOS2 myOS; // Création d'une nouvelle instance de multi-tâches

long unsigned int rxId; // Identifiant de la trame CAN reçue
const long unsigned int ID_ACTU = 0x36;
const long unsigned int ID_TEMPERATURE = 0xF6;
const long unsigned int ID_VITESSE = 0xB6;
const long unsigned int ID_INFO = 0x128;
const long unsigned int ID_COMODO = 0x94;
const long unsigned int ID_ESSENCE = 0x161;
const long unsigned int ID_VOLANT = 0x402;
const long unsigned int ID_GRAVE = 0x168;
//const int SEND_ACTU = 1;
//const int SEND_TEMP = 2;
//const int SEND_INFO = 3;
//const int SEND_VITESSE = 4;
long unsigned int send_trame = -1; // choix de la trame a envoyer
unsigned char len = 0; // Longueur de la trame
unsigned char rxBuf[8]; // Trame reçue 


// Initialisation des trames associées au tableau de bord
unsigned char trame_actu[8]={0x80, 0x00, 0x00, 0x7F, 0x01, 0x00, 0x00, 0x00}; // Trame d'actualisation du tableau de bord dont l'envoie conditionne l'émission vers le tableau de bord
unsigned char trame_temperature[8]={0xCA, 0x90, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00}; // Trame de la jauge de température
unsigned char trame_info[8]={0x80, 0x00, 0x01, 0x00, 0x00, 0xB4, 0x70, 0x28}; // Trame des voyants d'informations (phares, clignotants, abs etc...)
unsigned char trame_vitesse[8]={0x40, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x80}; // Trame des cadrants de vitesse et de compte-tours
unsigned char trame_essence[8]={0x40, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x80}; // Trame de la jauge de carburant



MCP_CAN CAN0(10); // Place Chip Select au pin 10


void setup()
{
  Serial.begin(115200); // Initialisation de la sortie série
  CAN0.begin(CAN_125KBPS);  // Initialise le bus CAN à un baudrate de 125 kbps 
  pinMode(2, INPUT);                            // Configure la pin 2 pour des entrée de 
  Serial.println("Gestion_CAN");
  
  // Déclaration des tâches
  myOS.begin(); // Activation de l'instance multi tâche
  myOS.addTask(reception, myOS.convertMs(16)); // La librairie leOS2 marche sur base temporelle multiple de 16ms
//  myOS.addTask(actu, myOS.convertMs(100));    // La fonction convertMs permet de convertir automatiquement pour l'utilisateur.
//  myOS.addTask(temperature, myOS.convertMs(1000));
//  myOS.addTask(vitesse, myOS.convertMs(50));  
  myOS.addTask(envoi, myOS.convertMs(100));
  myOS.addTask(modification_trame, myOS.convertMs(16)); // Cette tâche modifie les trames envoyées
  
}


const char* getIdName(long unsigned int id)
{
  switch (id) {
    case ID_ACTU: 
      return "actu";
      break;
    case ID_TEMPERATURE: 
      return "temp";
      break;
    case ID_VITESSE: 
      return "vitesse";
      break;
    case ID_INFO: 
      return "info";
      break;
    case ID_COMODO: 
      return "comodo";
      break;
      case ID_ESSENCE: 
      return "essence";
      break;
    case ID_VOLANT: 
      return "volant";
      break;
    case ID_GRAVE: 
      return "grave";
      break;

    default:
      return "unknown";
      break;
  } 
}


// Tâche de réception
void reception ()
{
 if(!digitalRead(2))                         // Si pin 2 est à l'état bas, On lit le buffer reçu
    {
      CAN0.readMsgBuf(&len, rxBuf);              // Lecture des données: len = longueur, buf = octets de donnée.
      rxId = CAN0.getCanId();                    // Récupèration de l'Id de la trame reçue
      Serial.print("ID: ");   
      Serial.print( getIdName(rxId) );
      Serial.print(" ");   
      Serial.print(rxId, HEX);
      Serial.print("  Data: ");
      for(int i = 0; i<len; i++)                // Impression sur le moniteur série des données reçues.
      {
        if(rxBuf[i] < 0x10)                     // Si l'octet de données est inférieur à 0x10, on ajoute un zéro.
        {
          Serial.print("0");
        }
        Serial.print(rxBuf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }

  
}

// Détail des tâches à exécuter
// Ces tâches vont envoyées les trames associées au tableau de bord lorsqu'on a modifié une trame.
void envoi(){
  switch (send_trame) {
    case ID_ACTU:
      CAN0.sendMsgBuf(ID_ACTU,0,8,trame_actu); // sendMsgBuf est la fonction qui envoie les trames par le bus CAN.
      send_trame = -1;
      Serial.print(" actu ");   
      break;
    case ID_TEMPERATURE:
      CAN0.sendMsgBuf(ID_TEMPERATURE,0,8,trame_temperature);
      send_trame = -1;
      break;
    case ID_INFO:
      CAN0.sendMsgBuf(ID_INFO,0,8,trame_info);
      send_trame = -1;
      break;
    case ID_VITESSE:
      CAN0.sendMsgBuf(ID_VITESSE,0,8,trame_vitesse);
      send_trame = -1;
    default:
      CAN0.sendMsgBuf(0,0,8,trame_info);
      break;
  } 
} 

  
  
//  Tâche de modification des trames envoyées
// Cette tâche filtre les différentes trames émises par la maquette commodo et va modifier les trames trames envoyées permettant de commander le tableau de bord
void modification_trame(){
  // Clignotant droit 
  if((rxId==ID_COMODO) && ((rxBuf[2]&0x80)==0x80)){ // On teste le bit associé à l'action voulue sur la maquette commodo
    trame_info[4]=(trame_info[4]&(0xFF^0x02)); // On modifie la trame pour réaliser l'action voulue sur le tableau de bord, ici allumage du clignotant droit
    trame_info[4]=(trame_info[4] |0x04); // Extinction du clignotant gauche si déjà allumé.
    send_trame = ID_INFO;
  }
  // Clignotant gauche
  if((rxId==ID_COMODO) && ((rxBuf[2]&0x40)==0x40)){
    trame_info[4]=(trame_info[4] |0x02);
    trame_info[4]=(trame_info[4]&(0xFF^0x04));
    send_trame = ID_INFO;
  }
 
  //Brouillard avant
  if((rxId==ID_COMODO) && ((rxBuf[0]&0x04)==0x04)){ 
    trame_info[4]=(trame_info[4] | 0x10); // Allumage
    send_trame = ID_INFO;
  }
  else if((rxId==ID_COMODO) && ((rxBuf[0]&0x04)!=0x04)){ 
    trame_info[4]=(trame_info[4]&(0xFF^0x10)); //Extinction
    send_trame = ID_INFO;
  }
  
  //Brouillard arriere
  if((rxId==ID_COMODO) && ((rxBuf[0]&0x02)==0x02)){ 
    trame_info[4]=(trame_info[4] | 0x08);
    send_trame = ID_INFO;
  }
  else if((rxId==ID_COMODO) && ((rxBuf[0]&0x02)!=0x02)){ 
    trame_info[4]=(trame_info[4]&(0xFF^0x08)); //Extinction
    send_trame = ID_INFO;
  }
  
  // Vitesse (position 1 du levier essui-glaces)
  if((rxId==ID_COMODO) && ((rxBuf[1]&0x20)==0x20)){ // Vitesse placée à environ 85 km/h
    trame_vitesse[2]= 0x20;
    send_trame = ID_VITESSE;
  }
  else if((rxId==ID_COMODO) && ((rxBuf[1]&0x20)!=0x20)){ // Remise à 0.
    trame_vitesse[2]= 0x00;
    send_trame = ID_VITESSE;
  }
  
  // On réitère la même structure pour les autres modifications de trames envoyées au tableau de bord.
}

 
void loop() {
  }
