from PIL import Image
import os

base = 'assets/textures/spritesheets/SS_Enemics_Level1'
names = ['SP_BlocCrawler_move', 'SP_BlocCrawler_wall', 'SP_BlocCrawler_hit', 'SP_BlocCrawler_die']

for n in names:
    img = Image.open(os.path.join(base, n + '.png'))
    w, h = img.size
    print(f"{n}: {w}x{h}")
    for fw in [64, 96, 128, 160, 192, 224, 256, 320]:
        if w % fw == 0:
            print(f"  {fw}px wide -> {w // fw} frames")
