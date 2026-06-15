import { defineCollection } from 'astro:content';
import { z } from 'astro:schema';
import { glob } from 'astro/loaders';

const blog = defineCollection({
    loader: glob({ pattern: '**/*.md', base: './src/content/blog' }),
    schema: z.object({
        title: z.string(),
        // Optional HTML title for the article header (e.g. an <em> accent word);
        // falls back to `title`.
        titleHtml: z.string().optional(),
        description: z.string(),
        // Longer standfirst shown under the header; falls back to `description`.
        deck: z.string().optional(),
        pubDate: z.coerce.date(),
        updatedDate: z.coerce.date().optional(),
        author: z.string().default('Buddy System'),
        authorInitials: z.string().optional(),
        authorRole: z.string().optional(),
        authorBio: z.string().optional(),
        tags: z.array(z.string()).default([]),
        draft: z.boolean().default(false),

        /* ---- blog design-language fields (BD-LOG) ---- */
        // identity hue — "color is wiring"
        family: z.enum(['ctl', 'snd', 'dat', 'lgc', 'uty', 'pwr']).default('snd'),
        // mono kicker line in the header, e.g. "ANNOUNCEMENT · BD-LOG-001"
        kicker: z.string().optional(),
        // breadcrumb leaf category, e.g. "ANNOUNCEMENTS"
        crumb: z.string().optional(),
        // show the all-families legend strip in the kicker
        famStrip: z.boolean().default(false),
        readTime: z.string().optional(),
        postType: z.string().optional(),
        hero: z
            .object({
                src: z.string(),
                alt: z.string().default(''),
                caption: z.string().optional(),
                ref: z.string().optional(),
            })
            .optional(),
        toc: z.array(z.object({ id: z.string(), label: z.string() })).default([]),
        prev: z.object({ href: z.string(), title: z.string() }).optional(),
        next: z.object({ href: z.string(), title: z.string() }).optional(),
        related: z
            .array(
                z.object({
                    href: z.string(),
                    cat: z.string(),
                    title: z.string(),
                    meta: z.string().optional(),
                    family: z.enum(['ctl', 'snd', 'dat', 'lgc', 'uty', 'pwr']).default('dat'),
                }),
            )
            .default([]),
    }),
});

const docs = defineCollection({
    loader: glob({ pattern: '**/*.md', base: './src/content/docs' }),
    schema: z.object({
        title: z.string(),
        description: z.string().optional(),
        order: z.number().default(0),
        section: z.string().default('Guide'),
    }),
});

export const collections = { blog, docs };
