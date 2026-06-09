# Echoes of Slumber - Footer and Download Button Figma 1:1 Implementation

## Overview
The goal is to update the website's footer and download button to accurately reflect the Figma design. The current implementation uses CSS filters and flexbox for the footer, leading to visual distortion and missing properties like `box-shadow: inset`. The download button is missing its blue inner shadow.

## Architecture & Implementation Strategy

### 1. Download Button
- **Problem**: The current button uses an `<img>` tag for its background, which prevents applying an inset `box-shadow` directly.
- **Solution**:
  - Keep the base button structure.
  - Add a new `div` inside the button, positioned absolutely (`inset: 0`).
  - Apply `pointer-events: none` to ensure it doesn't block clicks.
  - Apply the Figma specified inset shadow: `box-shadow: inset 0 146px 4px 0 #487cb7`.

### 2. Footer Silhouettes
- **Problem**: The current flexbox + CSS filter approach distorts the assets and doesn't match Figma's exact positioning and visual style (specifically the inset shadows).
- **Solution**:
  - Replace the current flexbox layout with a relative container based on a 1440px baseline width (Figma's reference width).
  - Use `transform: scale()` or a responsive aspect-ratio wrapper to ensure it scales down correctly on smaller screens.
  - Replace `<img>` tags with absolute positioned `div` elements.
  - Each `div` will have:
    - Exact `width`, `height`, `left`, and `top` coordinates derived from Figma.
    - `opacity: 0.2`.
    - `background: url(<asset-path>) lightgray 50% / cover no-repeat`.
    - The specific `box-shadow: inset` value extracted from the Figma node properties.

### 3. Footer Logos
- **Problem**: Logos might not be properly sized or spaced according to the exact Figma layout.
- **Solution**:
  - Update the `.footer-logos-row` and `.footer-logo` CSS.
  - Ensure the heights match Figma exactly (`36px` to `84px` depending on the logo).
  - Ensure the gap/spacing accurately reflects the Figma 1440px layout.

## Testing & Validation
- Verify the download button's blue inner shadow appears correctly and the button remains clickable.
- Verify the footer silhouettes do not overflow the page inappropriately and scale down accurately on smaller screens without distortion.
- Visually compare the footer layout against the Figma reference to confirm exact mapping.
