# RegionalDSKSFC — Project Website

This branch contains the static website for the **RegionalDSKSFC** research project,  
served by GitHub Pages at **https://sonianmol.github.io/RegionalDSKSFC/**.

---

## File structure

```
index.html      ← main page (edit this to update content)
style.css       ← all styles
script.js       ← mobile nav, scroll highlights, last-updated date
.nojekyll       ← disables Jekyll processing on GitHub Pages
assets/
  slides.pdf    ← drop your slide deck here
```

---

## How to update each section

### 1. Google Docs meeting notes link
In `index.html`, find `MEETING_NOTES_LINK` and replace it with your Google Docs share URL:
```html
<a href="https://docs.google.com/document/d/YOUR_DOC_ID/edit?usp=sharing" ...>
```

### 2. Slides PDF
1. Add or replace `assets/slides.pdf` with the updated deck.
2. Open `script.js` and update the `SLIDES_DATE` string to reflect the new date:
   ```js
   var SLIDES_DATE = 'March 18, 2026';
   ```

### 3. Conferences
In `index.html`, find the `#conferences` section. Copy any `.conf-card` block and fill in:
- `conf-date` — e.g. `June 2026`
- `conf-name` — full conference name
- `conf-location` — city, country
- `conf-desc` — brief description of the contribution

Move a card from the **Upcoming** grid to the **Past** grid after the event occurs,
and change `class="conf-card upcoming"` → `class="conf-card past"`.

### 4. Contact email
In `index.html`, find the `mailto:` anchor and update the address:
```html
<a href="mailto:your.actual@email.com" ...>
```

### 5. Working paper kicker
In `index.html`, update the `<p class="hero-kicker">` line with the correct status/date.

---

## Deployment

Any `git push origin gh-pages` from this directory automatically updates the live site  
(GitHub Pages rebuilds within ~60 seconds).

To push from this worktree:
```bash
cd /path/to/gh-pages-worktree
git add .
git commit -m "Update slides / add conference / ..."
git push origin gh-pages
```
