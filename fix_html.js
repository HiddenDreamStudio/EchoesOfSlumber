const fs = require('fs');
let html = fs.readFileSync('index.html', 'utf8');
html = html.replace(/<div class=\"sil-inner\" style=\"background-image: url\('([^']+)'\); box-shadow: inset 0 \d+px 4px 0 (#[a-fA-F0-9]+);\">/g, '<div class="sil-inner" style="--bg-img: url(\\'$1\\'); background-color: $2;">');
fs.writeFileSync('index.html', html);
console.log('Success');