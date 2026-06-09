import re

with open('index.html', 'r', encoding='utf-8') as f:
    content = f.read()

pattern = r'<div class="sil-inner" style="background-image: url\(\'([^\']+)\'\); box-shadow: inset 0 \d+px 4px 0 (#[a-fA-F0-9]+);"></div>'
replacement = r'<div class="sil-inner" style="--bg-img: url(\'\1\'); background-color: \2;"></div>'

new_content = re.sub(pattern, replacement, content)

with open('index.html', 'w', encoding='utf-8') as f:
    f.write(new_content)

print("Done")
