$lines = Get-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Physics.cpp" -Encoding UTF8
$newLines = @()
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($i -ge 260 -and $i -le 364) {
        if ($i -eq 260) {
            $newLines += "    PhysBody* pbody = new PhysBody();"
            $newLines += "    pbody->body = b;"
            $newLines += "    b2Body_SetUserData(b, ToUserData(pbody));"
            $newLines += "    allBodies.push_back(pbody);"
            $newLines += ""
            $newLines += "    return pbody;"
            $newLines += "}"
        }
        continue
    }
    if ($i -ge 416 -and $i -le 433) {
        continue
    }
    if ($i -ge 445 -and $i -le 480) {
        if ($i -eq 445) {
            $newLines += "void Physics::FlushPendingDeletes() {"
            $newLines += "  for (PhysBody *physBody : bodiesToDelete) {"
            $newLines += "    if (physBody && b2Body_IsValid(physBody->body)) {"
            $newLines += "      b2DestroyBody(physBody->body);"
            $newLines += "    }"
            $newLines += "    delete physBody;"
            $newLines += "  }"
            $newLines += "  bodiesToDelete.clear();"
            $newLines += "}"
        }
        continue
    }
    $newLines += $lines[$i]
}
$newLines | Set-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Physics.cpp" -Encoding UTF8
