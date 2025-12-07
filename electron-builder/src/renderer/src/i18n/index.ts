import { createI18n } from 'vue-i18n'
import ptBR from './locales/pt-BR'
import enUS from './locales/en-US'

// Get saved locale or detect from system
function getDefaultLocale(): string {
  const saved = localStorage.getItem('autopatch-locale')
  if (saved) return saved
  
  const systemLocale = navigator.language
  if (systemLocale.startsWith('pt')) return 'pt-BR'
  return 'en-US'
}

const i18n = createI18n({
  legacy: false, // Use Composition API
  locale: getDefaultLocale(),
  fallbackLocale: 'en-US',
  messages: {
    'pt-BR': ptBR,
    'en-US': enUS
  }
})

export default i18n

// Helper to change locale
export function setLocale(locale: string) {
  i18n.global.locale.value = locale as 'pt-BR' | 'en-US'
  localStorage.setItem('autopatch-locale', locale)
}

export function getCurrentLocale(): string {
  return i18n.global.locale.value
}
