import struct
import imghdr
import os

def get_image_size(fname):
    with open(fname, 'rb') as fhandle:
        head = fhandle.read(24)
        if len(head) != 24:
            return
        if imghdr.what(fname) == 'png':
            check = struct.unpack('>i', head[4:8])[0]
            if check != 0x0d0a1a0a:
                return
            width, height = struct.unpack('>ii', head[16:24])
            return width, height

paths = [
    r"C:\Users\Marc\Documents\GitHub\EchoesOfSlumber\assets\textures\spritesheets\SS_Enemics_Level1\spritesheet_Carmel_idle.png",
    r"C:\Users\Marc\Documents\GitHub\EchoesOfSlumber\assets\textures\spritesheets\SS_Enemics_Level1\spritesheet_Carmel_Roll.png",
    r"C:\Users\Marc\Documents\GitHub\EchoesOfSlumber\assets\textures\spritesheets\SS_Enemics_Level1\spritesheet_Carmel_scared.png",
    r"C:\Users\Marc\Documents\GitHub\EchoesOfSlumber\assets\textures\spritesheets\SS_Enemics_Level1\spritesheet_Carmel_Blowup.png",
]
for p in paths:
    print(f"{os.path.basename(p)}: {get_image_size(p)}")
