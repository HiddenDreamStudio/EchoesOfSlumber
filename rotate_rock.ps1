Add-Type -AssemblyName System.Drawing
$src = "C:\Users\victo\OneDrive\Documentos\GitHub\EchoesOfSlumber\assets\textures\AS_environment\redissenys assets_Nivell 1\AS_roca_escondite_01.png"
$dst = "C:\Users\victo\OneDrive\Documentos\GitHub\EchoesOfSlumber\assets\textures\AS_environment\redissenys assets_Nivell 1\AS_roca_escondite_01_rot180.png"
$img = [System.Drawing.Image]::FromFile($src)
$bmp = New-Object System.Drawing.Bitmap $img
$bmp.RotateFlip([System.Drawing.RotateFlipType]::Rotate180FlipNone)
$bmp.Save($dst, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
$img.Dispose()
Write-Host "Done - saved rotated image to $dst"
