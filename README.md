#  Posttest 3 Praktikum IoT - Sistem Monitoring dan Kontrol Bendungan Pintar (MQTT & Kodular)

Repositori ini dibuat untuk memenuhi tugas pengumpulan Posttest 3 Praktikum Internet of Things (IoT), Program Studi Informatika, Universitas Mulawarman (2026).

## Anggota Kelompok

| Nama                      | NIM        |
| :--                       | :--        |
| Christian Farrel A. P.    | 2309106032 |
| Zulfikar Heriansyah       | 2309106033 |
| Muhammad Rafly Pratama O. | 2309106043 |

---

## Judul Studi Kasus
**Miniatur Bendungan Pintar Berbasis MQTT & Kodular**

---

## Deskripsi Projek
Projek ini merupakan purwarupa sistem Internet of Things (IoT) yang mengintegrasikan pemantauan ketinggian air dan pengendalian pintu air pada miniatur bendungan dengan kemampuan kontrol dan monitoring jarak jauh menggunakan protokol MQTT dan aplikasi Kodular

Sistem ini bekerja dengan alur logika sebagai berikut:
1. **Otomatisasi Ketinggian Air:** Sensor Water Level membaca nilai ketinggian air secara *real-time*. 
   - Jika nilai sensor berada pada **<= 800 (Level Aman)**, maka pintu air tertutup (Servo 0°) dan buzzer dalam kondisi mati.
   - Jika nilai sensor berada pada **801 - 1500 (Level Waspada)**, maka pintu air terbuka setengah (Servo 90°) dan buzzer dalam kondisi mati.
   - Jika nilai sensor berada pada **> 1500 (Level Bahaya)**, maka pintu air terbuka penuh (Servo 180°) dan buzzer akan menyala berkedip.
2. **Monitoring Melalui Kodular:** Aplikasi Kodular digunakan untuk menampilkan data secara *real-time*. 
   - Menampilkan nilai level air.
   - Menampilkan status kondisi (Aman / Waspada / Bahaya).
   - Menampilkan status buzzer (Aktif / Mati).
   - Menampilkan nilai sudut servo.
3. **Mode Operasi (Otomatis & Manual):**
   - **Mode Otomatis:** Aktuator (servo dan buzzer) bekerja sesuai dengan kondisi ketinggian air.
   - **Mode Manual:** Aktuator dapat dikontrol secara langsung melalui aplikasi Kodular tanpa mengikuti kondisi sensor.
4. Seluruh sistem terhubung ke internet melalui jaringan Wi-Fi dengan protokol MQTT untuk sinkronisasi data dan kontrol perangkat secara *real-time*.

---

## Pembagian Tugas

Agar pengerjaan projek berjalan efektif, berikut adalah pembagian tugas masing-masing anggota kelompok:

| Nama                      | Deskripsi Tugas                                                                        |
| :---                      | :---                                                                                   |
| Christian Farrel A. P     | Merancang skematik dan membangun perangkat IoT, pengujian fungsional dan dokumentasi   |
| Zulfikar Heriansyah       | Merekam serta mengedit video, pengujian fungsional, membantu membangun logic perangkat |
| Muhammad Rafly Pratama O. | Menyiapkan MQTT dan aplikasi Kodular, memrogram perangkat IoT, pengujian fungsional    |

---

## Komponen yang Digunakan

Berikut adalah perangkat keras (*hardware*) yang digunakan dalam merangkai sistem ini:
* 1x Microcontroller Board (ESP32 / NodeMCU ESP8266)
* 1x Water Level Sensor
* 1x Motor Servo
* 1x Buzzer
* 1x Breadboard / Project Board
* Kabel Jumper Male-to-Male, Female-to-Female & Male-to-Female

**Platform IoT:** MQTT & Kodular

---

## Board Schematic

![Board Schematic](https://github.com/RaflyOlanda/posttest2-praktikum-iot-unmul-2026/blob/main/skematik%20pt2.jpeg)

---

## Video Demo

Berikut adalah video demonstrasi yang menampilkan bentuk fisik rangkaian serta pengujian *real-time* menggunakan aplikasi Kodular.

*(Tambahkan link video di sini)*
