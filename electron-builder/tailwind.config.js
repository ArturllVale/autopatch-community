/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./src/renderer/index.html",
    "./src/renderer/**/*.{vue,js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        // Tema escuro estilo VS Code
        primary: '#1e1e1e',
        secondary: '#252526',
        tertiary: '#2d2d30',
        border: '#3e3e42',
        accent: '#0078d4',
        'accent-hover': '#1c8ae0',
        'text-primary': '#cccccc',
        'text-secondary': '#9d9d9d',
        success: '#4ec9b0',
        warning: '#dcdcaa',
        error: '#f14c4c',
      },
    },
  },
  plugins: [],
}
