import js from '@eslint/js';
import tseslint from 'typescript-eslint';
import astro from 'eslint-plugin-astro';
import prettier from 'eslint-config-prettier/flat';
import globals from 'globals';

export default tseslint.config(
    { ignores: ['dist/', '.astro/', 'node_modules/'] },
    js.configs.recommended,
    ...tseslint.configs.recommended,
    ...astro.configs.recommended,
    {
        languageOptions: {
            globals: { ...globals.browser, ...globals.node },
        },
    },
    // keep formatting concerns to Prettier — disable conflicting ESLint rules
    prettier,
);
