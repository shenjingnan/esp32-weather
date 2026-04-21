import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import oxlintPlugin from 'vite-plugin-oxlint';

export default defineConfig({
  plugins: [preact(), oxlintPlugin()],

  server: {
    port: 3000,
    proxy: {
      '/api': {
        target: 'http://esp32-weather.local',
        changeOrigin: true,
      },
    },
  },

  build: {
    target: 'es2020',
    outDir: 'dist',
    assetsInlineLimit: 4096,
    cssCodeSplit: false,
    rolldownOptions: {
      output: {
        minify: true,
      },
    },
  },

  optimizeDeps: {
    include: ['preact'],
  },
});
