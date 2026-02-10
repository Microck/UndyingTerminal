## Basics
- [x] Favicon (favicon.ico generated)
- [x] Web app manifest (manifest.json created)
- [x] Theme color meta tag (set in manifest & layout)
- [ ] robots.txt
- [ ] sitemap.xml
- [x] Proper viewport meta tag (in layout.tsx)

## Performance
- [x] Code splitting (Next.js default)
- [x] Minified CSS/JS (Next.js default)
- [x] Image optimization (Next.js unoptimized=true for export, verified in config)
- [x] Font loading strategy (Next.js font optimization used)
- [x] Cache headers (Cache-Control configured in vercel.json)

## SEO
- [x] Unique title tags per page
- [x] Meta description
- [ ] Canonical URLs
- [x] Open Graph tags
- [x] Twitter Card tags
- [x] Semantic HTML (nav, main, footer used)
- [x] Alt text for images (checked icons)

## Accessibility
- [x] Keyboard navigation support (interactive elements are standard anchors/buttons)
- [ ] Visible focus indicators
- [x] Color contrast (Dark mode default, verified text-muted-foreground contrast)
- [x] Associated form labels (No forms present)
- [ ] Skip links

## Security
- [x] HTTPS enforced (Vercel default)
- [x] Content Security Policy (via vercel.json headers)
- [x] X-Frame-Options (via vercel.json headers)
- [x] X-Content-Type-Options (via vercel.json headers)
- [x] Referrer-Policy (via vercel.json headers)
- [x] Permissions-Policy (via vercel.json headers)

## UI/UX
- [x] Responsive design
- [x] Touch targets >= 44x44 px (Navbar/buttons checked)
- [ ] Custom 404 page (Next.js default exists, check custom styling)
- [x] Dark mode support (Forced dark mode in layout)
- [x] Consistent navigation

## Legal & Privacy
- [ ] Privacy policy link (if needed for app store/distribution)
- [ ] Terms of service link
- [x] Contact information (GitHub/Docs links present)
- [x] Copyright notice

## Development & Deployment
- [x] `vercel.json` configuration
- [x] Build pipeline (`npm run build` works)
- [x] Linting (ESLint configured)
- [x] Clean URLs (Vercel default)
