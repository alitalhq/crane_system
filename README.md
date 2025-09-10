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
- **Motor sürücüsü:** BTS7960 (yüksek akım motor kontrolü için)  
- **Encoder:** Motor pozisyonu ve hareket ölçümü için  
- **Loadcell + HX711:** Yük algılama ve ağırlık ölçümü için  
- **Kumanda alıcısı (PWM kanalları):** CH6 ve CH7 üzerinden kontrol girişleri  
- **LED / Buzzer (opsiyonel):** Durum göstergesi ve uyarılar için  
- **Uçuş bilgisayarı:** İrtifa verisi sağlayıcı (örneğin Pixhawk, Cube veya benzeri)

---

## 📂 Proje Yapısı

```

📦 VincYazilimi
┣ 📜 src/                # Kaynak kodlar
┣ 📜 Vinc.ino            # Ana yazılım dosyası (Arduino Sketch)
┣ 📜 README.md           # Proje açıklamaları
┗ 📜 LICENSE             # Lisans dosyası

```

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

## 🔧 Kurulum ve Kullanım

1. Arduino IDE üzerinden `Vinc.ino` dosyasını açın.  
2. Gerekli kütüphaneleri yükleyin:  
   - HX711 (Loadcell için)  
   - Encoder kütüphanesi  
3. Donanım bağlantılarını şemaya uygun şekilde yapın.  
4. Kodu Arduino’ya yükleyin.  
5. Seri port veya kumanda ile komut göndererek sistemi test edin.  

---

## 📡 Kumanda ve Seri Port Komutları

- **Kumanda CH6 / CH7:**  
  - Yukarı / Aşağı hareket  
  - Yük al / bırak işlemleri  

- **Seri Port Komutları:**  
  - `F1` → Yük Al  
  - `F2` → Yük Bırak  
  - `STOP` → Acil durdurma  

---

## 🛡️ Güvenlik Mekanizmaları

- Encoder ile pozisyon sınır kontrolü  
- Loadcell ile yük düşme tespiti  
- STOP komutu ile tüm işlemleri sonlandırma  
- Bekleme anında dahi güvenlik kontrolü (`millis` tabanlı bekleme)  

---

## 📖 Lisans

Bu proje açık kaynaklıdır ve [MIT Lisansı](./LICENSE) altında dağıtılmaktadır.  
