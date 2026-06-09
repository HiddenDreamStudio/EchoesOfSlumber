const fs = require('fs');
let html = fs.readFileSync('index.html', 'utf8');

// Restore background-image instead of --bg-img
html = html.replace(/--bg-img: url\('([^']+)'\); background-color: #[a-fA-F0-9]+;/g, "background-image: url('$1');");

// Remove dl-btn-inner-shadow div
html = html.replace(/<div class="dl-btn-inner-shadow"><\/div>\s*/g, '');

fs.writeFileSync('index.html', html);
console.log('HTML restored');
