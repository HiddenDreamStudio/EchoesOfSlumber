const fs = require('fs');

let html = fs.readFileSync('index.html', 'utf8');

// Use a simpler string replacement for each one to be absolutely safe
const replacements = [
  { old: `<div class="sil-inner" style="background-image: url('images/asset-1-1-38.png'); box-shadow: inset 0 421px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-1-1-38.png'); background-color: #337fa5;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-37-for-1-35.png'); box-shadow: inset 0 477px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-37-for-1-35.png'); background-color: #337fa5;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-2-1-41.png'); box-shadow: inset 0 146px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-2-1-41.png'); background-color: #337fa5;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-7-1-39.png'); box-shadow: inset 0 146px 4px 0 #337fa6;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-7-1-39.png'); background-color: #337fa6;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-3-8-40.png'); box-shadow: inset 0 146px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-3-8-40.png'); background-color: #337fa5;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-9-3-44.png'); box-shadow: inset 0 146px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-9-3-44.png'); background-color: #337fa5;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-7-1-2-43.png'); box-shadow: inset 0 208px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-7-1-2-43.png'); background-color: #337fa5;"></div>` },
  { old: `<div class="sil-inner" style="background-image: url('images/asset-37-for-2-36.png'); box-shadow: inset 0 404px 4px 0 #337fa5;"></div>`, new: `<div class="sil-inner" style="--bg-img: url('images/asset-37-for-2-36.png'); background-color: #337fa5;"></div>` }
];

replacements.forEach(r => {
  html = html.replace(r.old, r.new);
});

fs.writeFileSync('index.html', html);
console.log('Success');
