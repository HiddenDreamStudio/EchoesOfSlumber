// =============================================
// ECHOES OF SLUMBER — script.js
// =============================================

// =============================================
// INTRO LOADING & SPLASH SEQUENCE
// =============================================
window.addEventListener('load', () => {
    const loadingScreen = document.getElementById('loading-screen');
    const splash = document.getElementById('splash-screen');
    const citm = document.getElementById('citm-splash');
    const studio = document.getElementById('studio-splash');
    
    let isSkipped = false;
    const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
    
    function revealWebsite() {
        document.body.classList.add('loaded');
    }
    
    function skip() {
        if (isSkipped) return;
        isSkipped = true;
        splash.classList.remove('active');
        splash.classList.add('hidden');
        revealWebsite();
    }

    async function startTimeline() {
        // Show splash screen immediately (rendered behind loading-screen)
        splash.classList.add('active');

        // 1. Show loading screen for 3 seconds
        await delay(3000);
        
        // Hide loading screen (reveals the black splash screen background)
        loadingScreen.classList.add('hidden');
        
        // Listeners for skip
        splash.addEventListener('click', skip);
        window.addEventListener('keydown', (e) => {
            if (e.code === 'Space' || e.code === 'Enter') {
                skip();
            }
        });
        
        if (isSkipped) return;
        
        // --- 2. CITM SPLASH ---
        citm.style.display = 'flex';
        requestAnimationFrame(() => {
            requestAnimationFrame(() => {
                citm.style.opacity = '1';
                citm.style.transform = 'scale(1.05)';
            });
        });
        
        // CITM fade-in (800ms) + hold (1500ms) = 2300ms
        await delay(2300);
        if (isSkipped) return;
        
        // CITM fade-out (800ms)
        citm.style.opacity = '0';
        await delay(800);
        citm.style.display = 'none';
        if (isSkipped) return;
        
        // --- 3. STUDIO SPLASH ---
        studio.style.display = 'flex';
        requestAnimationFrame(() => {
            requestAnimationFrame(() => {
                studio.style.opacity = '1';
                studio.style.transform = 'scale(1.05)';
            });
        });
        
        // Studio fade-in (800ms) + hold (1500ms) = 2300ms
        await delay(2300);
        if (isSkipped) return;
        
        // Studio fade-out (800ms)
        studio.style.opacity = '0';
        await delay(800);
        studio.style.display = 'none';
        if (isSkipped) return;
        
        // --- 4. END SPLASH ---
        splash.classList.remove('active');
        splash.classList.add('hidden');
        revealWebsite();
    }
    
    startTimeline();
});

// Navbar scroll effect
window.addEventListener('scroll', () => {
    const navbar = document.getElementById('navbar');
    if (window.scrollY > 80) navbar.classList.add('scrolled');
    else navbar.classList.remove('scrolled');
});

// Mobile menu toggle
const hamburger = document.getElementById('hamburger');
const navMenu = document.getElementById('nav-menu');

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
    anchor.addEventListener('click', function (e) {
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
    const versionTag = document.getElementById('version-tag');
    const versionName = document.getElementById('version-name');
    const versionDate = document.getElementById('version-date');
    const fileSizeEl = document.getElementById('file-size');
    const downloadBtn = document.getElementById('download-btn');

    try {
        const response = await fetch(`https://api.github.com/repos/${REPO}/releases/latest`);
        if (!response.ok) throw new Error('No release found');
        const data = await response.json();

        // Version tag — derive phase from tag name
        const tag = data.tag_name || '';
        let phase = 'Latest Release';
        if (/gold/i.test(tag)) phase = 'Gold Release';
        else if (/alpha/i.test(tag)) phase = 'Alpha Release';
        else if (/vertical/i.test(tag)) phase = 'Vertical Slice Release';

        if (versionTag) versionTag.textContent = 'LATEST VERSION';
        if (versionName) versionName.textContent = `${phase}`;

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
        if (versionDate) versionDate.textContent = 'Released: 10 June 2026';
        if (downloadBtn) downloadBtn.href = 'https://github.com/HiddenDreamStudio/EchoesOfSlumber/releases/download/gold-release/EchoesOfSlumber_GoldRelease.zip';
        if (fileSizeEl) fileSizeEl.textContent = '(1805.1 MB)';
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
const sequence = ['ArrowUp', 'ArrowUp', 'ArrowDown', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'ArrowLeft', 'ArrowRight', 'KeyB', 'KeyA'];

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
function initParticlesForCanvas(canvasId) {
    const canvas = document.getElementById(canvasId);
    if (!canvas) return;
    const ctx = canvas.getContext('2d');

    let width, height;
    let particles = [];

    // Mouse tracking
    let mouse = {
        x: null,
        y: null,
        radius: 150 // increased interaction radius for a more dramatic effect
    };

    const parent = canvas.parentElement;
    parent.addEventListener('mousemove', (e) => {
        const rect = parent.getBoundingClientRect();
        mouse.x = e.clientX - rect.left;
        mouse.y = e.clientY - rect.top;
    });

    parent.addEventListener('mouseleave', () => {
        mouse.x = null;
        mouse.y = null;
    });

    function resize() {
        width = canvas.width = canvas.offsetWidth || window.innerWidth;
        height = canvas.height = canvas.offsetHeight || window.innerHeight;
    }

    class Particle {
        constructor() {
            this.x = Math.random() * width;
            this.y = Math.random() * height;
            this.size = Math.random() * 2.5 + 0.5;
            this.baseVx = (Math.random() - 0.5) * 0.4;
            this.baseVy = (Math.random() - 0.5) * 0.4 - 0.2; // slight drift up
            this.vx = this.baseVx;
            this.vy = this.baseVy;
            this.opacity = Math.random() * 0.5 + 0.1;
        }
        update() {
            // Check mouse interaction
            if (mouse.x !== null && mouse.y !== null) {
                const dx = this.x - mouse.x;
                const dy = this.y - mouse.y;
                const distance = Math.hypot(dx, dy);
                if (distance < mouse.radius) {
                    const force = (mouse.radius - distance) / mouse.radius; // 0 to 1
                    const angle = Math.atan2(dy, dx);
                    
                    // Swarm vortex pull: pull in towards cursor but orbit it
                    // Pull force (attraction)
                    const pullX = Math.cos(angle) * force * 0.5;
                    const pullY = Math.sin(angle) * force * 0.5;
                    
                    // Orbit force (perpendicular)
                    const orbitX = -Math.sin(angle) * force * 0.8;
                    const orbitY = Math.cos(angle) * force * 0.8;
                    
                    // Apply acceleration
                    this.vx += -pullX + orbitX;
                    this.vy += -pullY + orbitY;
                }
            }

            // Damping / Restoring velocity back to original gentle drift
            this.vx += (this.baseVx - this.vx) * 0.04;
            this.vy += (this.baseVy - this.vy) * 0.04;

            // Cap the maximum velocity
            const speed = Math.hypot(this.vx, this.vy);
            const maxSpeed = 3.0;
            if (speed > maxSpeed) {
                this.vx = (this.vx / speed) * maxSpeed;
                this.vy = (this.vy / speed) * maxSpeed;
            }

            this.x += this.vx;
            this.y += this.vy;

            // wrap around with margin so it looks natural
            const margin = 20;
            if (this.x < -margin) this.x = width + margin;
            if (this.x > width + margin) this.x = -margin;
            if (this.y < -margin) this.y = height + margin;
            if (this.y > height + margin) this.y = -margin;
        }
        draw() {
            let size = this.size;
            let opacity = this.opacity;
            let glow = 8;

            // Grow and glow when near mouse
            if (mouse.x !== null && mouse.y !== null) {
                const dx = this.x - mouse.x;
                const dy = this.y - mouse.y;
                const distance = Math.hypot(dx, dy);
                if (distance < mouse.radius) {
                    const factor = (mouse.radius - distance) / mouse.radius;
                    size += factor * 1.5;
                    opacity = Math.min(0.9, opacity + factor * 0.45);
                    glow += factor * 12;
                }
            }

            ctx.beginPath();
            ctx.arc(this.x, this.y, size, 0, Math.PI * 2);
            ctx.fillStyle = `rgba(86, 145, 211, ${opacity})`;
            ctx.fill();

            // glow
            ctx.shadowBlur = glow;
            ctx.shadowColor = '#5691d3';
        }
    }

    function drawConnections() {
        ctx.shadowBlur = 0; // disable glow for line drawing performance
        const maxDist = 90;
        
        // Draw constellation lines between particles
        for (let i = 0; i < particles.length; i++) {
            for (let j = i + 1; j < particles.length; j++) {
                const p1 = particles[i];
                const p2 = particles[j];
                const dx = p1.x - p2.x;
                const dy = p1.y - p2.y;
                const distance = Math.hypot(dx, dy);
                
                if (distance < maxDist) {
                    ctx.beginPath();
                    ctx.moveTo(p1.x, p1.y);
                    ctx.lineTo(p2.x, p2.y);
                    
                    // Opacity fades with distance
                    const opacity = (1 - distance / maxDist) * 0.16 * Math.min(p1.opacity, p2.opacity);
                    ctx.strokeStyle = `rgba(86, 145, 211, ${opacity})`;
                    ctx.lineWidth = 0.5;
                    ctx.stroke();
                }
            }
        }

        // Also draw faint line from mouse to nearby particles
        if (mouse.x !== null && mouse.y !== null) {
            particles.forEach(p => {
                const dx = p.x - mouse.x;
                const dy = p.y - mouse.y;
                const distance = Math.hypot(dx, dy);
                if (distance < mouse.radius) {
                    ctx.beginPath();
                    ctx.moveTo(p.x, p.y);
                    ctx.lineTo(mouse.x, mouse.y);
                    const opacity = (1 - distance / mouse.radius) * 0.22;
                    ctx.strokeStyle = `rgba(86, 145, 211, ${opacity})`;
                    ctx.lineWidth = 0.7;
                    ctx.stroke();
                }
            });
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
        drawConnections();
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

function initParticles() {
    initParticlesForCanvas('particles-canvas');
    initParticlesForCanvas('download-particles-canvas');
}

window.addEventListener('load', initParticles);