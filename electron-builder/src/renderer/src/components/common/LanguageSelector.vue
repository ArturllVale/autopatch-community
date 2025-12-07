<script setup lang="ts">
import { ref, computed } from 'vue'
import { useI18n } from 'vue-i18n'
import { setLocale } from '../../i18n'

const { locale } = useI18n()
const isOpen = ref(false)

const languages = [
  { code: 'pt-BR', name: 'Português (BR)', flag: '🇧🇷' },
  { code: 'en-US', name: 'English (US)', flag: '🇺🇸' }
]

const currentLanguage = computed(() => {
  return languages.find(l => l.code === locale.value) || languages[0]
})

function selectLanguage(code: string) {
  setLocale(code)
  isOpen.value = false
}

function toggleDropdown() {
  isOpen.value = !isOpen.value
}

// Close dropdown when clicking outside
function handleClickOutside(event: MouseEvent) {
  const target = event.target as HTMLElement
  if (!target.closest('.language-selector')) {
    isOpen.value = false
  }
}

// Add/remove click listener
import { onMounted, onUnmounted } from 'vue'
onMounted(() => document.addEventListener('click', handleClickOutside))
onUnmounted(() => document.removeEventListener('click', handleClickOutside))
</script>

<template>
  <div class="language-selector">
    <button class="language-button" @click.stop="toggleDropdown" :title="$t('settings.language')">
      <span class="flag">{{ currentLanguage.flag }}</span>
      <span class="code">{{ currentLanguage.code.split('-')[0].toUpperCase() }}</span>
      <svg class="chevron" :class="{ open: isOpen }" viewBox="0 0 24 24" width="14" height="14">
        <path fill="currentColor" d="M7 10l5 5 5-5z"/>
      </svg>
    </button>
    
    <div v-if="isOpen" class="language-dropdown">
      <button
        v-for="lang in languages"
        :key="lang.code"
        class="language-option"
        :class="{ active: lang.code === locale }"
        @click="selectLanguage(lang.code)"
      >
        <span class="flag">{{ lang.flag }}</span>
        <span class="name">{{ lang.name }}</span>
        <svg v-if="lang.code === locale" class="check" viewBox="0 0 24 24" width="16" height="16">
          <path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/>
        </svg>
      </button>
    </div>
  </div>
</template>

<style scoped>
.language-selector {
  position: relative;
}

.language-button {
  display: flex;
  align-items: center;
  gap: 3px;
  padding: 2.5px 6px;
  background: #2d2d30;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #e0e0e0;
  cursor: pointer;
  transition: all 0.15s ease;
  font-size: 10px;
}

.language-button:hover {
  background: #3e3e42;
  border-color: #555;
}

.flag {
  font-size: 12px;
}

.code {
  font-weight: 500;
}

.chevron {
  transition: transform 0.2s ease;
  opacity: 0.6;
}

.chevron.open {
  transform: rotate(180deg);
}

.language-dropdown {
  position: absolute;
  bottom: 100%;
  left: 0;
  margin-bottom: 4px;
  background: #252526;
  border: 1px solid #3e3e42;
  border-radius: 6px;
  overflow: hidden;
  box-shadow: 0 -4px 12px rgba(0, 0, 0, 0.4);
  z-index: 1000;
  min-width: 150px;
}

.language-option {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
  padding: 8px 10px;
  background: transparent;
  border: none;
  color: #e0e0e0;
  cursor: pointer;
  transition: background 0.15s ease;
  font-size: 12px;
  text-align: left;
}

.language-option:hover {
  background: #333;
}

.language-option.active {
  background: #3a3a3a;
  color: #ff9800;
}

.language-option .name {
  flex: 1;
}

.check {
  color: #ff9800;
}
</style>
