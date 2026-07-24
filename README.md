# Shuttle

**Modern**SH Workspace for Developers and**ystem Administrators**

Shuttle is a lightweight, privacy-focused SSH client designed for managing remote Linux servers from a modern desktop interface.

It combines:

- SSH terminal access
- SFTP file management
- SSH tunnels
- Remote system monitoring
- Automatic reconnection

All inside a single application.

No cloud.
No telemetry.
No subscriptions.

---

## Features

### SSH Terminal

- Interactive SSH sessions
- Multiple tabs
- Public key authentication
- Password authentication
- Automatic session reconnection
- Terminal title detection
- Scrollback history
- Copy / Paste support
- ANSI color support
- UTF‑8 support

### SFTP File Manager

- Browse remote files
- Upload files
- Download files
- Rename files and folders
- Delete files and folders
- Create directories
- Permission display
- Automatic reconnection

### SSH Tunnels

- Local port forwarding
- Tunnel management from the UI
- System tray integration
- One-click connect / disconnect

### Remote Monitoring

Monitor your servers in real time:

- CPU usage
- Memory usage
- Network throughput
- Disk utilization
- SSH sessions
- Interactive users

Directly from the status bar.

### Profile Management

- Saved SSH profiles
- Encrypted profile storage
- Password protection
- Private key support

### Desktop Integration

- System tray support
- Persistent sessions
- Single instance protection
- Multi-language support

### Languages

- French
- English
- Breton
- Japanese

---

## Security

Shuttle stores connection profiles locally using encryption.

Supported authentication methods:

- Password
- Private key
- Private key with passphrase

No remote services are required.

No connection metadata is transmitted to third-party providers.

---

## Build

### Requirements

- Visual Studio 2022
- Qt 6
- OpenSSL
- libssh2
- CMake

### Build

```powershell
.\Build.ps1 all_secure_dynamic Shuttle
```

Generated artifacts will be available in:

```text
Deploy/
├── Windows/
├── Installer/
└── Shuttle.AppImage
```

---

## Why Shuttle?

Most SSH clients focus on only one task:

- Terminal access
- File transfer
- Tunnel management

Shuttle combines all of them into a single application.

Open a terminal, browse remote files, monitor system resources and manage tunnels without switching tools.

---

## License

MIT License

Use it.
Modify it.
Fork it.

No warranty.

---

## Author

**Spiritecho**

If Shuttle helps you:

⭐ Star the repository

🐛 Report bugs

💡 Suggest improvements

🍴 Fork the project

---

# Shuttle

**Connect. Transfer. Monitor.**
