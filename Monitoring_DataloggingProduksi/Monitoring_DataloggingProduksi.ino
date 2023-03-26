#include <SPI.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <SoftwareSerial.h>

SoftwareSerial HC12(7, 8); // HC-12 TX Pin, HC-12 RX Pin
#if defined(ARDUINO_ARCH_SAMD)  // for Zero, output on USB Serial console
#define Serial SerialUSB
#endif

File dataFile;

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
char nama2_bulan[13][12] = {"","Januari", "Februari", "Maret", "April", "Mei", "Juni", "Juli", "Agustus", "September", "Oktober", "November", "Desember"};
char* hari;
char* nama_bulan;
String data_sd, pesan, header, temp, str_waktu, str_jml, str_total, str_jam1, str_menit1, str_detik1, str_shift, str_tgl;
char nama_file[50], nama_folder[50], judul[50], waktu_pesan[10], waktu1_pesan[10];
int shift, temp_shift, temp_tgl, tgl_sd, bulan_sd, tahun_sd, unix_waktu_sekarang, urutan_hari, unix_waktu_baterai_sebelum, tahun, bulan, tgl, jam, menit, detik, jam1, menit1, detik1, waktu_jeda, state_reset, state_cek, state_sd, ind1, ind2, ind3, ind4, ind5, ind6, ind7, ind8;
static int jumlah_produksi = 0, total_produksi = 0;

void setup() {
//inisialisasi state bahwa arduino setelah direset
state_reset=1;
state_cek=1;
Serial.begin(9600);
Serial.println();

//Inisialisasi RTC:
Serial.println("Inisialisasi RTC");
if (! rtc.begin()) {
    Serial.println("Inisialisasi RTC gagal, RTC tidak ditemukan");
    }
if (! rtc.isrunning()) {
    Serial.println("RTC tidak berjalan!");
    // setting RTC sesuai waktu program ini dicompile
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("Selesai inisialisasi RTC");

//Inisialisasi RF:
Serial.println("Inisialisasi RF");
HC12.begin(9600);
HC12.println("1,");      // Mengirim heartbeat ke receiver tanda connected  
Serial.println("Selesai inisialisasi RF");

//Inisialisasi SD Card
Serial.println("Inisialisasi SD Card");
if (!SD.begin(10)) {  //pin CS SD Card module pada datalogging shield ini konek ke pin 10
    Serial.println("Inisialisasi SD Card gagal!");
    state_sd=0; //inisialisasi state bahwa sd card gagal
    //Kirim RF:
    HC12.println("0,"); //kirim pesan ke receiver jika SD Card gagal terbaca
    Serial.println("RF berhasil mengirim");
    return;
    }
else {state_sd=1; //inisialisasi state bahwa sd card berhasil
    }

//Cek file temp
if (SD.exists("temp.txt")) {      
    //buka temp file 
    Serial.println("Membuka temp file");
    dataFile = SD.open("temp.txt");
    if (dataFile) {
        // read from the file until there's nothing else in it:
        while (dataFile.available()) {
            // memindahkan isi temp file ke dalam variabel temp            
            temp = dataFile.readStringUntil('\r');
            }
        // close the file:
        dataFile.close();
        }
    else {
        // if the file didn't open, print an error:
        Serial.println("Gagal membuka temp.txt");
        }

    // parsing sebaris isi file temp menjadi data jumlah produksi, waktu baterai sebelum, waktu baterai pertama, total produksi, tgl, dan shift saat file temp dibuat
    // memilah dengan memberi batas indeks tiap data
    ind1=temp.indexOf(',');
    str_jml=temp.substring(0, ind1);
    ind2=temp.indexOf(',', ind1+1);
    str_waktu=temp.substring(ind1+1, ind2+1);
    ind3=temp.indexOf(',', ind2+1);
    str_jam1=temp.substring(ind2+1, ind3+1);
    ind4=temp.indexOf(',', ind3+1);
    str_menit1=temp.substring(ind3+1, ind4+1);
    ind5=temp.indexOf(',', ind4+1);
    str_detik1=temp.substring(ind4+1, ind5+1);
    ind6=temp.indexOf(',', ind5+1);
    str_total=temp.substring(ind5+1, ind6+1);
    ind7=temp.indexOf(',', ind6+1);
    str_tgl=temp.substring(ind6+1, ind7+1);
    ind8=temp.indexOf(',', ind7+1);
    str_shift=temp.substring(ind7+1);

    // memasukkan data temp ke variabel
    jumlah_produksi= str_jml.toInt();
    unix_waktu_baterai_sebelum= str_waktu.toInt();  
    jam1= str_jam1.toInt();
    menit1= str_menit1.toInt();
    detik1= str_detik1.toInt();
    total_produksi= str_total.toInt();
    temp_tgl= str_tgl.toInt();
    temp_shift= str_shift.toInt();
    }
  
else {
    // jika temp file belum ada, membuatnya dengan isi adalah 0 semua
    Serial.println("Temp fle belum ada, membuat temp file...");
    dataFile = SD.open("temp.txt", FILE_WRITE);
    if (dataFile) {
        Serial.println("Menulis ke temp file...");
        dataFile.print("0,0,0,0,0,0,0,0");
        dataFile.print('\r');
        // close the file:
        dataFile.close();
        Serial.println("Membuat temp file berhasil");
        }
    else {
        // if the file didn't open, print an error:
        Serial.println("Gagal membuat temp.txt");
        }
    }
Serial.println("Selesai inisialisasi SD Card");

//Inisialisasi GSM:
Serial.println("Inisialisasi GSM");
Serial3.begin(9600); // Sim800L dikoneksikan ke rx3 dan tx3
delay(1000);
Serial.println("Selesai inisialisasi GSM");

//Inisialisasi Counter:
pinMode(2, INPUT);
}


void loop() {

//Bila dalam state sd card tidak ada masalah, kirim heartbeat ke receiver
if (state_sd==1){
    //Kirim RF:
    HC12.println("1,");      // Send heartbeat  
    }    

//   Input waktu:
DateTime waktu_sekarang = rtc.now();
unix_waktu_sekarang = waktu_sekarang.unixtime(); 
urutan_hari = waktu_sekarang.dayOfTheWeek();
hari = daysOfTheWeek[urutan_hari];
nama_bulan = nama2_bulan[waktu_sekarang.month()];
tgl = waktu_sekarang.day();
bulan = waktu_sekarang.month();
tahun = waktu_sekarang.year();
jam = waktu_sekarang.hour();
menit = waktu_sekarang.minute();
detik = waktu_sekarang.second();

//Deteksi shift sekarang
if(jam <= 5){
    shift=3;
    }
else if(jam == 6){
    if(menit < 30){
        shift=3;
        } 
    else {
        shift=1;
        }
    }
else if(jam <=14){
    shift=1;
    }
else if(jam == 15){
    if(menit < 30){
        shift=1;
        } 
    else {
        shift=2;
        }
    }
else if(jam <= 22){
    shift=2;
    }
else if(jam == 23){
    if(menit < 30){
        shift=2;
        } 
    else {
        shift=3;
        }
    }

//Cek pergantian shift
if(jam==6 && menit==30 && detik==0){
    ganti_shift();
    }
if(jam==15 && menit==30 && detik==0){
    ganti_shift();
    }
if(jam==23 && menit==30 && detik==0){
    ganti_shift();
    }

// Apabila arduino habis reset, maka cek validasi temp file
if(state_reset==1 && state_cek==1){
    Serial.println("---------------------------------------------------------------");
    Serial.println();
    Serial.print(hari);
    Serial.print(", ");
    Serial.print(tgl);
    Serial.print("/");
    Serial.print(bulan);
    Serial.print("/");
    Serial.print(tahun);
    Serial.print(", ");
    Serial.print(jam);
    Serial.print(":");
    Serial.print(menit);
    Serial.print(":");
    Serial.print(detik);
    Serial.print(", Shift ");
    Serial.println(shift);

    //Cek apakah temp file sesuai shift dan tgl saat ini
    //Mengupdate tgl, bulan, tahun utk nama file sd card. jika shift 3 jam 00.00-6.29, maka tgl yg digunakan adalah tgl kemarin supaya 1 file dengan jam 23.30-23.59
    if (jam <= 5) {
        DateTime dts (tahun, bulan, tgl, jam, menit, detik);
        DateTime dt = dts - TimeSpan(1, 0, 0, 0);
        tgl_sd= dt.day();
        bulan_sd= dt.month();
        tahun_sd= dt.year();
        }
    else if (jam == 6) {
        if (menit < 30) {
        DateTime dts (tahun, bulan, tgl, jam, menit, detik);
        DateTime dt = dts - TimeSpan(1, 0, 0, 0);
        tgl_sd= dt.day();
        bulan_sd= dt.month();
        tahun_sd= dt.year();
        }
        else {
            tgl_sd= tgl;
            bulan_sd= bulan;
            tahun_sd= tahun;
            }
        }
    else {
        tgl_sd= tgl;
        bulan_sd= bulan;
        tahun_sd= tahun;
        }

    //Cek kesamaan tgl file temp dengan tgl sekarang
    if(temp_tgl!=tgl_sd){
        //tgl temp file berbeda, reset temp file dan semua data temp
        Serial.println("Temp file sudah expired, memperbaharui temp file...");
        jumlah_produksi= 0;
        total_produksi= 0;
        unix_waktu_baterai_sebelum= 0;  
        //Reset temp file in SD Card
        temp="0,0,0,0,0,0,";
        temp+=String(tgl_sd);
        temp+=",";
        temp+=String(shift);
        SD.remove("temp.txt"); //menghapus file temp dahulu supaya data isinya tidak menumpuk lebih dari 1
        // if the file is available, write to it:
        dataFile = SD.open("temp.txt", FILE_WRITE);
        if (dataFile) {
            Serial.println("Menulis ke temp file...");
            dataFile.print(temp);
            dataFile.print('\r');
            // close the file:
            dataFile.close();
            Serial.println("Menyimpan temp file berhasil");
            }
        else {
            // if the file didn't open, print an error:
            Serial.println("Gagal buka temp.txt");
            }
        }
    //jika tgl temp file sama tapi shift nya yang berbeda, maka reset temp file kecuali total produksi
    else if(temp_shift!=shift){
        Serial.println("Temp file berbeda shift, memperbaharui temp file...");
        jumlah_produksi= 0;
        unix_waktu_baterai_sebelum= 0;
        //Reset temp file in SD Card
        temp="0,0,0,0,0,";
        temp+=String(total_produksi);
        temp+=",";
        temp+=String(tgl_sd);
        temp+=",";
        temp+=String(shift);
        SD.remove("temp.txt");  //menghapus file temp dahulu supaya data isinya tidak menumpuk lebih dari 1
        // if the file is available, write to it:
        dataFile = SD.open("temp.txt", FILE_WRITE);
        if (dataFile) {
            Serial.println("Menulis ke temp file...");
            dataFile.print(temp);
            dataFile.print('\r');
            // close the file:
            dataFile.close();
            Serial.println("Menyimpan temp file berhasil");
            }
        else {
            // if the file didn't open, print an error:
            Serial.println("Gagal buka temp.txt");
            }
      }
      if (jumlah_produksi > 0) {  //jika jumlah produksi >0 maka tampilkan data:
          Serial.print("Jumlah Produksi Shift Ini= ");
          Serial.println(jumlah_produksi);
          Serial.print("Waktu baterai pertama= ");
          Serial.print(jam1);
          Serial.print(":");
          Serial.print(menit1);
          Serial.print(":");
          Serial.println(detik1);
          waktu_jeda = unix_waktu_sekarang - unix_waktu_baterai_sebelum;
          Serial.print("Beda waktu dengan baterai sebelum= ");
          Serial.print(waktu_jeda);
          Serial.print(" detik atau ");
          Serial.print(waktu_jeda/60);
          Serial.println(" menit");
          }
      if (total_produksi > 0){  //jika total produksi >0 maka tampilkan data:
          Serial.print("Total Produksi Hari Ini= ");
          Serial.println(total_produksi);
          }
    state_cek=0;  //karena sudah melewati state cek temp file, maka mengganti state cek
    }

//Ada baterai diproduksi
if(digitalRead(2)== HIGH){ 
    hitung();
    }
delay(200); //delay untuk debouncer sekaligus jeda untuk RF kirim heartbeat. Waktu respon untuk counter naik adalah 0.5 sekon jika ada baterai baru yang lewat beruntun

//Cek waktu jeda baterai
cek_waktu_jeda();
}


//#################################################################################


void ganti_shift() {
Serial.println();
Serial.println("---------------------------GANTI SHIFT----------------------------------");
Serial.println();
Serial.print(hari);
Serial.print(", ");
Serial.print(tgl);
Serial.print("/");
Serial.print(bulan);
Serial.print("/");
Serial.print(tahun);
Serial.print(", ");
Serial.print(jam);
Serial.print(":");
Serial.print(menit);
Serial.print(":");
Serial.print(detik);
Serial.println();
Serial.print("Shift ");
Serial.println(shift);
Serial.print("Jumlah Produksi Shift Sebelumnya = ");
Serial.println(jumlah_produksi);

if(shift==1 && total_produksi!=0){ //jika menjadi shift 1, maka tampilkan data total produksi hari sebelumnya dan di file shift 3 sebelum shift ini ditulis total produksi hari itu di baris terakhir
    Serial.print("Total Produksi Kemarin= ");
    Serial.println(total_produksi);
    //Simpan SD Card
    data_sd = "Total Produksi Hari Ini=";
    data_sd += "\t";
    data_sd += String(total_produksi);
    dataFile = SD.open(nama_file, FILE_WRITE);
    if (dataFile) {
        // if the file is available, write to it:
        dataFile.println("----------------------------");
        dataFile.println(data_sd);
        dataFile.close();
        Serial.println("Selesai menyimpan di SD Card");}
        // if the file isn't open, pop up an error:
    else {
        Serial.println("Gagal menyimpan");
        }
    total_produksi = 0; //reset total produksi
    }
else{ //jika tidak ganti shift menjadi shift 1 maka:
    Serial.print("Total Produksi Hari Ini= ");
    Serial.println(total_produksi);
    }
jumlah_produksi = 0;  //reset jumlah produksi
  
//Reset temp file in SD Card & tgl sekarang
if (jam <= 5) {
    DateTime dts (tahun, bulan, tgl, jam, menit, detik);
    DateTime dt = dts - TimeSpan(1, 0, 0, 0);
    tgl_sd= dt.day();
    }
else if (jam == 6) {
    if (menit < 30) {
        DateTime dts (tahun, bulan, tgl, jam, menit, detik);
        DateTime dt = dts - TimeSpan(1, 0, 0, 0);
        tgl_sd= dt.day();
        }
    else {
        tgl_sd= tgl;
        }
    }
else {
    tgl_sd= tgl;
    }
temp="0,0,0,0,0,";
temp+=String(total_produksi);
temp+=",";
temp+=String(tgl_sd);
temp+=",";
temp+=String(shift);
SD.remove("temp.txt");
// if the file is available, write to it:
dataFile = SD.open("temp.txt", FILE_WRITE);
if (dataFile) {
    Serial.println("Menulis ke temp file...");
    dataFile.print(temp);
    dataFile.print('\r');
    // close the file:
    dataFile.close();
    Serial.println("Menyimpan temp file berhasil");
    }
else {
    // if the file didn't open, print an error:
    Serial.println("Gagal buka temp.txt");
    }
delay(1000); //delay supaya di serial tidak masuk ke function ini lebih dari 1x
}


//#################################################################################


void cek_waktu_jeda(){
if (jumlah_produksi > 0) {  //cek waktu jeda jika shift ini sudah memproduksi supaya sms sesuai
    waktu_jeda = unix_waktu_sekarang - unix_waktu_baterai_sebelum;  //cari selisih baterai terakhir dengan waktu sekarang
    if (waktu_jeda == 1800) { //terhambat 30 menit dalam detik
        Serial.println();
        Serial.println("---------------------------PRODUKSI TERHAMBAT !!!----------------------------------");
        Serial.print(jam);
        Serial.print(":");
        Serial.print(menit);
        Serial.print(":");
        Serial.print(detik);
        Serial.println();
        Serial.print("Waktu Jeda= ");
        Serial.print(waktu_jeda/60);
        Serial.println(" menit");
        Serial.println("Produksi terhambat sejak 30 menit lalu");

        //Kirim SMS
        Serial.println("SMS");
        Serial3.write("AT+CMGF=1\r\n");
        delay(1000);
        Serial3.write("AT+CMGS=\"085647789789\"\r\n");
        delay(1000);
        Serial3.write("Produksi terhambat sejak 30 menit lalu");
        delay(1000);
        Serial3.write((char)26);
        delay(1000);
  
        Serial3.write("AT+CMGF=1\r\n");
        delay(1000);
        Serial3.write("AT+CMGS=\"081285044978\"\r\n");
        delay(1000);
        Serial3.write("Produksi terhambat sejak 30 menit lalu");
        delay(1000);
        Serial3.write((char)26);
        delay(1000);
        Serial.println("SMS Terkirim");

        //Simpan SD Card
        dataFile = SD.open(nama_file, FILE_WRITE);  //menulis tanda bahwa produksi terhambat di file shift ini
        if (dataFile) {
            // if the file is available, write to it:
            dataFile.println("Produksi Terhambat Sejak 30 Menit Lalu");
            dataFile.close();
            Serial.println("Selesai menyimpan di SD Card");
            }
            // if the file isn't open, pop up an error:
        else {
            Serial.println("Gagal menyimpan");
            }
    }

    if (waktu_jeda == 3600) { //terhambat 1 jam dalam detik
        Serial.println();
        Serial.println("---------------------------PRODUKSI TERHAMBAT !!!----------------------------------");
        Serial.print(jam);
        Serial.print(":");
        Serial.print(menit);
        Serial.print(":");
        Serial.print(detik);
        Serial.println();
        Serial.print("Waktu Jeda= ");
        Serial.print(waktu_jeda/60);
        Serial.println(" menit");
        Serial.println("Produksi terhambat sejak 60 menit lalu");

        //Kirim SMS
        Serial.println("SMS");
        Serial3.write("AT+CMGF=1\r\n");
        delay(1000);
        Serial3.write("AT+CMGS=\"085647789789\"\r\n");
        delay(1000);
        Serial3.write("Produksi terhambat sejak 60 menit lalu");
        delay(1000);
        Serial3.write((char)26);
        delay(1000);
  
        Serial3.write("AT+CMGF=1\r\n");
        delay(1000);
        Serial3.write("AT+CMGS=\"081285044978\"\r\n");
        delay(1000);
        Serial3.write("Produksi terhambat sejak 60 menit lalu");
        delay(1000);
        Serial3.write((char)26);
        delay(1000);
        Serial.println("SMS Terkirim");
    
        //Simpan SD Card
        dataFile = SD.open(nama_file, FILE_WRITE);  //menulis tanda bahwa produksi terhambat di file shift ini
        if (dataFile) {
            // if the file is available, write to it:
            dataFile.println("Produksi Terhambat Sejak 1 Jam Lalu");
            dataFile.close();
            Serial.println("Selesai menyimpan di SD Card");
            }
            // if the file isn't open, pop up an error:
        else {
            Serial.println("Gagal menyimpan");
            }
    }
    
    if (waktu_jeda == 14400) {  //terhambat 4 jam dalam detik
        Serial.println();
        Serial.println("---------------------------PRODUKSI TERHAMBAT !!!----------------------------------");
        Serial.print(jam);
        Serial.print(":");
        Serial.print(menit);
        Serial.print(":");
        Serial.print(detik);
        Serial.println();
        Serial.print("Waktu Jeda= ");
        Serial.print(waktu_jeda/60);
        Serial.println(" menit");
        Serial.println("Produksi terhambat sejak 4 jam lalu");

        //Kirim SMS
        Serial.println("SMS");  
        Serial3.write("AT+CMGF=1\r\n");
        delay(1000);
        Serial3.write("AT+CMGS=\"081285044978\"\r\n");
        delay(1000);
        Serial3.write("Produksi terhambat sejak 4 jam lalu");
        delay(1000);
        Serial3.write((char)26);
        delay(1000);
        Serial.println("SMS Terkirim");
    
        //Simpan SD Card
        dataFile = SD.open(nama_file, FILE_WRITE);  //menulis tanda bahwa produksi terhambat di file shift ini
        if (dataFile) {
            // if the file is available, write to it:
            dataFile.println("Produksi Terhambat Sejak 4 Jam Lalu");
            dataFile.close();
            Serial.println("Selesai menyimpan di SD Card");}
            // if the file isn't open, pop up an error:
        else {
            Serial.println("Gagal menyimpan");
            }
        }
    }
}


//#################################################################################


void hitung(){
//counter tambah 1 jumlah dan total produksi
jumlah_produksi++;
total_produksi++;
Serial.println();
Serial.println("Ada baterai baru");
DateTime waktu_baterai (tahun,bulan,tgl,jam,menit,detik); //input waktu sekarang ke dalam variabel waktu baterai
Serial.print("Shift ");
Serial.print(shift);
Serial.print("; ");
Serial.print(hari);
Serial.print(", ");
Serial.print(jam);
Serial.print(":");
Serial.print(menit);
Serial.print(":");
Serial.print(detik);
Serial.print(", Jumlah Produksi Shift Ini= ");
Serial.print(jumlah_produksi);
Serial.print(", Total Produksi Hari Ini= ");
Serial.println(total_produksi);
  
if (jumlah_produksi == 1) { //jika baterai pertama, input ke waktu baterai pertama
    jam1=jam;
    menit1=menit;
    detik1=detik;
    }
else{ //jika bukan baterai pertama, tampilkan beda waktu baterai
    Serial.print("Beda waktu dengan baterai sebelum= ");
    Serial.print(waktu_jeda);
    Serial.print(" detik atau ");
    Serial.print(waktu_jeda/60);
    Serial.println(" menit");
    }
Serial.print("Waktu baterai pertama= ");
Serial.print(jam1);
Serial.print(":");
Serial.print(menit1);
Serial.print(":");
Serial.println(detik1);

//Simpan ke SD Card
sdcard();

//membuat pesan dalam bentuk string utk dikirim RF (shift,tgl-bulan-tahun,jam:menit:detik,jumlah,jam1:menit1:detik1,total,waktu jeda)
pesan= String(shift);
pesan+= ",";
pesan+= String(tgl);
pesan+= "-";
pesan+= String(bulan);
pesan+= "-";
pesan+= String(tahun);
pesan+= ",";
sprintf(waktu_pesan, "%02d:%02d:%02d", jam, menit, detik);
pesan+= String(waktu_pesan);
pesan+= ",";
pesan+= String(jumlah_produksi);
pesan+= ",";
sprintf(waktu1_pesan, "%02d:%02d:%02d", jam1, menit1, detik1);
pesan+= String(waktu1_pesan);
pesan+= ",";
pesan+= String(total_produksi);
pesan+= ",";
pesan+= String(waktu_jeda);
pesan+= ",";
    
//Kirim RF:
HC12.println(pesan);      // Send that data to HC-12  
Serial.println("RF berhasil mengirim");

unix_waktu_baterai_sebelum = waktu_baterai.unixtime();  //mencatat unixtime waktu baterai saat ini
state_reset=0;  //membuat state reset menjadi 0 yang artinya arduino sudah lama reset terakhirnya
}


//#################################################################################


void sdcard(){
//update temp file in SD Card
temp=String(jumlah_produksi);
temp+= ",";
temp+=String(unix_waktu_baterai_sebelum);
temp+= ",";
temp+=String(jam1);
temp+= ",";
temp+=String(menit1);
temp+= ",";
temp+=String(detik1);
temp+= ",";
temp+=String(total_produksi);
temp+=",";
temp+=String(tgl_sd);
temp+=",";
temp+=String(shift);
SD.remove("temp.txt");  //menghapus file temp dahulu supaya data isinya tidak menumpuk lebih dari 1
dataFile = SD.open("temp.txt", FILE_WRITE);
if (dataFile) {
    Serial.println("Menulis ke temp file...");
    dataFile.print(temp);
    dataFile.print('\r');
    // close the file:
    dataFile.close();
    Serial.println("Menyimpan temp file berhasil");
    } 
else {
    // if the file didn't open, print an error:
    Serial.println("Gagal buka temp.txt");
    }

//Save SD Card
//Membuat data string untuk menambah data di sd card
data_sd = String(jam);
data_sd += ":";
data_sd += String(menit);
data_sd += ":";
data_sd += String(detik);
data_sd += "\t";
data_sd += String(jumlah_produksi);
Serial.println("Menulis Data");
sprintf(nama_folder, "%d/%s", tahun_sd, nama2_bulan[bulan_sd]); //membuat nama folder (tahun/bulan)
sprintf(nama_file, "%s/%02d_Shft%d.csv", nama_folder, tgl_sd, shift); //membuat nama file menjadi file csv (tahun/bulan/tgl_Shftx.csv). nama file max 8 karakter
if(!SD.exists(nama_folder)){  //jika belum ada folder tsb, bikin dulu
    SD.mkdir(nama_folder);
    }
    
File dataFile = SD.open(nama_file, FILE_WRITE);
    if (dataFile) {
        if (jumlah_produksi==1) { //jika data pertama di file tsb, bikin judul dan header tabel dulu
        sprintf(judul, "%s, %d %s %d - Shift %d", daysOfTheWeek[urutan_hari], tgl, nama2_bulan[bulan], tahun, shift);
        header="Waktu";
        header+="\t";
        header+="Jumlah Produksi";
        dataFile.println("Monitoring Produksi Baterai PT GS Battery Plant Semarang");
        dataFile.println(judul);
        dataFile.println(header); 
        }
        else{ //jika bukan produksi pertama, cek state reset
            if(state_reset==1){ //jika state reset tanda arduino baru saja reset, tulis di file tsb tanda bahwa produksi sempat terhambat karena arduino reset
                dataFile.println("Produksi Sempat Terhambat");
                }
            }
        dataFile.println(data_sd);  //menulis data baterai
        dataFile.close();
        Serial.println("Selesai menyimpan di SD Card");
        }
        // if the file isn't open, pop up an error:
    else {
        Serial.println("Gagal menyimpan");
        }
}

//*Created by Benediktus Bryan B., Electrical Engineering, Diponegoro University*
//*For PT GS Battery Plant Semarang*
//*July 2018*
