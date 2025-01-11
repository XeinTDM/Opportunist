function XOR-Encrypt {
    param (
        [string]$InputString,
        [byte]$Key
    )

    $bytes = [System.Text.Encoding]::UTF8.GetBytes($InputString)
    $encryptedBytes = foreach ($byte in $bytes) {
        [Convert]::ToString($byte -bxor $Key, 16).PadLeft(2, '0')
    }
    return $encryptedBytes
}

function Format-For-Cpp {
    param (
        [string[]]$EncryptedBytes
    )

    $allBytes = @("AA") + $EncryptedBytes + @("BB")
    $formatted = ($allBytes | ForEach-Object { "0x$_" }) -join ", "
    return $formatted
}

$botToken = Read-Host "Enter your bot token"
$chatId = Read-Host "Enter your chat ID"
$keyInput = Read-Host "Enter your encryption key (recommended to keep as 117)"
$key = if ($keyInput) { [byte]$keyInput } else { 117 }

$encryptedBotTokenBytes = XOR-Encrypt -InputString $botToken -Key $key
$formattedBotToken = Format-For-Cpp -EncryptedBytes $encryptedBotTokenBytes

$encryptedChatIdBytes = XOR-Encrypt -InputString $chatId -Key $key
$formattedChatId = Format-For-Cpp -EncryptedBytes $encryptedChatIdBytes

Write-Output "Encrypted Bot Token:"
Write-Output $formattedBotToken
Write-Output ""
Write-Output "Encrypted Chat ID:"
Write-Output $formattedChatId
