# 🚡 Vinç Kontrol Yazılımı

Bu proje, yükün **güvenli, kontrollü ve otomatik şekilde alınması ve bırakılması** için geliştirilmiş bir vinç kontrol yazılımıdır. Yazılım **C++ dili** ile geliştirilmiş olup, hem manuel hem de otomatik modlarda çalışabilmektedir. Sistem, güvenlik öncelikli olacak şekilde tasarlanmış ve çeşitli sensörlerden gelen verilerle desteklenmiştir.

---

## 📌 Özellikler

- **İki ana çalışma modu:**
  - **Manuel mod:**  
    - Kumanda veya seri port üzerinden gelen komutlarla motor doğrudan kontrol edilir.  
    - Encoder tabanlı sınır kontrolü ile motor güvenli limitlere ulaştığında otomatik durdurulur.
  - **Otomatik mod:**  
    - `Yük_Al` ve `Yük_Bırak` fonksiyonları motoru encoder verileriyle hedef yüksekliğe yönlendirir.  
    - Loadcell ile yükün varlığı sürekli kontrol edilir.  
    - Yük düşerse algoritma süreci güvenli şekilde tekrar başlatır.  

- **Güvenlik önlemleri:**
  - Seri port veya kumandadan gelen **STOP** komutu, tüm işlemleri anında sonlandırır.  
  - Bekleme süreçleri **millis() tabanlı** `bekleVeKontrol` fonksiyonu ile yapılır, böylece bekleme sırasında da STOP ve güvenlik kontrolleri devam eder.  

- **Uçuş bilgisayarı entegrasyonu:**  
  - Yazılım, uçuş bilgisayarından gelen **irtifa verilerini** okuyarak motor hareketlerini daha hassas ve senkronize bir şekilde gerçekleştirir.

---

## 🛠️ Kullanılan Donanımlar

- **Mikrodenetleyici:** Arduino Uno  
- **Motor sürücüsü:** BTS7960
- **DC Motor + Makara sistemi**
- **Encoder:** Motor pozisyonu ve hareket ölçümü için  
- **Loadcell + HX711:** Yük algılama ve ağırlık ölçümü için  
- **Kumanda alıcısı (PWM kanalları):** CH6 ve CH7 üzerinden kontrol girişleri  
- **Uçuş bilgisayarı:** İrtifa verisi sağlayıcı (örneğin Pixhawk, Cube veya benzeri)
- **Seri haberleşme bağlantısı (USB üzerinden uçuş bilgisayarı ile)**
- **RC Kumanda (CH6 & CH7 sinyalleri için)**

---

## 💻 Kullanım Kılavuzu

### 🔹 Seri Port Komutları
Yazılım, seri porttan gelen komutları okuyarak çalışır. Kullanabileceğiniz komutlar:

| Komut             | Açıklama                                                                 |
|-------------------|---------------------------------------------------------------------------|
| `Yuk_Al X`        | Yük alma işlemini başlatır. `X` değeri hedef encoder konumunu belirtir.   |
| `Yuk_Birak X`     | Yük bırakma işlemini başlatır. `X` değeri hedef encoder konumunu belirtir.|
| `Manuel Y`        | Motoru **manuel modda yukarı** hareket ettirir.                           |
| `Manuel A`        | Motoru **manuel modda aşağı** hareket ettirir.                            |
| `STOP`            | Herhangi bir anda tüm işlemleri derhal durdurur.                          |

✅ Örnekler:  


```
Yuk_Al 50000
Yuk_Birak 20000
Manuel Y
STOP
```
### 🔹 Kumanda ile Kontrol
- **CH6** → Yön kontrolü (Yukarı / Aşağı seçimi)  
- **CH7** → STOP kontrolü  
  - CH7 belirlenen aralıkta sinyal gönderdiğinde, yazılım STOP komutunu çalıştırır.  

İlk açılışta CH6 ve CH7 yaklaşık **1500 µs** merkez değerindedir. Kumanda kolu yukarı/aşağı hareket ettirildiğinde bu değer **1000 µs – 2000 µs** arasında değişir.  

---

## ⚙️ PID Algoritması

Motor hareketleri sırasında **PID algoritması** kullanılarak:
- Hedefe uzak mesafelerde maksimum hızda hareket edilir.
- Hedefe yaklaştıkça hız kademeli olarak düşürülür.
- Böylece yük ani durmaz, sistem daha kararlı ve güvenli çalışır.  

PID parametreleri (`Kp`, `Ki`, `Kd`) kullanıcı tarafından proje sırasında kalibre edilmelidir.  

---

## ⚙️ Çalışma Mantığı

1. **Başlangıç:** Sistem açıldığında encoder sıfırlanır, loadcell kalibre edilir.  
2. **Manuel Mod:**  
   - Kumanda CH6 / CH7 kanalları veya seri port üzerinden motor yukarı/aşağı hareket ettirilir.  
   - Encoder limitine ulaşıldığında motor otomatik durdurulur.  
3. **Otomatik Mod:**  
   - `Yük_Al`: Motor belirlenen hedef konuma kadar yükü kaldırır.  
   - `Yük_Bırak`: Motor belirlenen hedefe kadar yükü indirir.  
   - Süreç boyunca loadcell ile yük varlığı doğrulanır.  
4. **STOP Komutu:**  
   - Seri port veya kumandadan STOP geldiğinde tüm fonksiyonlar sonlandırılır.  

---


## 🛡️ Güvenlik Mekanizmaları

- Encoder ile pozisyon sınır kontrolü  
- Loadcell ile yük düşme tespiti  
- STOP komutu ile tüm işlemleri sonlandırma  
- Bekleme anında dahi güvenlik kontrolü (`millis` tabanlı bekleme)  

---

## 📖 Lisans

Bu proje açık kaynaklıdır ve [MIT Lisansı](./LICENSE) altında dağıtılmaktadır.  

---

