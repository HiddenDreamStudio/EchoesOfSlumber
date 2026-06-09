# Footer and Download Button Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the exact Figma layout for the footer silhouettes, correct the logos placement, and add the missing inner shadow to the download button.

**Architecture:** We will replace the current flexbox/filter-based footer implementation with an absolute-positioned scaling container based on a 1440px virtual width. The download button will receive a new absolute-positioned inner div for its shadow.

**Tech Stack:** HTML5, CSS3

---

### Task 1: Update the Download Button HTML and CSS

**Files:**
- Modify: `index.html`
- Modify: `css/styles.css`

- [ ] **Step 1: Add inner shadow div to HTML**
Update the `<div class="dl-btn-inner-full">` wrapper to include the new shadow overlay in `index.html`.

```html
<!-- Inside index.html near <section id="download" ... -->
<a href="https://github.com/HiddenDreamStudio" target="_blank" class="dl-btn-full" id="download-btn">
    <img src="images/boto1-141.png" alt="" class="dl-btn-bg-full">
    <div class="dl-btn-inner-full">
        <!-- New shadow overlay -->
        <div class="dl-btn-inner-shadow"></div>
        <img src="images/downloading-updates-143.png" alt="" class="dl-icon-full">
        <span class="dl-btn-label-full">DOWNLOAD</span>
        <span class="file-size" id="file-size">(loading...)</span>
    </div>
</a>
```

- [ ] **Step 2: Add CSS for the inner shadow**
Update `css/styles.css` to add the `.dl-btn-inner-shadow` style.

```css
/* Add to css/styles.css around line 431 */
.dl-btn-inner-shadow {
    position: absolute;
    inset: 0;
    border-radius: inherit;
    box-shadow: inset 0 146px 4px 0 #487cb7;
    pointer-events: none;
    z-index: -1;
}
.dl-btn-inner-full {
    /* ensure inner full can position the absolute shadow correctly */
    position: absolute;
    inset: 0;
    display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 0.1rem;
    pointer-events: none;
}
```

- [ ] **Step 3: Commit the changes**
```bash
git add index.html css/styles.css
git commit -m "fix: add blue inner shadow to download button"
```

### Task 2: Update Footer HTML Structure

**Files:**
- Modify: `index.html`

- [ ] **Step 1: Replace `.footer-silhouettes` HTML**
Replace the current `.footer-silhouettes` div in `index.html` with the new absolute-positioned structure.

```html
<!-- Replace the existing <div class="footer-silhouettes">...</div> with: -->
<div class="footer-silhouettes-container">
    <div class="footer-silhouettes-canvas">
        <div class="sil" style="left: -27px; top: 0px; width: 224.265px; height: 364px;">
            <div class="sil-inner" style="background-image: url('images/asset-1-1-38.png'); box-shadow: inset 0 421px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil" style="left: 149px; top: 25px; width: 97.492px; height: 309px;">
            <div class="sil-inner" style="background-image: url('images/asset-37-for-1-35.png'); box-shadow: inset 0 477px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil" style="left: 246px; top: 133px; width: 98.026px; height: 182px;">
            <div class="sil-inner" style="background-image: url('images/asset-7-1-1-42.png'); box-shadow: inset 0 208px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil" style="left: 469px; top: 195px; width: 128.087px; height: 112px;">
            <div class="sil-inner" style="background-image: url('images/asset-2-1-41.png'); box-shadow: inset 0 146px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil" style="left: 623px; top: 195px; width: 193.103px; height: 112px;">
            <div class="sil-inner" style="background-image: url('images/asset-7-1-39.png'); box-shadow: inset 0 146px 4px 0 #337fa6;"></div>
        </div>
        <div class="sil" style="left: 850px; top: 195px; width: 95.197px; height: 112px;">
            <div class="sil-inner" style="background-image: url('images/asset-3-8-40.png'); box-shadow: inset 0 146px 4px 0 #337fa5;"></div>
        </div>
        <!-- Rotated elements -->
        <div class="sil sil-flip" style="left: 981px; top: 181px; width: 96.965px; height: 133px;">
            <div class="sil-inner" style="background-image: url('images/asset-9-3-44.png'); box-shadow: inset 0 146px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil sil-flip" style="left: 1096px; top: 132px; width: 98.026px; height: 182px;">
            <div class="sil-inner" style="background-image: url('images/asset-7-1-2-43.png'); box-shadow: inset 0 208px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil sil-flip" style="left: 1194px; top: 25px; width: 97.492px; height: 309px;">
            <div class="sil-inner" style="background-image: url('images/asset-37-for-2-36.png'); box-shadow: inset 0 404px 4px 0 #337fa5;"></div>
        </div>
        <div class="sil" style="left: 1243px; top: 0px; width: 268.042px; height: 334px;">
            <div class="sil-inner" style="background-image: url('images/asset-4-1-37.png'); box-shadow: inset 0 359px 4px 0 #337fa5;"></div>
        </div>
    </div>
</div>
```

- [ ] **Step 2: Commit the changes**
```bash
git add index.html
git commit -m "refactor: update footer html to match figma 1440px layout"
```

### Task 3: Update Footer CSS

**Files:**
- Modify: `css/styles.css`

- [ ] **Step 1: Replace footer CSS rules**
Replace `.footer-silhouettes` and `.sil-asset` rules with the new canvas layout in `css/styles.css`.

```css
/* Replace old .footer-silhouettes and .sil-asset rules around line 454 with: */
.footer-silhouettes-container {
    width: 100%;
    position: relative;
    overflow: hidden;
    /* Maintain aspect ratio of the 1440x364 Figma block */
    aspect-ratio: 1440 / 364;
    max-height: 364px;
    background: transparent;
}
.footer-silhouettes-canvas {
    position: absolute;
    top: 0;
    left: 50%;
    transform: translateX(-50%);
    width: 1440px;
    height: 364px;
    transform-origin: top center;
}
/* Responsive scale down if viewport < 1440px */
@media (max-width: 1440px) {
    .footer-silhouettes-canvas {
        transform: translateX(-50%) scale(calc(100vw / 1440));
    }
}
.sil {
    position: absolute;
    opacity: 0.2;
    pointer-events: none;
}
.sil.sil-flip {
    transform: scaleX(-1);
}
.sil-inner {
    position: absolute;
    inset: 0;
    background-color: lightgray;
    background-position: 50%;
    background-size: cover;
    background-repeat: no-repeat;
    border-radius: inherit;
}
```

- [ ] **Step 2: Update Logo alignment**
Ensure the `.footer-logos-row` and `.footer-logo` use exact Figma sizing.

```css
/* Update .footer-logo and .footer-logos-row around line 482 */
.footer-logos-row {
    display: flex; align-items: center; justify-content: center;
    gap: 3rem; flex-wrap: wrap; margin-bottom: 1.25rem;
    position: relative;
    z-index: 10;
}
.footer-logo {
    height: auto; width: auto; opacity: 0.7;
    filter: brightness(0) invert(1);
    transition: opacity var(--tr);
}
.footer-logo:nth-child(1) { height: 56.57px; } /* Hidden Studios */
.footer-logo:nth-child(2) { height: 84px; }    /* CITM/UPC */
.footer-logo:nth-child(3) { height: 56.57px; } /* Echoes of Slumber */
.footer-logo:hover { opacity: 1; }
```

- [ ] **Step 3: Commit the changes**
```bash
git add css/styles.css
git commit -m "style: apply exact figma CSS for footer layout and logos"
```
