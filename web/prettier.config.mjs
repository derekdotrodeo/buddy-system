/** @type {import("prettier").Config} */
export default {
    plugins: ['prettier-plugin-astro'],
    tabWidth: 4,
    useTabs: false,
    semi: true,
    singleQuote: true,
    trailingComma: 'all',
    printWidth: 100,
    overrides: [
        {
            files: '*.astro',
            options: { parser: 'astro' },
        },
    ],
};
