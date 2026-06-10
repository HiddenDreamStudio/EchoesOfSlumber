$lines = Get-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Map.cpp" -Encoding UTF8
$newLines = @()
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($i -ge 181 -and $i -le 280) { continue }
    if ($i -ge 811 -and $i -le 1242) {
        if ($i -eq 811) {
            $newLines += "          }"
            $newLines += "          // IMMEDIATELY CALCULATE AND PIN CAMERA POSITION FOR PARALLAX"
            $newLines += "          Render* render = Engine::GetInstance().render.get();"
            $newLines += "          float vW = render->GetWorldViewportWidth();"
            $newLines += "          float vH = render->GetWorldViewportHeight();"
            $newLines += "          initCameraX = -(player->position.getX() - vW / 2.0f);"
            $newLines += "          initCameraY = -(player->position.getY() - vH * 0.75f);"
            $newLines += "          hasInitCamera = true;"
            $newLines += "          LOG(`"PARALLAX PINNED in LoadEntities: %f, %f`", initCameraX, initCameraY);"
            $newLines += "        } else if (entityType == `"Enemy`" || entityType == `"SpiderCandy`") {"
            $newLines += "          auto enemy = std::dynamic_pointer_cast<EnemyCarmel>("
            $newLines += "              Engine::GetInstance().entityManager->CreateEntity("
            $newLines += "                  EntityType::ENEMY));"
            $newLines += "          enemy->position = Vector2D(x, y);"
            $newLines += ""
            $newLines += "          float patrolLeft = x - 200.0f;"
            $newLines += "          float patrolRight = x + 200.0f;"
            $newLines += "          pugi::xml_node props = objectNode.child(`"properties`");"
            $newLines += "          if (props) {"
            $newLines += "            for (pugi::xml_node prop = props.child(`"property`"); prop;"
            $newLines += "                 prop = prop.next_sibling(`"property`")) {"
            $newLines += "              std::string propName = prop.attribute(`"name`").as_string();"
            $newLines += "              if (propName == `"patrol_left`")"
            $newLines += "                patrolLeft = prop.attribute(`"value`").as_float();"
            $newLines += "              if (propName == `"patrol_right`")"
            $newLines += "                patrolRight = prop.attribute(`"value`").as_float();"
            $newLines += "            }"
            $newLines += "          }"
            $newLines += "          enemy->SetPatrolPoints(patrolLeft, patrolRight);"
            $newLines += "          enemy->Start();"
            $newLines += "          LOG(`"Enemy (SpiderCandy) spawned at: %f, %f (patrol: %.0f-%.0f)`", x,"
            $newLines += "              y, patrolLeft, patrolRight);"
        }
        continue
    }
    $newLines += $lines[$i]
}
$newLines | Set-Content "C:\Users\aidan\Documents\GitHub\EchoesOfSlumber\src\Map.cpp" -Encoding UTF8
