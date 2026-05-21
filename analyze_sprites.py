from PIL import Image

files = [
    ('der_a_izq', 'assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_der_a_izq.png'),
    ('izq_a_der', 'assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_izq_a_der.png'),
    ('Muro', 'assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_Muro.png'),
    ('Dead', 'assets/textures/spritesheets/SS_Enemics_Level1/spritesheet_Block_Crawler_Dead.png'),
]

for name, f in files:
    img = Image.open(f).convert('RGBA')
    w, h = img.size
    tw = 128
    cols = w // tw
    print(f"\n=== {name}: {w}x{h}, {cols} frames @ {tw}x{h} ===")
    non_empty = 0
    for i in range(cols):
        frame = img.crop((i*tw, 0, (i+1)*tw, h))
        bbox = frame.getbbox()
        if bbox:
            non_empty += 1
            print(f"  Frame {i}: content at {bbox}")
        else:
            print(f"  Frame {i}: empty")
    print(f"  Non-empty: {non_empty}")
