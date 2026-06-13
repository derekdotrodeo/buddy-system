# Buddy System — website

The marketing site, feature pages, documentation, and blog for **Buddy System**, a
Eurorack-compatible breadboard prototyping platform. Built with [Astro](https://astro.build)
(static output), styled with vanilla CSS using design tokens ported from
`../docs/buddy-design-system.html`. Self-hosted in Docker behind a Cloudflare Tunnel.

> This is a scaffold with placeholder content. Real copy and full design come from the
> documents in `../docs/` in a later pass.

## Structure

```text
src/
├── content.config.ts        # blog + docs collections (Markdown)
├── styles/                  # tokens.css (ported design system) + global.css
├── components/              # BaseHead, Nav, Footer, Card
├── layouts/                 # BaseLayout, DocLayout, BlogPostLayout
├── pages/                   # index, features, docs/, blog/, 404
└── content/{docs,blog}/     # Markdown content
```

Add a doc: drop a `.md` in `src/content/docs/` (frontmatter: `title`, `order`, `section`).
Add a post: drop a `.md` in `src/content/blog/` (frontmatter: `title`, `description`,
`pubDate`, optional `tags`, `draft`).

## Develop

```sh
npm install
npm run dev        # http://localhost:4321
npm run build      # -> dist/
npm run preview    # serve the built output
npm run check      # type/diagnostics
```

## Deploy (Docker + Cloudflare Tunnel)

The Cloudflare Tunnel terminates TLS and routes the public hostname to `http://web:80`;
nginx serves the static `dist/` over plain HTTP. Configure the tunnel's public hostname
in the Cloudflare Zero Trust dashboard to point at `http://web:80`.

```sh
cp .env.example .env          # paste your TUNNEL_TOKEN
docker compose --env-file .env up --build -d
docker compose logs -f cloudflared   # expect "Registered tunnel connection"
```

Build/run just the web image locally (no tunnel):

```sh
docker build -t buddy-web .
docker run --rm -p 8080:80 buddy-web   # http://localhost:8080
```
