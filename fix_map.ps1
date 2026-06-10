$lines = Get-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Map.cpp" -Encoding UTF8
$newLines = @()
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($i -ge 1356 -and $i -le 1366) { continue }
    if ($i -ge 1600 -and $i -le 1635) {
        if ($i -eq 1600) {
            $newLines += "          }"
            $newLines += "        }"
            $newLines += "      }"
        }
        continue
    }
    if ($lines[$i] -match "render->DrawTexture\(deco->texture, drawX, \(int\)\(deco->y - deco->height\),") {
        $newLines += $lines[$i]
        $i++
        $newLines += "                          nullptr, 1.0f, 1.0f, deco->rotation, 0, (int)deco->height,"
        $i++
        $newLines += $lines[$i]
        $i++
        $newLines += $lines[$i]
        continue
    }
    $newLines += $lines[$i]
}
$newLines | Set-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Map.cpp" -Encoding UTF8
