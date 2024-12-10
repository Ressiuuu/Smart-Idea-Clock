# Smart Clock Idea
![image](https://github.com/user-attachments/assets/e6e03125-8c35-4517-85c3-291a070f1104)

**Smart Clock Idea** adalah proyek untuk mengembangkan perangkat berbasis ESP yang dapat diakses melalui jaringan WiFi. Proyek ini menggunakan dua ESP:  
- **ESP1**: Berfungsi sebagai pusat pengolahan data dan antarmuka dashboard.  
- **ESP2**: Berfungsi untuk mengirimkan data ke ESP1.  

---

## Fitur
- **WiFi Configuration**: Menggunakan WiFiManager untuk konfigurasi jaringan.
- **Dashboard Akses**: Mengakses halaman web dashboard melalui jaringan lokal pada ESP1.
- **Data Communication**: ESP2 mengirimkan data ke ESP1 untuk diproses lebih lanjut.

---

## Tutorial Menjalankan Proyek

### Persyaratan
- 2 buah ESP Microcontroller (misalnya ESP8266/ESP32)
- Power Supply untuk masing-masing ESP
- Perangkat dengan konektivitas WiFi (laptop, smartphone, dll.)
- Web browser

### Langkah-langkah
#### Konfigurasi Awal (ESP1)
1. Sambungkan **ESP1** ke power supply.
2. Periksa daftar jaringan WiFi pada perangkat lain (seperti laptop atau smartphone) dan cari Access Point (AP) dengan nama **"ESP Smart Clock"**.
3. Hubungkan perangkat ke AP tersebut.  
   **Catatan**: AP ini dibuat oleh ESP1 menggunakan library WiFiManager.
4. Setelah terhubung:
   - Jika halaman konfigurasi terbuka secara otomatis, lanjutkan ke langkah berikutnya.  
   - Jika tidak, buka alamat berikut melalui browser:  
     `192.168.4.1`
5. Pada halaman yang muncul, pilih **"Configure WiFi"**.
6. Masukkan nama jaringan WiFi (SSID) dan kata sandi, lalu klik **"Save"**.
7. Tunggu beberapa detik hingga konfigurasi tersimpan, lalu tekan tombol **Reset** pada ESP1.
8. Setelah reset, ESP1 akan terhubung ke jaringan WiFi yang telah dikonfigurasi. Dashboard dapat diakses melalui URL berikut di browser pada perangkat yang terhubung ke jaringan yang sama:  
   `http://smartclock18.local`

#### Konfigurasi dan Pengaturan ESP2
1. Sambungkan **ESP2** ke power supply.
2. ESP2 akan otomatis mencari jaringan WiFi yang telah dikonfigurasi pada ESP1.
3. Setelah ESP2 terhubung ke WiFi, data akan dikirimkan secara berkala ke ESP1 melalui protokol komunikasi (misalnya HTTP atau MQTT).  
   **Catatan**: Pastikan kedua ESP terhubung pada jaringan WiFi yang sama.

---

## Solusi jika mendapatkan kasus seperti ini 
1. **Tidak menemukan AP "ESP Smart Clock"**  
   - Pastikan ESP1 mendapatkan daya yang cukup.  
   - Tekan tombol reset pada ESP1 untuk memulai ulang proses.

2. **Halaman konfigurasi tidak muncul otomatis**  
   - Periksa koneksi ke AP "ESP Smart Clock".  
   - Buka `192.168.4.1` secara manual melalui browser.

3. **ESP2 gagal mengirim data ke ESP1**  
   - Pastikan ESP2 terhubung ke jaringan WiFi yang sama dengan ESP1.  
   - Periksa protokol komunikasi yang digunakan untuk pengiriman data.


### Foto Rangkaian : 
![image](https://github.com/user-attachments/assets/4492a98e-fa02-4351-ae3d-d2f75b349012)
![image](https://github.com/user-attachments/assets/f26215e6-eb9a-49b4-bae7-ff9721699eae)
![image](https://github.com/user-attachments/assets/763db7c7-743b-4fd6-a66a-301e7b593bc5)

