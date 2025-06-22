# Goodix 27C6:5125 Linux Driver (Reverse Engineering)

This is an attempt to develop a Linux driver for the Goodix fingerprint sensor `27c6:5125`. While some progress has been made, key issues remain unresolved.

This project reverse-engineers the USB protocol of the original Windows driver, allowing communication with the device over USB using `libusb`. A 32-byte zero PSK is used for TLS, and I have managed to establish a TLS connection with the device (or at least I believe I have).

![image](https://github.com/user-attachments/assets/c22b2bbf-9189-4a79-8ef7-2956f766ce00)

The main problem: the device does not send the image in response to the `McuGetImage` command.

![image](https://github.com/user-attachments/assets/92946193-0c84-4d41-b5b4-7f24da0d94bc)

I don't fully understand why this happens. One hypothesis is that the original Windows driver performs **a single bulk IN transfer before** sending the command, while in my implementation, I do **two bulk IN transfers after** the command — which might be causing a sync issue with the internal device state machine.

## 🚀 Features

- USB communication with Goodix device via `libusb`
- TLS handshake with the embedded MCU (mimicking the Windows driver behavior)
- Querying device state and switching MCU modes (e.g., to FDT mode)
- Attempts to acquire fingerprint images (currently failing)

## 🔧 Dependencies

- [`libusb-1.0`](https://libusb.info/)
- [`OpenSSL`](https://www.openssl.org/) (for TLS)
- Standard C toolchain (`gcc`, `make`)

## 🛠 How to Run

Before launching the driver, start a local TLS server that mimics the behavior of the embedded server the device expects. The driver connects to it and performs a handshake over this connection.

You can use [`OpenSSL`](https://www.openssl.org/) or a custom minimal TLS echo server with PSK support. For example:

```bash
openssl s_server \
  -nocert \
  -psk 0000000000000000000000000000000000000000000000000000000000000000 \
  -port 5353 \
  -quiet \
  -debug \
  -cipher PSK-AES128-GCM-SHA256
```

## ⚠️ Disclaimer

This is an experimental and unofficial project. Use it at your own risk. I am not affiliated with Goodix in any way.

## 📬 Contact

Feel free to open issues or discussions on GitHub.

GitHub: [available314](https://github.com/available314)
