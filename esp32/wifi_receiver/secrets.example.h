// Copy this file to secrets.h in the same folder and fill it in.
// secrets.h is ignored by git, so your WiFi password is never committed.

#define WIFI_SSID  "your hotspot name"
#define WIFI_PASS  "your hotspot password"

// The PC's address on the hotspot: run `ipconfig` on the PC and use the IPv4
// address of the WiFi adapter. It must be the PC that runs replay_dataset.py.
#define PC_IP      "192.168.43.100"
#define PC_PORT    5000
