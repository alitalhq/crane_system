/* kod yuklenmeden once girilmesi gereken degiskenler */
const int yuk = 200;                    //yukun agirligi (gram)
const float calibration_factor = 492.5; //loadscell kalibrasyonu manuel olarak hesaplanıp girilmeli
const float tamburCevresi = 0.3;        // tamburun cevresi (metre)
unsigned long pulseWidth_fonk; //CH7
unsigned long pulseWidth_man;  //CH6
unsigned long ch7_2;

#include "HX711.h"  //loadcell kütüphanesi

const int encoderPinA = 2;     //interrupt 0
const int encoderPinB = 3;     //interrupt 1

const int loadcell_SCK = 6;    //loadcell sck
const int loadcell_DOUT = 7;   //loadcell dout

const int R_EN = 8;    //motor surucu R_EN
const int L_EN = 9;    //motor surucu L_EN
const int RPWM = 10;   //motor surucu RPWM
const int LPWM = 11;   //motor surucu LPWM

volatile unsigned long ch6_start, ch6_width;
volatile unsigned long ch7_start, ch7_width;



HX711 scale;

int manuelMod = 0; // manuel modun durumu (1 = yukari, -1 = asagi)

volatile bool stopFlag = false;  // Durum kontrol bayrağı (fonksiyonlardan cikis icin)

volatile long int encoderPosition = 0;
volatile long int lastEncoded = 0;

void setup() {
  Serial.begin(9600);

  pinMode(encoderPinA, INPUT);
  pinMode(encoderPinB, INPUT);
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  pinMode(20, INPUT);   // pwm okuma
  pinMode(21, INPUT);   // pwm okuma

  digitalWrite(R_EN, HIGH);  //motor surucuyu aktif et
  digitalWrite(L_EN, HIGH);

  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);

  scale.begin(loadcell_DOUT, loadcell_SCK);
  scale.set_scale(calibration_factor);  // Kalibrasyon katsayısı sabit
  scale.tare();                         // Şu anki ağırlığı sıfır kabul et

  // Gerekirse iç pull-up'ları aç (yalnızca pull-up direnci bağlamadıysan):
  // digitalWrite(encoderPinA, HIGH);
  // digitalWrite(encoderPinB, HIGH);

  // Hem A hem B pinlerine interrupt bağlanıyor
  attachInterrupt(digitalPinToInterrupt(encoderPinA), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(encoderPinB), updateEncoder, CHANGE);

  attachInterrupt(digitalPinToInterrupt(20), isrCH6, CHANGE);   // interrupt pwm0
  attachInterrupt(digitalPinToInterrupt(21), isrCH7, CHANGE);   // interrupt pwm1
}

void loop() {
  stopFlag = false; //fonksiyonlardan cikis yapildiginda "true" olur. bu yuzden sifirliyoruz

    if (manuelMod == 1 && encoderPosition <= 6000) { // eger manuel modda ise encoder 6000 degerine gelirse olasi kazalari onlemek icin otomatiik durdurur.
    motorDur();
    manuelMod = 0;
    Serial.println("Motor yukari limit aşıldı, durduruldu.");
  }

  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data == "STOP") { //serial monitorden STOP okursa durdurur. (hem manuel hem otomatik fonksiyonlarda ise yarar)
      stopFlag = true;
      manuelMod = 0;
      motorDur();
      Serial.println("Islem durduruldu.");
    }
    else if (data.startsWith("Yuk_Al")) { //yuk al fonksiyonunu algilar
      int parameter = data.substring(7).toInt(); // ornek girdi Yuk_Al 30
      Yuk_Al(parameter);
    }
    else if (data.startsWith("Yuk_Birak")) { //yuk birak fonksiyonunu algilar
      int parameter = data.substring(10).toInt();// ornek girdi Yuk_Birak 30
      Yuk_Birak(parameter);
    }
    else if (data.startsWith("Manuel")) { //manuel fonksiyonunun algilar
    char dirChar = data.charAt(7); // ornek girdi Manuel Y  veya  Manuel A
    int dir = (dirChar == 'Y') ? 1 : ((dirChar == 'A') ? -1 : 0);
    manuelKontrol(dir);
    }
  }


    


  unsigned long ch6 = readStableValue(&ch6_width); //interruptı durdurmak yerine farklı şekilde kontrol sağlanıyor
  unsigned long ch7 = readStableValue(&ch7_width);//loop içinde çok fazla interrupt durdurulursa encoder değerine zarar verebilir
  ch7_2 = ch7;

  Serial.print("CH6: "); Serial.print(ch6);
  Serial.print("  CH7: "); Serial.println(ch7);
  delay(100);


    if(ch7 > 1050 && ch7 < 1220){ // FONKSİYONEL STOP
      stopFlag = true;
      motorDur();
      manuelMod = 0;
      Serial.println("Kumanda ile stop komutu verildi");
    }
    else if (ch7 > 1220 && ch7 < 1400){ //FONKSİYONEL YUK AL 2m
      Serial.println("Kumanda ile yuk al2 komutu verildi");
      Yuk_Al(2);
    }
    else if(ch7 > 1400 && ch7 < 1700){ // FONKSİYONEL YUK BİRAK 2m
      Serial.println("Kumanda ile yuk birak2 komutu verildi");
      Yuk_Birak(2);
    }

    if (ch6 > 1000 && ch6 < 1300){  //MANUEL YUKARİ
      Serial.println("Kumanda ile manuel asagi komutu verildi");
      manuelKontrol(1);
    }
    else if(ch6 > 1500 && ch6 < 1800){ //MANUEL ASAGİ
      Serial.println("Kumanda ile manuel yukari komutu verildi");
      manuelKontrol(-1);
    }


}//LOOP SONU

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void motorYukari(){
  analogWrite(RPWM, 0);  // motor yukari
  analogWrite(LPWM, 255);
}

void motorAsagi(){
  analogWrite(RPWM, 255);  // motor asagi
  analogWrite(LPWM, 0);
}

void motorDur(){
  analogWrite(RPWM, 0);  // motor dur
  analogWrite(LPWM, 0);
}

// Manuel motor kontrol fonksiyonu
void manuelKontrol(int direction) {
  if (direction == 1) { //yukari
    analogWrite(RPWM, 0);
    analogWrite(LPWM, 255);
  }
  else if (direction == -1) { //asagi
    analogWrite(RPWM, 255);
    analogWrite(LPWM, 0);
  }
  else {
    motorDur();
  }
}

bool yukVarMi(){ //yuk kontrolu
  return (scale.get_units(10) >= yuk*0.8);
}

void hedefeIn(long int hedef){     //kod tamamen hazir olduğu zaman buradaki print'leri silebilirsiniz.
  Serial.print("anlik encoder: ");  //vinc Cengaver'e bağlandığı zaman serial monitoru okuyamayacağız.
  Serial.println(encoderPosition);
  Serial.print("hedef encoder: ");
  Serial.println(hedef);
  Serial.println("hedefe iniliyor...");
  Serial.println(ch7_2);

  while (encoderPosition < hedef){
    if(kontrol()){ //kontrol fonksiyonu ile birlikte STOP komutu girilmiş mi diye kontrol ediliyor
      return;
    }

    motorAsagi();
  }
  motorDur();
  Serial.println("hedefe ulasildi.");
  Serial.print("konum: ");
  Serial.println(encoderPosition);
  bekleVeKontrol(100);
}

void hedefeCik(long int hedef){   //kod tamamen hazir olduğu zaman buradaki print'leri silebilirsiniz.
  Serial.print("anlik encoder: "); //vinc Cengaver'e bağlandığı zaman serial monitoru okuyamıyacağız.
  Serial.println(encoderPosition);
  Serial.print("hedef encoder: ");
  Serial.println(hedef);
  Serial.println("hedefe cikiliyor...");

  while (encoderPosition > hedef) {
    if(kontrol()){ //kontrol fonksiyonu ile birlikte STOP komutu girilmiş mi diye kontrol ediliyor
      return;
    }

    motorYukari();
  }
  motorDur();
  Serial.println("hedefe ulasildi.");
  Serial.print("konum: ");
  Serial.println(encoderPosition);
  bekleVeKontrol(100);
}

void bekleVeKontrol(unsigned long sure) { // delay komutu yerine millis kullanildi bu sayede istenilen vakit kadar beklerken ayni zamanda STOP kontrolu yapar
  unsigned long baslangicZamani = millis();
  while (millis() - baslangicZamani < sure) {
    if (kontrol()){ //kontrol fonksiyonu ile birlikte STOP komutu girilmiş mi diye kontrol ediliyor
      return;
    }
  }
}


bool kontrol(){  //kontrol fonksiyonu ile birlikte STOP komutu girilmis mi diye kontrol ediliyor
  if (Serial.available()) {
    String data = Serial.readStringUntil('\n');
    data.trim();
    if (data == "STOP") {
      stopFlag = true; //stop girilirse flag true olur
      motorDur(); // motor durur
      manuelMod = 0; // manuel mod flagi sifirlanir
      Serial.println("Islem durduruldu.");
    }
  }
unsigned long ch7_current = readStableValue(&ch7_width); //ch7 anlik degeri aliniyor

if(ch7_current > 1050 && ch7_current < 1220){ // FONKSİYONEL STOP
    stopFlag = true;
    motorDur();
    manuelMod = 0;
    Serial.println("Kumanda ile stop komutu verildi");
    }


  return stopFlag;
}

unsigned long readStableValue(volatile unsigned long* var) {
  unsigned long val1, val2;
  do {
    val1 = *var;
    val2 = *var;
  } while (val1 != val2);
  return val1;
}

// Enkoder verisini işleyen fonksiyon
void updateEncoder() {
  int MSB = digitalRead(encoderPinA);  // Kanal A
  int LSB = digitalRead(encoderPinB);  // Kanal B

  int encoded = (MSB << 1) | LSB;  // 2 bitlik sayı oluştur
  int sum = (lastEncoded << 2) | encoded;

  // Gray code çözümü: yön tayini
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderPosition++;
  else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderPosition--;

  lastEncoded = encoded;
}

void isrCH6() {
  if (digitalRead(20)) {
    ch6_start = micros();
  } else {
    ch6_width = micros() - ch6_start;
  }
}

void isrCH7() {
  if (digitalRead(21)) {
    ch7_start = micros();
  } else {
    ch7_width = micros() - ch7_start;
  }
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Yuk_Al(int hedefYukseklik) {
  long int turSayisi = hedefYukseklik / tamburCevresi;    // salinacak tur sayısı hesaplanir.Tamburun çevresi yaklaşık 30cm
  long int encoderAnlik = encoderPosition;                // encoderin anlik degeri alinir
  long int encoderHedef = encoderAnlik + 4000 * turSayisi;

  hedefeIn(encoderHedef); //hedefe kadar iner
  bekleVeKontrol(15000); //delay yerine artik bekleVeKontrol fonksiyonu kullaniliyor


  bool flag = true;
  while (flag) {

    if(kontrol()){ // STOP kontrolunu her dongunun basina koydum
      return;
    }

    hedefeCik((long)(encoderHedef*0.7)); //yuk kontrolu icin mesafenin yuzde 30una kadar geriler

    if (yukVarMi()) {  // yuk bindi mi?
      flag = false;        // yuk varsa kontrol dongusunden cikar
      break;
    }
    else {
      hedefeIn(encoderHedef); //yuk yoksa hedefe tekrar iner ve döngü. içerisinde devam eder
      bekleVeKontrol(15000);
    }
  }

  if(kontrol()){
    return;
  }

  hedefeCik((long)(encoderHedef*0.5)); //yukari cikarken yüzde 50lik mesafede yuk kontrolu yapilir
  bekleVeKontrol(500);

  if(!yukVarMi()){ //yuk yoksa fonksiyona tekrar girilir
    Yuk_Al(hedefYukseklik);
    return; //suanki fonksiyondan cikar
  }

  hedefeCik((long)(encoderHedef*0.25)); //yukari cikarken yüzde 25lik mesafede ikinci yuk kontrolu yapilir
  bekleVeKontrol(500);

  if(!yukVarMi()){ //yuk yoksa fonksiyona tekrar girilir
    Yuk_Al(hedefYukseklik);
    return; //suanki fonksiyondan cikar
  }

  hedefeCik(4000); //yukari cekilir.
  bekleVeKontrol(1000);
  Serial.println("YUK_AL_TAMAM");
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Yuk_Birak(int hedefYukseklik) {

  long int turSayisi = hedefYukseklik / tamburCevresi;   // salinacak tur sayısı hesaplanir
  long int encoderAnlik = encoderPosition;               // encoderin anlik degeri alinir
  long int encoderHedef = encoderAnlik + 4000 * turSayisi;

  while (encoderPosition <= encoderHedef + 4000 && yukVarMi()) {  // hedef yükseklige inene kadar veya yuk dusene kadar motor asagi salinir
    if(kontrol()){
      return;
    }

    Serial.print("Motor Asagi: ");
    Serial.println(encoderPosition);
    motorAsagi();
  }
  motorDur();
  bekleVeKontrol(1000);

  if(encoderPosition >= encoderHedef){  //motorun durma sebebi yuk dusmesi mi yoksa hedef bolgeye gelmemiz mi?
    hedefeCik((long)(encoderHedef*0.8));        //eger encoderHedef+4000 bolgesine geldigi icin durdu ise bu dongu calisir ve yuk birakilana kadar tekrarlanir.
    bekleVeKontrol(1000);

    while(yukVarMi()){

      if(kontrol()){
        return;
      }

      hedefeIn((long)(encoderHedef + 4000));
      bekleVeKontrol(1000);

      hedefeCik((long)(encoderHedef*0.8));
      bekleVeKontrol(2000);
    }
  }

  if(kontrol()){
    return;
  }

  hedefeCik(1000);                      //yuk birakilir ise veya yuk hedefe ulasmadan dusmus ise vinc geri sarilir.
  bekleVeKontrol(1000);
  Serial.println("YUK_BIRAK_TAMAM");
}

 
