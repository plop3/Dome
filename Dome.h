// WiFi
// STA
char ssid[40] = "astro";
char password[40] = "12345678";
// AP
char assid[40] = "dome";
char asecret[40] = "12345678";
#define local_IP IPAddress(10, 0, 0, 2)

// Firmata
//#define remote_ip IPAddress(10, 0, 0, 1)
#define remote_host "astro2.local"
#define remote_port 3030
