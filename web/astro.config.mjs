// @ts-check
import { defineConfig } from 'astro/config';

// https://astro.build/config
export default defineConfig({
  // Set to the real Cloudflare hostname when known; used for canonical URLs + sitemaps.
  site: 'https://buddysystem.example',
  // Fully prerendered static output served by nginx. Do not add an SSR adapter.
  output: 'static',
  trailingSlash: 'ignore',
  // Clean URLs: /docs/getting-started/index.html — must stay in sync with nginx try_files.
  build: { format: 'directory' },
});
