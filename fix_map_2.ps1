$lines = Get-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Map.cpp" -Encoding UTF8
$newLines = @()
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($i -ge 1381 -and $i -le 1501) {
        if ($i -eq 1381) {
            $newLines += "    bool skip = false;"
            $newLines += "    for (const std::string &excluded : excludedNames) {"
            $newLines += "      if (groupName == excluded) {"
            $newLines += "        skip = true;"
            $newLines += "        break;"
            $newLines += "      }"
            $newLines += "    }"
        }
        continue
    }
    if ($i -eq 1745) {
        $newLines += $lines[$i]
        $newLines += "}"
        continue
    }
    $newLines += $lines[$i]
}
$newLines | Set-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Map.cpp" -Encoding UTF8
