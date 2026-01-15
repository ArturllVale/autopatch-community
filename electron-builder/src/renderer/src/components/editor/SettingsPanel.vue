<script setup lang="ts">
import { ref, watch, onMounted } from 'vue'
import { useI18n } from 'vue-i18n'
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'

const { t } = useI18n()
const projectStore = useProjectStore()
const uiStore = useUiStore()

// Local refs for form inputs
const serverName = ref(projectStore.project.config.serverName)
const patchListUrl = ref(projectStore.project.config.patchListUrl)
const newsUrl = ref(projectStore.project.config.newsUrl || '')
const clientExe = ref(projectStore.project.config.clientExe)
const clientArgs = ref(projectStore.project.config.clientArgs || '')
const grfFiles = ref(projectStore.project.config.grfFiles.join('\n'))
const windowWidth = ref(projectStore.project.config.windowWidth)
const windowHeight = ref(projectStore.project.config.windowHeight)
const iconPath = ref(projectStore.project.config.iconPath || '')

// Video background settings
const videoEnabled = ref(false)
const videoPath = ref('')
const videoLoop = ref(true)
const videoAutoplay = ref(true)
const videoMuted = ref(true)
const videoShowControls = ref(false)

// Video control button style
const videoBtnX = ref(740)
const videoBtnY = ref(550)
const videoBtnSize = ref(50)
const videoBtnBgColor = ref('#000000')
const videoBtnIconColor = ref('#ffffff')
const videoBtnBorderColor = ref('#ffffff')
const videoBtnBorderWidth = ref(2)
const videoBtnOpacity = ref(50)

// Load video settings from store
function loadVideoSettings() {
  const vb = projectStore.project.config.videoBackground
  videoEnabled.value = vb?.enabled || false
  videoPath.value = vb?.path || ''
  videoLoop.value = vb?.loop ?? true
  videoAutoplay.value = vb?.autoplay ?? true
  videoMuted.value = vb?.muted ?? true
  videoShowControls.value = vb?.showControls ?? false

  // Control button style
  const btn = vb?.controlButton
  videoBtnX.value = btn?.x ?? 740
  videoBtnY.value = btn?.y ?? 550
  videoBtnSize.value = btn?.size ?? 50
  videoBtnBgColor.value = btn?.backgroundColor ?? '#000000'
  videoBtnIconColor.value = btn?.iconColor ?? '#ffffff'
  videoBtnBorderColor.value = btn?.borderColor ?? '#ffffff'
  videoBtnBorderWidth.value = btn?.borderWidth ?? 2
  videoBtnOpacity.value = btn?.opacity ?? 50
}

// Load on mount and watch for store changes
onMounted(() => {
  loadVideoSettings()
})

// Watch store changes to reload video settings when navigating back
watch(() => projectStore.project.config.videoBackground, () => {
  loadVideoSettings()
}, { deep: true })

function saveSettings() {
  projectStore.updateConfig({
    serverName: serverName.value,
    patchListUrl: patchListUrl.value,
    newsUrl: newsUrl.value || undefined,
    clientExe: clientExe.value,
    clientArgs: clientArgs.value || undefined,
    grfFiles: grfFiles.value.split('\n').filter(f => f.trim()),
    windowWidth: windowWidth.value,
    windowHeight: windowHeight.value,
    iconPath: iconPath.value || undefined,
    videoBackground: {
      enabled: videoEnabled.value,
      path: videoPath.value,
      loop: videoLoop.value,
      autoplay: videoAutoplay.value,
      muted: videoMuted.value,
      showControls: videoShowControls.value,
      controlButton: {
        x: videoBtnX.value,
        y: videoBtnY.value,
        size: videoBtnSize.value,
        backgroundColor: videoBtnBgColor.value,
        iconColor: videoBtnIconColor.value,
        borderColor: videoBtnBorderColor.value,
        borderWidth: videoBtnBorderWidth.value,
        opacity: videoBtnOpacity.value
      }
    }
  })
  uiStore.setStatus(t('status.settingsSaved'))
}

async function selectClientExe() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('settings.selectClientExe'),
      filters: [{ name: t('settings.executables'), extensions: ['exe'] }]
    })
    if (result) {
      clientExe.value = result.split(/[/\\]/).pop() || result
    }
  } catch (e) {
    console.error(e)
  }
}

async function selectIconFile() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('settings.selectIcon'),
      filters: [{ name: t('settings.icons'), extensions: ['ico'] }]
    })
    if (result) {
      iconPath.value = result
    }
  } catch (e) {
    console.error(e)
  }
}

async function selectVideoFile() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('settings.selectVideo'),
      filters: [{ name: t('settings.videos'), extensions: ['mp4', 'wmv', 'avi', 'webm'] }]
    })
    if (result) {
      videoPath.value = result
    }
  } catch (e) {
    console.error(e)
  }
}

async function handleExport() {
  console.log('[EXPORT] handleExport called')
  saveSettings()

  // Debug: log what videoBackground is after save
  console.log('[EXPORT] videoBackground after save:', JSON.stringify(projectStore.project.config.videoBackground))
  console.log('[EXPORT] videoEnabled:', videoEnabled.value)
  console.log('[EXPORT] videoPath:', videoPath.value)

  try {
    // Teste de ping primeiro
    console.log('[EXPORT] Testing ping...')
    const pingResult = await window.electron.ipcRenderer.invoke('test:ping')
    console.log('[EXPORT] Ping result:', pingResult)

    // Apenas pedir o caminho de saída do EXE
    console.log('[EXPORT] Opening save dialog...')
    const outputPath = await window.electron.ipcRenderer.invoke('dialog:save-file', {
      title: t('settings.savePatcherAs'),
      filters: [{ name: t('settings.executables'), extensions: ['exe'] }],
      defaultPath: `${serverName.value.replace(/\s+/g, '_')}_Patcher.exe`
    })

    console.log('[EXPORT] Output path:', outputPath)
    if (!outputPath) {
      console.log('[EXPORT] User cancelled')
      return
    }

    uiStore.startBuild()
    uiStore.setStatus(t('status.preparingImages'))

    // Preparar configuração completa para o patcher
    // Use JSON.parse/stringify to create a plain object (remove Vue reactivity)
    const patcherConfig = JSON.parse(JSON.stringify(projectStore.project.config))

    // Converter imagem de fundo para base64
    if (patcherConfig.backgroundImagePath) {
      try {
        console.log('[EXPORT] Converting background image to base64...')
        const bgBase64 = await window.electron.ipcRenderer.invoke('file:read-binary', patcherConfig.backgroundImagePath)
        patcherConfig.backgroundImage = bgBase64
        console.log('[EXPORT] Background image converted, length:', bgBase64.length)
      } catch (e) {
        console.error('[EXPORT] Failed to convert background:', e)
      }
    }

    // Converter imagens dos elementos para base64
    for (const element of patcherConfig.elements) {
      if (element.type === 'image' && element.backgroundImage) {
        try {
          console.log('[EXPORT] Converting element image to base64:', element.id)
          const imgBase64 = await window.electron.ipcRenderer.invoke('file:read-binary', element.backgroundImage)
          element.backgroundImage = imgBase64
          console.log('[EXPORT] Element image converted, length:', imgBase64.length)
        } catch (e) {
          console.error('[EXPORT] Failed to convert element image:', element.id, e)
        }
      }

      // Converter imagens de estados de botões
      if (element.type === 'button' && element.states) {
        for (const stateName of ['normal', 'hover', 'pressed', 'disabled']) {
          const state = element.states[stateName]
          if (state?.imagePath && !state.imagePath.startsWith('data:')) {
            try {
              console.log('[EXPORT] Converting button state image:', element.id, stateName)
              const stateBase64 = await window.electron.ipcRenderer.invoke('file:read-binary', state.imagePath)
              state.imagePath = stateBase64
            } catch (e) {
              console.error('[EXPORT] Failed to convert button state image:', e)
            }
          }
        }
      }
    }

    console.log('[EXPORT] Config prepared:', JSON.stringify(patcherConfig).substring(0, 200))

    uiStore.setStatus(t('status.generatingPatcher'))

    // Get background and icon paths as plain strings
    const bgPath = projectStore.project.config.backgroundImagePath || ''
    const icoPath = projectStore.project.config.iconPath || ''

    // Call the backend to generate the EXE
    console.log('[EXPORT] Calling build:generate-exe...')
    const result = await window.electron.ipcRenderer.invoke('build:generate-exe', {
      config: patcherConfig,
      outputPath: outputPath,
      backgroundImagePath: bgPath,
      iconPath: icoPath
    })
    console.log('[EXPORT] Result:', result)

    if (result.success) {
      uiStore.setStatus(t('status.patcherSuccess', { path: outputPath }))
      // Mostrar mensagem de sucesso
      await window.electron.ipcRenderer.invoke('dialog:show-message', {
        type: 'info',
        title: t('common.success'),
        message: t('generate.success'),
        detail: outputPath
      })
    } else {
      uiStore.setStatus(t('status.error', { message: result.error }))
      await window.electron.ipcRenderer.invoke('dialog:show-message', {
        type: 'error',
        title: t('common.error'),
        message: t('generate.error'),
        detail: result.error
      })
    }
  } catch (e: any) {
    uiStore.setStatus(`Erro: ${e.message}`)
  } finally {
    uiStore.endBuild()
  }
}
</script>

<template>
  <div class="settings-panel">
    <!-- Header with back button -->
    <div class="settings-header">
      <button class="back-btn" @click="uiStore.setActiveTab('design')">
        <svg viewBox="0 0 24 24" fill="currentColor">
          <path d="M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z"/>
        </svg>
        Voltar ao Editor
      </button>
    </div>

    <div class="settings-content">
      <h2>{{ t('settings.projectSettings') }}</h2>

      <!-- Server Settings -->
      <div class="settings-section">
        <h3>{{ t('settings.server') }}</h3>
        <div class="form-group">
          <label>{{ t('settings.serverName') }}</label>
          <input type="text" v-model="serverName" :placeholder="t('settings.serverNamePlaceholder')" />
        </div>
        <div class="form-group">
          <label>{{ t('settings.patchlistUrl') }}</label>
          <input type="url" v-model="patchListUrl" :placeholder="t('settings.patchlistUrlPlaceholder')" />
        </div>
        <div class="form-group">
          <label>{{ t('settings.newsUrl') }}</label>
          <input type="url" v-model="newsUrl" :placeholder="t('settings.newsUrlPlaceholder')" />
        </div>
      </div>

      <!-- Client Settings -->
      <div class="settings-section">
        <h3>{{ t('settings.client') }}</h3>
        <div class="form-group">
          <label>{{ t('settings.clientExe') }}</label>
          <div class="input-with-button">
            <input type="text" v-model="clientExe" :placeholder="t('settings.clientExePlaceholder')" />
            <button @click="selectClientExe">{{ t('common.browse') }}</button>
          </div>
        </div>
        <div class="form-group">
          <label>{{ t('settings.clientArgs') }}</label>
          <input type="text" v-model="clientArgs" :placeholder="t('settings.clientArgsPlaceholder')" />
        </div>
        <div class="form-group">
          <label>{{ t('settings.grfFiles') }}</label>
          <textarea v-model="grfFiles" rows="4" :placeholder="t('settings.grfFilesPlaceholder')"></textarea>
        </div>
      </div>

      <!-- Window Settings -->
      <div class="settings-section">
        <h3>{{ t('settings.window') }}</h3>
        <div class="form-row">
          <div class="form-group">
            <label>{{ t('settings.width') }}</label>
            <input type="number" v-model.number="windowWidth" min="400" max="1920" />
          </div>
          <div class="form-group">
            <label>{{ t('settings.height') }}</label>
            <input type="number" v-model.number="windowHeight" min="300" max="1080" />
          </div>
        </div>
        <div class="form-group">
          <label>{{ t('settings.patcherIcon') }}</label>
          <div class="input-with-button">
            <input type="text" v-model="iconPath" :placeholder="t('settings.noIconSelected')" readonly />
            <button @click="selectIconFile">{{ t('common.browse') }}</button>
          </div>
          <small class="help-text">{{ t('settings.useDefaultIcon') }}</small>
        </div>
      </div>

      <!-- Video Background -->
      <div class="settings-section">
        <h3>🎬 {{ t('settings.videoBackground') }}</h3>
        <div class="form-group">
          <label class="checkbox-label">
            <input type="checkbox" v-model="videoEnabled" />
            <span>{{ t('settings.enableVideo') }}</span>
          </label>
          <small class="help-text">{{ t('settings.videoHelp') }}</small>
        </div>

        <template v-if="videoEnabled">
          <div class="form-group">
            <label>{{ t('settings.videoFile') }}</label>
            <div class="input-with-button">
              <input type="text" v-model="videoPath" :placeholder="t('settings.noVideoSelected')" readonly />
              <button @click="selectVideoFile">{{ t('common.browse') }}</button>
            </div>
            <small class="help-text">{{ t('settings.videoFormats') }}</small>
          </div>

          <div class="form-row">
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoLoop" />
                <span>{{ t('settings.videoLoop') }}</span>
              </label>
            </div>
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoAutoplay" />
                <span>{{ t('settings.videoAutoplay') }}</span>
              </label>
            </div>
          </div>

          <div class="form-row">
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoMuted" />
                <span>{{ t('settings.videoMuted') }}</span>
              </label>
            </div>
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoShowControls" />
                <span>{{ t('settings.videoShowControls') }}</span>
              </label>
            </div>
          </div>

          <!-- Control Button Style -->
          <template v-if="videoShowControls">
            <h4 class="subsection-title">{{ t('settings.controlButtonStyle') }}</h4>

            <div class="form-row">
              <div class="form-group">
                <label>{{ t('settings.positionX') }}</label>
                <input type="number" v-model.number="videoBtnX" min="0" />
              </div>
              <div class="form-group">
                <label>{{ t('settings.positionY') }}</label>
                <input type="number" v-model.number="videoBtnY" min="0" />
              </div>
              <div class="form-group">
                <label>{{ t('settings.size') }}</label>
                <input type="number" v-model.number="videoBtnSize" min="20" max="200" />
              </div>
            </div>

            <div class="form-row">
              <div class="form-group">
                <label>{{ t('settings.backgroundColor') }}</label>
                <div class="color-input">
                  <input type="color" v-model="videoBtnBgColor" />
                  <input type="text" v-model="videoBtnBgColor" />
                </div>
              </div>
              <div class="form-group">
                <label>{{ t('settings.iconColor') }}</label>
                <div class="color-input">
                  <input type="color" v-model="videoBtnIconColor" />
                  <input type="text" v-model="videoBtnIconColor" />
                </div>
              </div>
            </div>

            <div class="form-row">
              <div class="form-group">
                <label>{{ t('settings.borderColor') }}</label>
                <div class="color-input">
                  <input type="color" v-model="videoBtnBorderColor" />
                  <input type="text" v-model="videoBtnBorderColor" />
                </div>
              </div>
              <div class="form-group">
                <label>{{ t('settings.borderWidth') }}</label>
                <input type="number" v-model.number="videoBtnBorderWidth" min="0" max="10" />
              </div>
              <div class="form-group">
                <label>{{ t('settings.opacityPercent') }}</label>
                <input type="number" v-model.number="videoBtnOpacity" min="0" max="100" />
              </div>
            </div>
          </template>
        </template>
      </div>

      <!-- Actions -->
      <div class="settings-actions">
        <button class="btn-secondary" @click="saveSettings">
          {{ t('settings.saveSettings') }}
        </button>
        <button class="btn-primary" @click="handleExport" :disabled="uiStore.isBuilding">
          <span v-if="uiStore.isBuilding">{{ t('settings.generating') }}</span>
          <span v-else>{{ t('settings.exportPatcher') }}</span>
        </button>
      </div>
    </div>
  </div>
</template>

<style scoped>
.settings-panel {
  flex: 1;
  overflow-y: auto;
  background-color: #1e1e1e;
  display: flex;
  flex-direction: column;
}

.settings-header {
  padding: 12px 16px;
  background-color: #252526;
  border-bottom: 1px solid #3e3e42;
  flex-shrink: 0;
}

.back-btn {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  background-color: #2d2d30;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #cccccc;
  font-size: 13px;
  cursor: pointer;
  transition: all 0.15s;
}

.back-btn:hover {
  background-color: #3e3e42;
  color: #ffffff;
  border-color: #0078d4;
}

.back-btn svg {
  width: 16px;
  height: 16px;
}

.settings-content {
  max-width: 600px;
  margin: 0 auto;
  padding: 40px;
}

h2 {
  font-size: 24px;
  font-weight: 300;
  color: #ffffff;
  margin-bottom: 30px;
}

.settings-section {
  background-color: #252526;
  border: 1px solid #3e3e42;
  border-radius: 8px;
  padding: 20px;
  margin-bottom: 20px;
}

.settings-section h3 {
  font-size: 14px;
  font-weight: 600;
  color: #cccccc;
  margin-bottom: 16px;
  padding-bottom: 8px;
  border-bottom: 1px solid #3e3e42;
}

.form-group {
  margin-bottom: 16px;
}

.form-group:last-child {
  margin-bottom: 0;
}

.form-group label {
  display: block;
  font-size: 12px;
  color: #9d9d9d;
  margin-bottom: 6px;
}

.form-group input,
.form-group textarea {
  width: 100%;
  background-color: #3c3c3c;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  padding: 10px 12px;
  color: #cccccc;
  font-size: 14px;
}

.form-group input:focus,
.form-group textarea:focus {
  border-color: #0078d4;
  outline: none;
}

.form-group textarea {
  resize: vertical;
  font-family: 'Consolas', monospace;
}

.form-row {
  display: flex;
  gap: 16px;
}

.form-row .form-group {
  flex: 1;
}

.input-with-button {
  display: flex;
  gap: 8px;
}

.input-with-button input {
  flex: 1;
}

.input-with-button button {
  padding: 10px 16px;
  background-color: #3e3e42;
  border: none;
  border-radius: 4px;
  color: #cccccc;
  cursor: pointer;
  white-space: nowrap;
}

.input-with-button button:hover {
  background-color: #4e4e52;
}

.help-text {
  display: block;
  font-size: 11px;
  color: #6e6e6e;
  margin-top: 4px;
}

.settings-actions {
  display: flex;
  gap: 12px;
  justify-content: flex-end;
  margin-top: 30px;
}

.btn-primary,
.btn-secondary {
  padding: 12px 24px;
  border-radius: 4px;
  font-size: 14px;
  cursor: pointer;
  transition: all 0.2s;
}

.btn-primary {
  background-color: #0e639c;
  border: none;
  color: white;
}

.btn-primary:hover:not(:disabled) {
  background-color: #1177bb;
}

.btn-primary:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.btn-secondary {
  background-color: transparent;
  border: 1px solid #3e3e42;
  color: #cccccc;
}

.btn-secondary:hover {
  border-color: #6e6e6e;
  background-color: #2a2d2e;
}

/* Checkbox styling */
.checkbox-label {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  font-size: 13px;
  color: #cccccc;
}

.checkbox-label input[type="checkbox"] {
  width: 16px;
  height: 16px;
  accent-color: #0e639c;
  cursor: pointer;
}

.checkbox-label span {
  user-select: none;
}

/* Subsection title */
.subsection-title {
  font-size: 12px;
  font-weight: 600;
  color: #9d9d9d;
  margin: 16px 0 12px 0;
  padding-top: 12px;
  border-top: 1px solid #3e3e42;
}

/* Color input */
.color-input {
  display: flex;
  gap: 8px;
  align-items: center;
}

.color-input input[type="color"] {
  width: 40px;
  height: 32px;
  padding: 2px;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  background-color: #3c3c3c;
  cursor: pointer;
}

.color-input input[type="text"] {
  flex: 1;
  width: auto;
}
</style>
