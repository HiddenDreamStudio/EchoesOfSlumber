// =============================================
// ECHOES OF SLUMBER — script.js
// =============================================

// Loading Screen — 3s
window.addEventListener('load', () => {
    const loadingScreen = document.getElementById('loading-screen');
    setTimeout(() => {
        loadingScreen.classList.add('hidden');
    }, 3000);
});

// Navbar scroll effect
window.addEventListener('scroll', () => {
    const navbar = document.getElementById('navbar');
    if (window.scrollY > 80) navbar.classList.add('scrolled');
    else navbar.classList.remove('scrolled');
});

// Mobile menu toggle
const hamburger = document.getElementById('hamburger');
const navMenu   = document.getElementById('nav-menu');

hamburger.addEventListener('click', () => {
    hamburger.classList.toggle('active');
    navMenu.classList.toggle('active');
    document.body.style.overflow = navMenu.classList.contains('active') ? 'hidden' : '';
});

document.querySelectorAll('.nav-link').forEach(link => {
    link.addEventListener('click', () => {
        hamburger.classList.remove('active');
        navMenu.classList.remove('active');
        document.body.style.overflow = '';
    });
});

document.addEventListener('click', (e) => {
    if (!hamburger.contains(e.target) && !navMenu.contains(e.target)) {
        hamburger.classList.remove('active');
        navMenu.classList.remove('active');
        document.body.style.overflow = '';
    }
});

// Smooth scroll
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function(e) {
        e.preventDefault();
        const target = document.querySelector(this.getAttribute('href'));
        if (target) target.scrollIntoView({ behavior: 'smooth', block: 'start' });
    });
});

// Tab system
document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        const tabId = btn.dataset.tab;
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
        btn.classList.add('active');
        const panel = document.getElementById('tab-' + tabId);
        if (panel) panel.classList.add('active');
    });
});

// Scroll reveal with stagger
const revealObserver = new IntersectionObserver((entries) => {
    entries.forEach((entry, i) => {
        if (entry.isIntersecting) {
            setTimeout(() => entry.target.classList.add('revealed'), i * 90);
            revealObserver.unobserve(entry.target);
        }
    });
}, { threshold: 0.12, rootMargin: '0px 0px -40px 0px' });

document.addEventListener('DOMContentLoaded', () => {
    document.querySelectorAll('.scroll-reveal').forEach(el => revealObserver.observe(el));
    fetchLatestRelease();
});

// Parallax on hero bg
window.addEventListener('scroll', () => {
    const scrollY = window.pageYOffset;
    const heroBg = document.querySelector('.hero-bg-levels');
    if (heroBg && scrollY < window.innerHeight) {
        heroBg.style.transform = `translateY(${scrollY * 0.25}px)`;
    }
});

// =============================================
// GITHUB RELEASE FETCH
// Fetches latest release info and updates version tag, name, date and file size
// =============================================
async function fetchLatestRelease() {
    const REPO = 'HiddenDreamStudio/EchoesOfSlumber'; // ← update when repo is public
    const versionTag  = document.getElementById('version-tag');
    const versionName = document.getElementById('version-name');
    const versionDate = document.getElementById('version-date');
    const fileSizeEl  = document.getElementById('file-size');
    const downloadBtn = document.getElementById('download-btn');

    try {
        const response = await fetch(`https://api.github.com/repos/${REPO}/releases/latest`);
        if (!response.ok) throw new Error('No release found');
        const data = await response.json();

        // Version tag — derive phase from tag name
        const tag = data.tag_name || '';
        let phase = 'Latest Release';
        if (/gold/i.test(tag))        phase = 'Gold Release';
        else if (/alpha/i.test(tag))  phase = 'Alpha Release';
        else if (/beta/i.test(tag))   phase = 'Beta Release';
        else if (/silver/i.test(tag)) phase = 'Silver Release';

        if (versionTag)  versionTag.textContent  = 'LATEST VERSION';
        if (versionName) versionName.textContent  = `${phase} ${tag}`;

        // Release date
        if (versionDate && data.published_at) {
            const date = new Date(data.published_at);
            versionDate.textContent = `Released: ${date.toLocaleDateString('en-GB', { day: 'numeric', month: 'long', year: 'numeric' })}`;
        }

        // Find main zip asset
        const gameAsset = data.assets?.find(a => a.name.includes('.zip') || a.name.includes('.exe'));
        if (gameAsset) {
            // Update download link
            if (downloadBtn) downloadBtn.href = gameAsset.browser_download_url;

            // File size
            if (fileSizeEl) {
                const sizeInMB = (gameAsset.size / (1024 * 1024)).toFixed(1);
                fileSizeEl.style.opacity = '0';
                setTimeout(() => {
                    fileSizeEl.textContent = `(${sizeInMB} MB)`;
                    fileSizeEl.style.opacity = '1';
                    fileSizeEl.style.transition = 'opacity 0.3s ease';
                }, 100);
            }
        } else {
            if (fileSizeEl) fileSizeEl.textContent = '';
        }

    } catch (error) {
        console.warn('Could not fetch release info:', error.message);
        // Fallback values
        if (versionName) versionName.textContent = 'Gold Release';
        if (versionDate) versionDate.textContent  = 'Released: 2025';
        if (fileSizeEl)  fileSizeEl.textContent   = '';
    }
}

// =============================================
// DOWNLOAD BUTTON TRACKING
// =============================================
document.querySelector('#download-btn')?.addEventListener('click', () => {
    console.log('Game download initiated');
    // Add analytics here if needed
});

// =============================================
// EASTER EGG: KONAMI CODE
// =============================================
let konamiCode = [];
const sequence = ['ArrowUp','ArrowUp','ArrowDown','ArrowDown','ArrowLeft','ArrowRight','ArrowLeft','ArrowRight','KeyB','KeyA'];

document.addEventListener('keydown', (e) => {
    konamiCode.push(e.code);
    if (konamiCode.length > sequence.length) konamiCode.shift();

    if (konamiCode.join(',') === sequence.join(',')) {
        // Dream glitch effect
        document.body.style.filter = 'hue-rotate(180deg) saturate(200%)';
        setTimeout(() => {
            document.body.style.filter = 'hue-rotate(0deg) saturate(100%)';
            document.body.style.transition = 'filter 1s ease';
            setTimeout(() => { document.body.style.filter = ''; }, 1000);
        }, 2000);
        console.log('✨ You found the truth... or did you?');
    }
});

// =============================================
// HERO JS PARTICLES
// =============================================
function initParticles() {
    const canvas = document.getElementById('particles-canvas');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    let width, height;
    let particles = [];

    function resize() {
        width = canvas.width = window.innerWidth;
        height = canvas.height = canvas.parentElement.offsetHeight || window.innerHeight;
    }

    class Particle {
        constructor() {
            this.x = Math.random() * width;
            this.y = Math.random() * height;
            this.size = Math.random() * 2.5 + 0.5;
            this.speedX = (Math.random() - 0.5) * 0.4;
            this.speedY = (Math.random() - 0.5) * 0.4 - 0.2; // slight drift up
            this.opacity = Math.random() * 0.5 + 0.1;
            this.color = '#337FA5';
        }
        update() {
            this.x += this.speedX;
            this.y += this.speedY;

            // wrap around
            if (this.x < 0) this.x = width;
            if (this.x > width) this.x = 0;
            if (this.y < 0) this.y = height;
            if (this.y > height) this.y = 0;
        }
        draw() {
            ctx.beginPath();
            ctx.arc(this.x, this.y, this.size, 0, Math.PI * 2);
            ctx.fillStyle = `rgba(51, 127, 165, ${this.opacity})`;
            ctx.fill();
            
            // glow
            ctx.shadowBlur = 10;
            ctx.shadowColor = '#5691d3';
        }
    }

    function createParticles() {
        particles = [];
        const count = Math.min(window.innerWidth / 15, 100); // responsive count
        for (let i = 0; i < count; i++) {
            particles.push(new Particle());
        }
    }

    function animate() {
        ctx.clearRect(0, 0, width, height);
        particles.forEach(p => {
            p.update();
            p.draw();
        });
        requestAnimationFrame(animate);
    }

    window.addEventListener('resize', () => {
        resize();
        createParticles();
    });

    resize();
    createParticles();
    animate();
}

window.addEventListener('DOMContentLoaded', initParticles);