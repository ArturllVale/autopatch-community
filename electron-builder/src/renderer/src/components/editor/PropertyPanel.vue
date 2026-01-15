<script setup lang="ts">
import { computed, ref } from 'vue'
import { useI18n } from 'vue-i18n'
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'

const { t } = useI18n()
const projectStore = useProjectStore()
const uiStore = useUiStore()

const element = computed(() => projectStore.selectedElement)

// Layers section expanded state
const isLayersExpanded = ref(true)

// Active tab for button states
const activeStateTab = ref<'normal' | 'hover' | 'pressed' | 'disabled'>('normal')

// Layers - sorted by z-index (highest first)
const sortedElements = computed(() => {
  return [...projectStore.project.config.elements].sort((a, b) => (b.zIndex || 0) - (a.zIndex || 0))
})

// Actions for buttons - computed to be reactive to locale changes
const buttonActions = computed(() => [
  { value: 'start_game', label: t('properties.actions.startGame') },
  { value: 'check_files', label: t('properties.actions.checkFiles') },
  { value: 'settings', label: t('properties.actions.settings') },
  { value: 'website', label: t('properties.actions.website') },
  { value: 'close', label: t('properties.actions.close') },
  { value: 'exit', label: t('properties.actions.exit') },
  { value: 'minimize', label: t('properties.actions.minimize') }
])

// Text alignment options - computed to be reactive
const textAlignOptions = computed(() => [
  { value: 'left', label: '◀ ' + t('properties.align.left') },
  { value: 'center', label: '⬛ ' + t('properties.align.center') },
  { value: 'right', label: '▶ ' + t('properties.align.right') }
])

const verticalAlignOptions = computed(() => [
  { value: 'top', label: '▲ ' + t('properties.align.top') },
  { value: 'middle', label: '⬛ ' + t('properties.align.middle') },
  { value: 'bottom', label: '▼ ' + t('properties.align.bottom') }
])

function updateProperty(key: string, value: any) {
  if (element.value) {
    projectStore.updateElement(element.value.id, { [key]: value })
  }
}

function updateNestedProperty(parent: string, key: string, value: any) {
  if (element.value) {
    const current = (element.value as any)[parent] || {}
    projectStore.updateElement(element.value.id, {
      [parent]: { ...current, [key]: value }
    })
  }
}

function updateStateProperty(state: string, key: string, value: any) {
  if (element.value) {
    const states = element.value.states || {}
    const currentState = (states as any)[state] || {}
    projectStore.updateElement(element.value.id, {
      states: {
        ...states,
        [state]: { ...currentState, [key]: value }
      }
    })
  }
}

function updateEffectProperty(effect: string, key: string, value: any) {
  if (element.value) {
    const effects = element.value.effects || {}
    const currentEffect = (effects as any)[effect] || {}
    projectStore.updateElement(element.value.id, {
      effects: {
        ...effects,
        [effect]: { ...currentEffect, [key]: value }
      }
    })
  }
}

function updateBoxStyle(key: string, value: any) {
  if (element.value) {
    const boxStyle = element.value.boxStyle || {
      fillColor: '#000000',
      fillOpacity: 50,
      borderColor: '#ffffff',
      borderWidth: 1,
      borderRadius: 8
    }
    projectStore.updateElement(element.value.id, {
      boxStyle: { ...boxStyle, [key]: value }
    })
  }
}

function updateWebviewConfig(key: string, value: any) {
  if (element.value) {
    const webviewConfig = element.value.webviewConfig || {
      url: 'https://example.com',
      borderRadius: 8,
      borderColor: '#333333',
      borderWidth: 1,
      backgroundColor: '#1e1e1e'
    }
    projectStore.updateElement(element.value.id, {
      webviewConfig: { ...webviewConfig, [key]: value }
    })
  }
}

function updateProgressBar(key: string, value: any) {
  const progressBar = { ...projectStore.project.config.progressBar, [key]: value }
  projectStore.updateConfig({ progressBar })
}

// Video background functions
const videoConfig = computed(() => projectStore.project.config.videoBackground)
const videoBtnConfig = computed(() => {
  return videoConfig.value?.controlButton || {
    x: 10,
    y: 10,
    size: 40,
    backgroundColor: '#333333',
    iconColor: '#ffffff',
    borderColor: '#666666',
    borderWidth: 2,
    opacity: 1.0
  }
})

function updateVideoConfig(key: string, value: any) {
  const current = projectStore.project.config.videoBackground || {
    enabled: false,
    path: '',
    showControls: true,
    autoplay: true,
    loop: true,
    muted: true,
    controlButton: videoBtnConfig.value
  }
  projectStore.updateConfig({
    videoBackground: { ...current, [key]: value }
  })
}

function updateVideoButtonConfig(key: string, value: any) {
  const currentVideo = videoConfig.value || {
    enabled: false,
    path: '',
    showControls: true,
    autoplay: true,
    loop: true,
    muted: true,
    controlButton: videoBtnConfig.value
  }
  projectStore.updateConfig({
    videoBackground: {
      ...currentVideo,
      controlButton: {
        ...videoBtnConfig.value,
        [key]: value
      }
    }
  })
}

async function selectVideoFile() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('properties.selectVideo'),
      filters: [{ name: t('properties.videos'), extensions: ['mp4', 'webm', 'avi', 'wmv'] }]
    })
    if (result) {
      updateVideoConfig('path', result)
    }
  } catch (e) {
    console.error(e)
  }
}

// Layer functions
function selectLayerElement(id: string) {
  projectStore.selectElement(id)
  uiStore.selectProgressBar(false)
  uiStore.selectVideoButton(false)
}

function toggleVisibility(id: string) {
  const el = projectStore.project.config.elements.find(e => e.id === id)
  if (el) {
    projectStore.updateElement(id, { visible: el.visible === false ? true : false })
  }
}

function toggleLock(id: string) {
  const el = projectStore.project.config.elements.find(e => e.id === id)
  if (el) {
    projectStore.updateElement(id, { locked: !el.locked })
  }
}

function moveLayerUp(id: string) {
  projectStore.moveLayerUp(id)
}

function moveLayerDown(id: string) {
  projectStore.moveLayerDown(id)
}

function deleteLayerElement(id: string) {
  projectStore.removeElement(id)
}

function getTypeIcon(type: string) {
  switch (type) {
    case 'button': return '🔲'
    case 'label': return '📝'
    case 'status': return '📊'
    case 'percentage': return '%'
    case 'box': return '📦'
    case 'image': return '🖼️'
    case 'webview': return '🌐'
    default: return '▢'
  }
}

function getLayerElementName(el: any) {
  // For elements with text content, show text first
  if (el.type === 'button' || el.type === 'label' || el.type === 'status' || el.type === 'percentage') {
    return el.text || el.name || `${el.type} ${el.id.slice(-4)}`
  }
  // For other elements, show name or type
  return el.name || `${el.type} ${el.id.slice(-4)}`
}

async function selectStateImage(state: string) {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('properties.selectImageState', { state }),
      filters: [{ name: t('properties.images'), extensions: ['png', 'jpg', 'jpeg', 'bmp'] }]
    })
    if (result) {
      updateStateProperty(state, 'imagePath', result)
    }
  } catch (e) {
    console.error(e)
  }
}

async function selectBackgroundImage() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('properties.selectBackgroundImage'),
      filters: [{ name: t('properties.images'), extensions: ['png', 'jpg', 'jpeg', 'bmp'] }]
    })
    if (result) {
      updateProperty('backgroundImage', result)
    }
  } catch (e) {
    console.error(e)
  }
}

function getStateValue(state: string, key: string, defaultValue: any = '') {
  if (!element.value?.states) return defaultValue
  const stateObj = (element.value.states as any)[state]
  return stateObj?.[key] ?? defaultValue
}

function getEffectValue(key: string, defaultValue: any = '') {
  if (!element.value?.effects) return defaultValue
  return (element.value.effects as any)[key] ?? defaultValue
}

function getShadowValue(key: string, defaultValue: any = '') {
  if (!element.value?.effects?.shadow) return defaultValue
  return (element.value.effects.shadow as any)[key] ?? defaultValue
}

function getGlowValue(key: string, defaultValue: any = '') {
  if (!element.value?.effects?.glow) return defaultValue
  return (element.value.effects.glow as any)[key] ?? defaultValue
}
</script>

<template>
  <div class="property-panel">
    <div class="panel-header">
      <h3>{{ t('properties.title') }}</h3>
    </div>

    <div class="panel-content">
      <!-- No selection -->
      <div v-if="!element" class="no-selection">
        <div class="empty-icon">
          <svg viewBox="0 0 24 24" fill="currentColor" width="48" height="48">
            <path d="M3 5v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2zm12 4c0 1.66-1.34 3-3 3s-3-1.34-3-3 1.34-3 3-3 3 1.34 3 3zm-9 8c0-2 4-3.1 6-3.1s6 1.1 6 3.1v1H6v-1z"/>
          </svg>
        </div>
        <p>{{ t('properties.selectToEdit') }}</p>
      </div>

      <!-- Element properties -->
      <template v-else>
        <!-- Type indicator & Name -->
        <div class="property-section">
          <div class="element-header">
            <span class="type-badge" :class="element.type">{{ element.type }}</span>
            <input
              type="text"
              :value="element.name || ''"
              @input="updateProperty('name', ($event.target as HTMLInputElement).value)"
              :placeholder="t('properties.elementName')"
              class="element-name"
            />
          </div>
        </div>

        <!-- Position & Size -->
        <div class="property-section">
          <div class="section-title">📐 {{ t('properties.positionSize') }}</div>
          <div class="property-grid-4">
            <label>
              <span>X</span>
              <input
                type="number"
                :value="element.x"
                @input="updateProperty('x', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
            <label>
              <span>Y</span>
              <input
                type="number"
                :value="element.y"
                @input="updateProperty('y', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
            <label>
              <span>W</span>
              <input
                type="number"
                :value="element.width"
                @input="updateProperty('width', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
            <label>
              <span>H</span>
              <input
                type="number"
                :value="element.height"
                @input="updateProperty('height', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
          </div>
          <div class="property-row">
            <label>
              <span>Z-Index</span>
              <input
                type="number"
                :value="element.zIndex || 0"
                @input="updateProperty('zIndex', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
            <label class="checkbox-label">
              <input
                type="checkbox"
                :checked="element.visible !== false"
                @change="updateProperty('visible', ($event.target as HTMLInputElement).checked)"
              />
              <span>Visível</span>
            </label>
            <label class="checkbox-label">
              <input
                type="checkbox"
                :checked="element.locked === true"
                @change="updateProperty('locked', ($event.target as HTMLInputElement).checked)"
              />
              <span>Travado</span>
            </label>
          </div>
        </div>

        <!-- Text (not for box/image) -->
        <div v-if="element.type !== 'box' && element.type !== 'image'" class="property-section">
          <div class="section-title">📝 {{ t('properties.text') }}</div>
          <input
            type="text"
            :value="element.text"
            @input="updateProperty('text', ($event.target as HTMLInputElement).value)"
            class="full-width"
            :placeholder="t('properties.elementText')"
          />

          <!-- Font settings -->
          <div class="property-row mt-8">
            <label class="flex-2">
              <span>{{ t('properties.font') }}</span>
              <select
                :value="element.fontName || 'Segoe UI'"
                @change="updateProperty('fontName', ($event.target as HTMLSelectElement).value)"
              >
                <option value="Segoe UI">Segoe UI</option>
                <option value="Arial">Arial</option>
                <option value="Helvetica">Helvetica</option>
                <option value="Verdana">Verdana</option>
                <option value="Tahoma">Tahoma</option>
                <option value="Trebuchet MS">Trebuchet MS</option>
                <option value="Times New Roman">Times New Roman</option>
                <option value="Georgia">Georgia</option>
                <option value="Garamond">Garamond</option>
                <option value="Courier New">Courier New</option>
                <option value="Monaco">Monaco</option>
                <option value="Comic Sans MS">Comic Sans MS</option>
                <option value="Impact">Impact</option>
                <option value="Lucida Sans">Lucida Sans</option>
                <option value="Palatino">Palatino</option>
              </select>
            </label>
            <label class="flex-1">
              <span>{{ t('properties.size') }}</span>
              <input
                type="number"
                :value="element.fontSize || 12"
                @input="updateProperty('fontSize', Number(($event.target as HTMLInputElement).value))"
                min="6"
                max="72"
              />
            </label>
          </div>

          <div class="property-row">
            <label class="checkbox-label">
              <input
                type="checkbox"
                :checked="element.fontBold === true"
                @change="updateProperty('fontBold', ($event.target as HTMLInputElement).checked)"
              />
              <span><b>Bold</b></span>
            </label>
            <label class="checkbox-label">
              <input
                type="checkbox"
                :checked="element.fontItalic === true"
                @change="updateProperty('fontItalic', ($event.target as HTMLInputElement).checked)"
              />
              <span><i>{{ t('properties.italic') }}</i></span>
            </label>
            <label>
              <span>{{ t('properties.color') }}</span>
              <input
                type="color"
                :value="element.fontColor || '#ffffff'"
                @input="updateProperty('fontColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
          </div>

          <!-- Text alignment -->
          <div class="property-row">
            <label>
              <span>{{ t('properties.alignH') }}</span>
              <select
                :value="element.textAlign || 'center'"
                @change="updateProperty('textAlign', ($event.target as HTMLSelectElement).value)"
              >
                <option v-for="opt in textAlignOptions" :key="opt.value" :value="opt.value">
                  {{ opt.label }}
                </option>
              </select>
            </label>
            <label>
              <span>{{ t('properties.alignV') }}</span>
              <select
                :value="element.textVerticalAlign || 'middle'"
                @change="updateProperty('textVerticalAlign', ($event.target as HTMLSelectElement).value)"
              >
                <option v-for="opt in verticalAlignOptions" :key="opt.value" :value="opt.value">
                  {{ opt.label }}
                </option>
              </select>
            </label>
          </div>
        </div>

        <!-- Action (for buttons only) -->
        <div v-if="element.type === 'button'" class="property-section">
          <div class="section-title">⚡ {{ t('properties.action') }}</div>
          <select
            :value="element.action || 'start_game'"
            @change="updateProperty('action', ($event.target as HTMLSelectElement).value)"
            class="full-width"
          >
            <option v-for="action in buttonActions" :key="action.value" :value="action.value">
              {{ action.label }}
            </option>
          </select>
        </div>

        <!-- Button States (for buttons only) -->
        <div v-if="element.type === 'button'" class="property-section">
          <div class="section-title">🎨 {{ t('properties.buttonStates') }}</div>

          <div class="state-tabs">
            <button
              v-for="state in ['normal', 'hover', 'pressed', 'disabled']"
              :key="state"
              :class="['state-tab', { active: activeStateTab === state }]"
              @click="activeStateTab = state as any"
            >
              {{ state === 'normal' ? t('properties.stateNormalEmoji') :
                 state === 'hover' ? t('properties.stateHoverEmoji') :
                 state === 'pressed' ? t('properties.statePressedEmoji') : t('properties.stateDisabledEmoji') }}
            </button>
          </div>

          <div class="state-properties">
            <!-- Image for state -->
            <div class="property-row">
              <label class="full-width">
                <span>{{ t('properties.imageLabel') }}</span>
                <div class="file-input">
                  <input
                    type="text"
                    :value="getStateValue(activeStateTab, 'imagePath', '')"
                    readonly
                    :placeholder="t('properties.noImage')"
                  />
                  <button @click="selectStateImage(activeStateTab)">📂</button>
                </div>
              </label>
            </div>

            <!-- Colors for state -->
            <div class="property-row">
              <label>
                <span>{{ t('properties.backgroundLabel') }}</span>
                <input
                  type="color"
                  :value="getStateValue(activeStateTab, 'backgroundColor', '#0078d4')"
                  @input="updateStateProperty(activeStateTab, 'backgroundColor', ($event.target as HTMLInputElement).value)"
                />
              </label>
              <label>
                <span>{{ t('properties.textLabel') }}</span>
                <input
                  type="color"
                  :value="getStateValue(activeStateTab, 'fontColor', '#ffffff')"
                  @input="updateStateProperty(activeStateTab, 'fontColor', ($event.target as HTMLInputElement).value)"
                />
              </label>
              <label>
                <span>{{ t('properties.borderLabel') }}</span>
                <input
                  type="color"
                  :value="getStateValue(activeStateTab, 'borderColor', '#005a9e')"
                  @input="updateStateProperty(activeStateTab, 'borderColor', ($event.target as HTMLInputElement).value)"
                />
              </label>
            </div>

            <!-- Offset for pressed state -->
            <div v-if="activeStateTab === 'pressed'" class="property-row">
              <label>
                <span>{{ t('properties.offsetX') }}</span>
                <input
                  type="number"
                  :value="getStateValue('pressed', 'offsetX', 0)"
                  @input="updateStateProperty('pressed', 'offsetX', Number(($event.target as HTMLInputElement).value))"
                />
              </label>
              <label>
                <span>{{ t('properties.offsetY') }}</span>
                <input
                  type="number"
                  :value="getStateValue('pressed', 'offsetY', 2)"
                  @input="updateStateProperty('pressed', 'offsetY', Number(($event.target as HTMLInputElement).value))"
                />
              </label>
            </div>

            <!-- Opacity for disabled state -->
            <div v-if="activeStateTab === 'disabled'" class="property-row">
              <label class="full-width">
                <span>{{ t('properties.opacityLabel') }}: {{ getStateValue('disabled', 'opacity', 60) }}%</span>
                <input
                  type="range"
                  min="0"
                  max="100"
                  :value="getStateValue('disabled', 'opacity', 60)"
                  @input="updateStateProperty('disabled', 'opacity', Number(($event.target as HTMLInputElement).value))"
                />
              </label>
            </div>
          </div>
        </div>

        <!-- Box Style (for box only) -->
        <div v-if="element.type === 'box'" class="property-section">
          <div class="section-title">📦 {{ t('properties.boxStyle') }}</div>

          <div class="property-row">
            <label>
              <span>{{ t('properties.fillColor') }}</span>
              <input
                type="color"
                :value="element.boxStyle?.fillColor || '#000000'"
                @input="updateBoxStyle('fillColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
            <label>
              <span>{{ t('properties.opacityLabel') }}: {{ element.boxStyle?.fillOpacity || 50 }}%</span>
              <input
                type="range"
                min="0"
                max="100"
                :value="element.boxStyle?.fillOpacity || 50"
                @input="updateBoxStyle('fillOpacity', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
          </div>

          <div class="property-row">
            <label>
              <span>{{ t('properties.borderColor') }}</span>
              <input
                type="color"
                :value="element.boxStyle?.borderColor || '#ffffff'"
                @input="updateBoxStyle('borderColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
            <label>
              <span>{{ t('properties.thickness') }}</span>
              <input
                type="number"
                :value="element.boxStyle?.borderWidth || 1"
                @input="updateBoxStyle('borderWidth', Number(($event.target as HTMLInputElement).value))"
                min="0"
                max="20"
              />
            </label>
            <label>
              <span>{{ t('properties.radius') }}</span>
              <input
                type="number"
                :value="element.boxStyle?.borderRadius || 8"
                @input="updateBoxStyle('borderRadius', Number(($event.target as HTMLInputElement).value))"
                min="0"
                max="50"
              />
            </label>
          </div>
        </div>

        <!-- Image element -->
        <div v-if="element.type === 'image'" class="property-section">
          <div class="section-title">🖼️ {{ t('properties.image') }}</div>
          <div class="file-input full-width">
            <input
              type="text"
              :value="element.backgroundImage || ''"
              readonly
              :placeholder="t('properties.selectImagePlaceholder')"
            />
            <button @click="selectBackgroundImage">📂</button>
          </div>
        </div>

        <!-- WebView element -->
        <div v-if="element.type === 'webview'" class="property-section webview-section">
          <div class="section-title">🌐 {{ t('properties.webview') }}</div>
          <div class="property-row full-width">
            <label class="full-width">
              <span>{{ t('properties.pageUrl') }}</span>
              <input
                type="text"
                :value="element.webviewConfig?.url || 'https://example.com'"
                @input="updateWebviewConfig('url', ($event.target as HTMLInputElement).value)"
                :placeholder="t('properties.pageUrlPlaceholder')"
              />
            </label>
          </div>
          <div class="info-box">
            💡 {{ t('properties.webviewTip') }}
          </div>
          <div class="property-row">
            <label>
              <span>{{ t('properties.backgroundColor') }}</span>
              <input
                type="color"
                :value="element.webviewConfig?.backgroundColor || '#1e1e1e'"
                @input="updateWebviewConfig('backgroundColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
            <label>
              <span>{{ t('properties.borderColor') }}</span>
              <input
                type="color"
                :value="element.webviewConfig?.borderColor || '#333333'"
                @input="updateWebviewConfig('borderColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
          </div>
          <div class="property-row">
            <label>
              <span>{{ t('properties.thickness') }}</span>
              <input
                type="number"
                :value="element.webviewConfig?.borderWidth || 1"
                @input="updateWebviewConfig('borderWidth', Number(($event.target as HTMLInputElement).value))"
                min="0"
                max="10"
              />
            </label>
            <label>
              <span>{{ t('properties.borderRadius') }}</span>
              <input
                type="number"
                :value="element.webviewConfig?.borderRadius || 8"
                @input="updateWebviewConfig('borderRadius', Number(($event.target as HTMLInputElement).value))"
                min="0"
                max="50"
              />
            </label>
          </div>
        </div>

        <!-- Effects (all elements) -->
        <div class="property-section">
          <div class="section-title">✨ {{ t('properties.effects') }}</div>

          <!-- Opacity -->
          <div class="property-row">
            <label class="full-width">
              <span>{{ t('properties.opacityLabel') }}: {{ getEffectValue('opacity', 100) }}%</span>
              <input
                type="range"
                min="0"
                max="100"
                :value="getEffectValue('opacity', 100)"
                @input="updateNestedProperty('effects', 'opacity', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
          </div>

          <!-- Border Radius -->
          <div class="property-row">
            <label class="full-width">
              <span>{{ t('properties.roundedBorder') }}: {{ getEffectValue('borderRadius', 0) }}px</span>
              <input
                type="range"
                min="0"
                max="50"
                :value="getEffectValue('borderRadius', 0)"
                @input="updateNestedProperty('effects', 'borderRadius', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
          </div>

          <!-- Shadow -->
          <div class="effect-group">
            <label class="checkbox-label">
              <input
                type="checkbox"
                :checked="getShadowValue('enabled', false)"
                @change="updateEffectProperty('shadow', 'enabled', ($event.target as HTMLInputElement).checked)"
              />
              <span>{{ t('properties.shadowLabel') }}</span>
            </label>

            <div v-if="getShadowValue('enabled', false)" class="effect-details">
              <div class="property-row">
                <label>
                  <span>{{ t('properties.colorLabel') }}</span>
                  <input
                    type="color"
                    :value="getShadowValue('color', '#000000')"
                    @input="updateEffectProperty('shadow', 'color', ($event.target as HTMLInputElement).value)"
                  />
                </label>
                <label>
                  <span>{{ t('properties.blur') }}</span>
                  <input
                    type="number"
                    :value="getShadowValue('blur', 10)"
                    @input="updateEffectProperty('shadow', 'blur', Number(($event.target as HTMLInputElement).value))"
                    min="0"
                    max="50"
                  />
                </label>
              </div>
              <div class="property-row">
                <label>
                  <span>{{ t('properties.offsetX') }}</span>
                  <input
                    type="number"
                    :value="getShadowValue('offsetX', 0)"
                    @input="updateEffectProperty('shadow', 'offsetX', Number(($event.target as HTMLInputElement).value))"
                  />
                </label>
                <label>
                  <span>{{ t('properties.offsetY') }}</span>
                  <input
                    type="number"
                    :value="getShadowValue('offsetY', 4)"
                    @input="updateEffectProperty('shadow', 'offsetY', Number(($event.target as HTMLInputElement).value))"
                  />
                </label>
              </div>
            </div>
          </div>

          <!-- Glow -->
          <div class="effect-group">
            <label class="checkbox-label">
              <input
                type="checkbox"
                :checked="getGlowValue('enabled', false)"
                @change="updateEffectProperty('glow', 'enabled', ($event.target as HTMLInputElement).checked)"
              />
              <span>{{ t('properties.glowLabel') }}</span>
            </label>

            <div v-if="getGlowValue('enabled', false)" class="effect-details">
              <div class="property-row">
                <label>
                  <span>{{ t('properties.colorLabel') }}</span>
                  <input
                    type="color"
                    :value="getGlowValue('color', '#00ff00')"
                    @input="updateEffectProperty('glow', 'color', ($event.target as HTMLInputElement).value)"
                  />
                </label>
                <label>
                  <span>{{ t('properties.intensity') }}</span>
                  <input
                    type="number"
                    :value="getGlowValue('intensity', 10)"
                    @input="updateEffectProperty('glow', 'intensity', Number(($event.target as HTMLInputElement).value))"
                    min="0"
                    max="50"
                  />
                </label>
              </div>
            </div>
          </div>
        </div>
      </template>

      <!-- Progress Bar Section -->
      <div :class="['property-section', 'progress-section', { 'progress-selected': uiStore.isProgressBarSelected }]">
        <div class="section-title">📊 {{ t('properties.progressBar') }}</div>
        <div class="property-grid-4">
          <label>
            <span>X</span>
            <input
              type="number"
              :value="projectStore.project.config.progressBar.x"
              @input="updateProgressBar('x', Number(($event.target as HTMLInputElement).value))"
            />
          </label>
          <label>
            <span>Y</span>
            <input
              type="number"
              :value="projectStore.project.config.progressBar.y"
              @input="updateProgressBar('y', Number(($event.target as HTMLInputElement).value))"
            />
          </label>
          <label>
            <span>W</span>
            <input
              type="number"
              :value="projectStore.project.config.progressBar.width"
              @input="updateProgressBar('width', Number(($event.target as HTMLInputElement).value))"
            />
          </label>
          <label>
            <span>H</span>
            <input
              type="number"
              :value="projectStore.project.config.progressBar.height"
              @input="updateProgressBar('height', Number(($event.target as HTMLInputElement).value))"
            />
          </label>
        </div>
        <div class="property-row">
          <label>
            <span>{{ t('properties.backgroundLabel') }}</span>
            <input
              type="color"
              :value="projectStore.project.config.progressBar.backgroundColor"
              @input="updateProgressBar('backgroundColor', ($event.target as HTMLInputElement).value)"
            />
          </label>
          <label>
            <span>{{ t('properties.progressFill') }}</span>
            <input
              type="color"
              :value="projectStore.project.config.progressBar.fillColor"
              @input="updateProgressBar('fillColor', ($event.target as HTMLInputElement).value)"
            />
          </label>
          <label>
            <span>{{ t('properties.borderLabel') }}</span>
            <input
              type="color"
              :value="projectStore.project.config.progressBar.borderColor"
              @input="updateProgressBar('borderColor', ($event.target as HTMLInputElement).value)"
            />
          </label>
        </div>
      </div>

      <!-- Video Button Section -->
      <div :class="['property-section', 'video-section', { 'video-selected': uiStore.isVideoButtonSelected }]">
        <div class="section-title">🎬 {{ t('properties.videoBackground') }}</div>

        <!-- Enable Video -->
        <div class="property-row">
          <label class="checkbox-label full-width">
            <input
              type="checkbox"
              :checked="videoConfig?.enabled || false"
              @change="updateVideoConfig('enabled', ($event.target as HTMLInputElement).checked)"
            />
            <span>{{ t('properties.enableVideoBackground') }}</span>
          </label>
        </div>

        <!-- Video Path -->
        <div class="property-row">
          <label class="full-width">
            <span>{{ t('properties.videoFile') }}</span>
            <div class="file-input-row">
              <input
                type="text"
                :value="videoConfig?.path || ''"
                readonly
                class="file-path"
                :placeholder="t('properties.noVideoSelected')"
              />
              <button @click="selectVideoFile" class="select-btn">📁</button>
            </div>
          </label>
        </div>

        <!-- Video Options -->
        <div class="property-row">
          <label class="checkbox-label">
            <input
              type="checkbox"
              :checked="videoConfig?.showControls !== false"
              @change="updateVideoConfig('showControls', ($event.target as HTMLInputElement).checked)"
            />
            <span>{{ t('properties.showControls') }}</span>
          </label>
          <label class="checkbox-label">
            <input
              type="checkbox"
              :checked="videoConfig?.autoplay !== false"
              @change="updateVideoConfig('autoplay', ($event.target as HTMLInputElement).checked)"
            />
            <span>{{ t('properties.autoPlay') }}</span>
          </label>
          <label class="checkbox-label">
            <input
              type="checkbox"
              :checked="videoConfig?.loop !== false"
              @change="updateVideoConfig('loop', ($event.target as HTMLInputElement).checked)"
            />
            <span>{{ t('properties.loop') }}</span>
          </label>
        </div>

        <!-- Control Button Properties (only shown when video enabled and controls shown) -->
        <template v-if="videoConfig?.enabled && videoConfig?.showControls">
          <div class="subsection-title">▶ {{ t('properties.playPauseButton') }}</div>

          <!-- Position -->
          <div class="property-grid-4">
            <label>
              <span>X</span>
              <input
                type="number"
                :value="videoBtnConfig.x"
                @input="updateVideoButtonConfig('x', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
            <label>
              <span>Y</span>
              <input
                type="number"
                :value="videoBtnConfig.y"
                @input="updateVideoButtonConfig('y', Number(($event.target as HTMLInputElement).value))"
              />
            </label>
            <label>
              <span>{{ t('properties.sizeLabel') }}</span>
              <input
                type="number"
                :value="videoBtnConfig.size"
                @input="updateVideoButtonConfig('size', Number(($event.target as HTMLInputElement).value))"
                min="16"
                max="100"
              />
            </label>
            <label>
              <span>{{ t('properties.opacityLabel') }}</span>
              <input
                type="number"
                :value="(videoBtnConfig.opacity || 1) * 100"
                @input="updateVideoButtonConfig('opacity', Number(($event.target as HTMLInputElement).value) / 100)"
                min="0"
                max="100"
                step="5"
              />
            </label>
          </div>

          <!-- Colors -->
          <div class="property-row">
            <label>
              <span>{{ t('properties.backgroundLabel') }}</span>
              <input
                type="color"
                :value="videoBtnConfig.backgroundColor"
                @input="updateVideoButtonConfig('backgroundColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
            <label>
              <span>{{ t('settings.iconColor') }}</span>
              <input
                type="color"
                :value="videoBtnConfig.iconColor"
                @input="updateVideoButtonConfig('iconColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
            <label>
              <span>{{ t('properties.borderLabel') }}</span>
              <input
                type="color"
                :value="videoBtnConfig.borderColor"
                @input="updateVideoButtonConfig('borderColor', ($event.target as HTMLInputElement).value)"
              />
            </label>
          </div>

          <!-- Border Width -->
          <div class="property-row">
            <label>
              <span>{{ t('properties.borderWidth') }}</span>
              <input
                type="number"
                :value="videoBtnConfig.borderWidth"
                @input="updateVideoButtonConfig('borderWidth', Number(($event.target as HTMLInputElement).value))"
                min="0"
                max="10"
              />
            </label>
          </div>
        </template>
      </div>
    </div>

    <!-- Layers Section -->
    <div class="section layers-section" :class="{ collapsed: !isLayersExpanded }">
      <div class="layers-header" @click="isLayersExpanded = !isLayersExpanded">
        <span class="collapse-icon">{{ isLayersExpanded ? '▼' : '▶' }}</span>
        <h3>{{ t('layers.title') }}</h3>
        <span class="layer-count">{{ sortedElements.length }}</span>
      </div>
      <div v-show="isLayersExpanded" class="layers-list">
        <div
          v-for="layerEl in sortedElements"
          :key="layerEl.id"
          class="layer-item"
          :class="{
            selected: projectStore.selectedElement?.id === layerEl.id,
            hidden: layerEl.visible === false,
            locked: layerEl.locked
          }"
          @click="selectLayerElement(layerEl.id)"
        >
          <span class="layer-icon">{{ getTypeIcon(layerEl.type) }}</span>
          <span class="layer-name">{{ getLayerElementName(layerEl) }}</span>
          <span class="layer-zindex">z:{{ layerEl.zIndex || 0 }}</span>
          <div class="layer-actions">
            <button
              class="layer-btn"
              :title="t('layers.moveUp')"
              @click.stop="moveLayerUp(layerEl.id)"
            >
              ▲
            </button>
            <button
              class="layer-btn"
              :title="t('layers.moveDown')"
              @click.stop="moveLayerDown(layerEl.id)"
            >
              ▼
            </button>
            <button
              class="layer-btn"
              :class="{ active: layerEl.visible !== false }"
              :title="layerEl.visible === false ? t('layers.show') : t('layers.hide')"
              @click.stop="toggleVisibility(layerEl.id)"
            >
              {{ layerEl.visible === false ? '👁️‍🗨️' : '👁️' }}
            </button>
            <button
              class="layer-btn"
              :class="{ active: layerEl.locked }"
              :title="layerEl.locked ? t('layers.unlock') : t('layers.lock')"
              @click.stop="toggleLock(layerEl.id)"
            >
              {{ layerEl.locked ? '🔒' : '🔓' }}
            </button>
            <button
              class="layer-btn delete"
              :title="t('layers.delete')"
              @click.stop="deleteLayerElement(layerEl.id)"
            >
              🗑️
            </button>
          </div>
        </div>
        <div v-if="sortedElements.length === 0" class="no-layers">
          {{ t('layers.noElements') }}
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.property-panel {
  width: 300px;
  background-color: #252526;
  border-left: 1px solid #3e3e42;
  display: flex;
  flex-direction: column;
  flex-shrink: 0;
}

.panel-header {
  padding: 12px 16px;
  border-bottom: 1px solid #3e3e42;
  background-color: #2d2d30;
}

.panel-header h3 {
  margin: 0;
  font-size: 12px;
  font-weight: 600;
  color: #cccccc;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.panel-content {
  flex: 1;
  overflow-y: auto;
  padding: 12px;
}

.no-selection {
  text-align: center;
  padding: 40px 20px;
  color: #6e6e6e;
  font-size: 13px;
}

.empty-icon {
  color: #3e3e42;
  margin-bottom: 16px;
}

.property-section {
  margin-bottom: 16px;
  padding-bottom: 16px;
  border-bottom: 1px solid #3e3e42;
}

.property-section:last-child {
  border-bottom: none;
}

.element-header {
  display: flex;
  align-items: center;
  gap: 8px;
}

.element-name {
  flex: 1;
  font-size: 14px !important;
  font-weight: 500;
}

.type-badge {
  display: inline-block;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 600;
  text-transform: uppercase;
  white-space: nowrap;
}

.type-badge.button { background-color: #0078d4; color: white; }
.type-badge.label { background-color: #6e6e6e; color: white; }
.type-badge.status { background-color: #00c853; color: #1e1e1e; }
.type-badge.percentage { background-color: #ffc107; color: #1e1e1e; }
.type-badge.box { background-color: #9c27b0; color: white; }
.type-badge.image { background-color: #ff5722; color: white; }

.section-title {
  font-size: 11px;
  font-weight: 600;
  color: #9d9d9d;
  margin-bottom: 10px;
  text-transform: uppercase;
  letter-spacing: 0.3px;
}

.property-row {
  display: flex;
  gap: 8px;
  margin-bottom: 8px;
}

.property-row:last-child {
  margin-bottom: 0;
}

.property-grid-4 {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 8px;
  margin-bottom: 8px;
}

.property-row label,
.property-grid-4 label {
  display: flex;
  flex-direction: column;
  gap: 4px;
  font-size: 11px;
  color: #9d9d9d;
}

.property-row label.flex-1 { flex: 1; }
.property-row label.flex-2 { flex: 2; }
.property-row label.full-width { flex: 1; width: 100%; }

.checkbox-label {
  flex-direction: row !important;
  align-items: center;
  gap: 6px !important;
}

.checkbox-label input[type="checkbox"] {
  width: auto;
}

input, select {
  background-color: #3c3c3c;
  border: 1px solid #555;
  border-radius: 4px;
  padding: 6px 8px;
  color: #cccccc;
  font-size: 12px;
}

input:focus, select:focus {
  outline: none;
  border-color: #0078d4;
}

input[type="color"] {
  padding: 2px;
  height: 28px;
  cursor: pointer;
}

input[type="range"] {
  padding: 0;
  height: 20px;
  cursor: pointer;
}

input[type="number"] {
  width: 100%;
}

select {
  width: 100%;
  cursor: pointer;
}

.full-width {
  width: 100%;
}

.mt-8 {
  margin-top: 8px;
}

/* State tabs */
.state-tabs {
  display: flex;
  gap: 4px;
  margin-bottom: 12px;
}

.state-tab {
  flex: 1;
  padding: 6px 4px;
  font-size: 10px;
  background-color: #3c3c3c;
  border: 1px solid #555;
  border-radius: 4px;
  color: #9d9d9d;
  cursor: pointer;
  transition: all 0.2s;
}

.state-tab:hover {
  background-color: #4a4a4a;
}

.state-tab.active {
  background-color: #0078d4;
  border-color: #0078d4;
  color: white;
}

.state-properties {
  background-color: #2d2d30;
  padding: 12px;
  border-radius: 6px;
}

/* File input */
.file-input {
  display: flex;
  gap: 4px;
}

.file-input input {
  flex: 1;
}

.file-input button {
  padding: 6px 10px;
  background-color: #0078d4;
  border: none;
  border-radius: 4px;
  color: white;
  cursor: pointer;
}

.file-input button:hover {
  background-color: #1084d8;
}

/* Effect groups */
.effect-group {
  background-color: #2d2d30;
  padding: 10px;
  border-radius: 6px;
  margin-top: 8px;
}

.effect-details {
  margin-top: 10px;
  padding-top: 10px;
  border-top: 1px solid #3e3e42;
}

/* WebView section */
.webview-section {
  border-color: #2196f3;
}

.webview-section .section-title {
  color: #2196f3;
}

.info-box {
  background-color: rgba(33, 150, 243, 0.1);
  border: 1px solid rgba(33, 150, 243, 0.3);
  border-radius: 6px;
  padding: 10px;
  font-size: 11px;
  color: #90caf9;
  margin: 8px 0;
}

/* Progress section */
.progress-section {
  border-top: 2px solid #00c853;
  padding-top: 16px;
  margin-top: 16px;
  background-color: rgba(0, 200, 83, 0.05);
  border-radius: 6px;
  padding: 12px;
  margin-left: -12px;
  margin-right: -12px;
}

.progress-section.progress-selected {
  background-color: rgba(0, 200, 83, 0.15);
  border: 2px solid #00c853;
}

.progress-section .section-title {
  color: #00c853;
}

/* Layers section */
.layers-section {
  border-top: 2px solid #ff9800;
  background-color: rgba(255, 152, 0, 0.05);
  border-radius: 6px;
  padding: 12px;
  margin: 0;
  flex-shrink: 0;
  max-height: 50%;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.layers-section.collapsed {
  max-height: none;
  flex-shrink: 0;
}

.layers-header {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  padding: 4px 0;
  user-select: none;
  transition: opacity 0.15s;
}

.layers-header:hover {
  opacity: 0.8;
}

.collapse-icon {
  font-size: 10px;
  color: #ff9800;
  width: 12px;
}

.layers-section h3 {
  color: #ff9800;
  margin: 0;
  font-size: 14px;
  font-weight: 600;
  flex: 1;
}

.layer-count {
  font-size: 11px;
  color: #ff9800;
  background: rgba(255, 152, 0, 0.2);
  padding: 2px 8px;
  border-radius: 10px;
}

.layers-list {
  display: flex;
  flex-direction: column;
  gap: 4px;
  overflow-y: auto;
  flex: 1;
  min-height: 0;
  margin-top: 12px;
}

.layer-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px;
  background: #2a2a2a;
  border-radius: 4px;
  cursor: pointer;
  transition: all 0.15s ease;
  border: 1px solid transparent;
}

.layer-item:hover {
  background: #333;
}

.layer-item.selected {
  background: #3a3a3a;
  border-color: #ff9800;
}

.layer-item.hidden {
  opacity: 0.5;
}

.layer-item.locked {
  background: #2a2a30;
}

.layer-icon {
  font-size: 14px;
  width: 20px;
  text-align: center;
}

.layer-name {
  flex: 1;
  font-size: 12px;
  color: #e0e0e0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.layer-zindex {
  font-size: 10px;
  color: #888;
  padding: 2px 4px;
  background: #222;
  border-radius: 3px;
}

.layer-actions {
  display: flex;
  gap: 2px;
}

.layer-btn {
  background: transparent;
  border: none;
  padding: 4px;
  cursor: pointer;
  font-size: 10px;
  border-radius: 3px;
  transition: background 0.15s;
  opacity: 0.6;
}

.layer-btn:hover {
  background: #444;
  opacity: 1;
}

.layer-btn.active {
  opacity: 1;
}

.layer-btn.delete:hover {
  background: #ff4444;
}

.no-layers {
  text-align: center;
  color: #666;
  font-size: 12px;
  padding: 20px;
}

/* Video Section */
.video-section {
  border: 1px solid transparent;
  transition: all 0.2s ease;
}

.video-section.video-selected {
  border-color: #9c27b0;
  background: rgba(156, 39, 176, 0.1);
}

.subsection-title {
  font-size: 11px;
  color: #9c27b0;
  margin: 12px 0 8px 0;
  padding-bottom: 4px;
  border-bottom: 1px solid #333;
}

.file-input-row {
  display: flex;
  gap: 4px;
  align-items: stretch;
}

.file-input-row .file-path {
  flex: 1;
  background: #1e1e1e;
  border: 1px solid #444;
  border-radius: 4px;
  padding: 6px 8px;
  font-size: 11px;
  color: #aaa;
  overflow: hidden;
  text-overflow: ellipsis;
}

.file-input-row .select-btn {
  background: #333;
  border: 1px solid #555;
  border-radius: 4px;
  padding: 4px 8px;
  cursor: pointer;
  transition: all 0.15s;
}

.file-input-row .select-btn:hover {
  background: #444;
  border-color: #666;
}
</style>
