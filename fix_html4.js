const fs = require('fs');

// Fix index.html inline styles
let html = fs.readFileSync('index.html', 'utf8');

// Change top: \d+px; to bottom: 0; for all sil elements
html = html.replace(/<div class="sil( sil-flip)?" style="left: ([^;]+); top: \d+px; width: ([^;]+); height: ([^;]+);">/g, 
  '<div class="sil$1" style="left: $2; bottom: 0; width: $3; height: $4;">');

fs.writeFileSync('index.html', html);

// Fix styles.css for the logos
let css = fs.readFileSync('css/styles.css', 'utf8');

css = css.replace(/\.footer-logos-row \{[\s\S]*?z-index: 10;\n\}/, 
`.footer-logos-row {
    display: flex; align-items: center; justify-content: center;
    gap: 3rem; flex-wrap: wrap; margin-bottom: 0;
    position: absolute;
    top: 80px;
    width: 100%;
    z-index: 10;
}`);

// Ensure .footer-silhouettes-container is placed properly if logos are absolute
// Actually, I should just make the .container in the footer absolute or the row itself absolute.
// But .footer-logos-row is inside .container. .container has max-width: 1200px.
// If .footer-logos-row is absolute, it's absolute relative to .footer (which has position: relative).
// We should add position: relative to .footer. It already has it.
// To center it when absolute:
css = css.replace(/top: 80px;\n    width: 100%;/, `top: 80px;\n    left: 50%;\n    transform: translateX(-50%);\n    width: 100%;`);

fs.writeFileSync('css/styles.css', css);
console.log('Fixes applied');