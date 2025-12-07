<script setup lang="ts">
import { ref, watch, onMounted } from 'vue'
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'

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
  uiStore.setStatus('Configurações salvas')
}

async function selectClientExe() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: 'Selecionar Executável do Cliente',
      filters: [{ name: 'Executáveis', extensions: ['exe'] }]
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
      title: 'Selecionar Ícone do Patcher',
      filters: [{ name: 'Ícones', extensions: ['ico'] }]
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
      title: 'Selecionar Vídeo de Fundo',
      filters: [{ name: 'Vídeos', extensions: ['mp4', 'wmv', 'avi', 'webm'] }]
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
      title: 'Salvar Patcher Como',
      filters: [{ name: 'Executáveis', extensions: ['exe'] }],
      defaultPath: `${serverName.value.replace(/\s+/g, '_')}_Patcher.exe`
    })

    console.log('[EXPORT] Output path:', outputPath)
    if (!outputPath) {
      console.log('[EXPORT] User cancelled')
      return
    }

    uiStore.startBuild()
    uiStore.setStatus('Preparando imagens...')

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

    uiStore.setStatus('Gerando patcher...')

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
      uiStore.setStatus(`Patcher gerado com sucesso: ${outputPath}`)
      // Mostrar mensagem de sucesso
      await window.electron.ipcRenderer.invoke('dialog:show-message', {
        type: 'info',
        title: 'Sucesso',
        message: 'Patcher gerado com sucesso!',
        detail: outputPath
      })
    } else {
      uiStore.setStatus(`Erro: ${result.error}`)
      await window.electron.ipcRenderer.invoke('dialog:show-message', {
        type: 'error',
        title: 'Erro',
        message: 'Falha ao gerar o patcher',
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
    <div class="settings-content">
      <h2>Configurações do Projeto</h2>

      <!-- Server Settings -->
      <div class="settings-section">
        <h3>Servidor</h3>
        <div class="form-group">
          <label>Nome do Servidor</label>
          <input type="text" v-model="serverName" placeholder="Meu Servidor RO" />
        </div>
        <div class="form-group">
          <label>URL da Patchlist</label>
          <input type="url" v-model="patchListUrl" placeholder="http://exemplo.com/patchlist.txt" />
        </div>
        <div class="form-group">
          <label>URL de Notícias (opcional)</label>
          <input type="url" v-model="newsUrl" placeholder="http://exemplo.com/news.html" />
        </div>
      </div>

      <!-- Client Settings -->
      <div class="settings-section">
        <h3>Cliente</h3>
        <div class="form-group">
          <label>Executável do Cliente</label>
          <div class="input-with-button">
            <input type="text" v-model="clientExe" placeholder="ragexe.exe" />
            <button @click="selectClientExe">Procurar</button>
          </div>
        </div>
        <div class="form-group">
          <label>Argumentos (opcional)</label>
          <input type="text" v-model="clientArgs" placeholder="1sak1" />
        </div>
        <div class="form-group">
          <label>Arquivos GRF (um por linha)</label>
          <textarea v-model="grfFiles" rows="4" placeholder="data.grf&#10;rdata.grf"></textarea>
        </div>
      </div>

      <!-- Window Settings -->
      <div class="settings-section">
        <h3>Janela</h3>
        <div class="form-row">
          <div class="form-group">
            <label>Largura</label>
            <input type="number" v-model.number="windowWidth" min="400" max="1920" />
          </div>
          <div class="form-group">
            <label>Altura</label>
            <input type="number" v-model.number="windowHeight" min="300" max="1080" />
          </div>
        </div>
        <div class="form-group">
          <label>Ícone do Patcher (.ico)</label>
          <div class="input-with-button">
            <input type="text" v-model="iconPath" placeholder="Nenhum ícone selecionado" readonly />
            <button @click="selectIconFile">Procurar</button>
          </div>
          <small class="help-text">Deixe vazio para usar o ícone padrão</small>
        </div>
      </div>

      <!-- Video Background -->
      <div class="settings-section">
        <h3>🎬 Vídeo de Fundo</h3>
        <div class="form-group">
          <label class="checkbox-label">
            <input type="checkbox" v-model="videoEnabled" />
            <span>Habilitar vídeo de fundo</span>
          </label>
          <small class="help-text">O vídeo será exibido atrás de todos os elementos. A imagem de fundo será usada como fallback.</small>
        </div>
        
        <template v-if="videoEnabled">
          <div class="form-group">
            <label>Arquivo de Vídeo</label>
            <div class="input-with-button">
              <input type="text" v-model="videoPath" placeholder="Nenhum vídeo selecionado" readonly />
              <button @click="selectVideoFile">Procurar</button>
            </div>
            <small class="help-text">Formatos suportados: MP4, WMV, AVI. O vídeo será copiado junto com o patcher.</small>
          </div>
          
          <div class="form-row">
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoLoop" />
                <span>Repetir vídeo</span>
              </label>
            </div>
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoAutoplay" />
                <span>Iniciar automaticamente</span>
              </label>
            </div>
          </div>
          
          <div class="form-row">
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoMuted" />
                <span>Sem som</span>
              </label>
            </div>
            <div class="form-group">
              <label class="checkbox-label">
                <input type="checkbox" v-model="videoShowControls" />
                <span>Mostrar botão Play/Pause</span>
              </label>
            </div>
          </div>
          
          <!-- Control Button Style -->
          <template v-if="videoShowControls">
            <h4 class="subsection-title">Estilo do Botão de Controle</h4>
            
            <div class="form-row">
              <div class="form-group">
                <label>Posição X</label>
                <input type="number" v-model.number="videoBtnX" min="0" />
              </div>
              <div class="form-group">
                <label>Posição Y</label>
                <input type="number" v-model.number="videoBtnY" min="0" />
              </div>
              <div class="form-group">
                <label>Tamanho</label>
                <input type="number" v-model.number="videoBtnSize" min="20" max="200" />
              </div>
            </div>
            
            <div class="form-row">
              <div class="form-group">
                <label>Cor de Fundo</label>
                <div class="color-input">
                  <input type="color" v-model="videoBtnBgColor" />
                  <input type="text" v-model="videoBtnBgColor" />
                </div>
              </div>
              <div class="form-group">
                <label>Cor do Ícone</label>
                <div class="color-input">
                  <input type="color" v-model="videoBtnIconColor" />
                  <input type="text" v-model="videoBtnIconColor" />
                </div>
              </div>
            </div>
            
            <div class="form-row">
              <div class="form-group">
                <label>Cor da Borda</label>
                <div class="color-input">
                  <input type="color" v-model="videoBtnBorderColor" />
                  <input type="text" v-model="videoBtnBorderColor" />
                </div>
              </div>
              <div class="form-group">
                <label>Largura da Borda</label>
                <input type="number" v-model.number="videoBtnBorderWidth" min="0" max="10" />
              </div>
              <div class="form-group">
                <label>Opacidade (%)</label>
                <input type="number" v-model.number="videoBtnOpacity" min="0" max="100" />
              </div>
            </div>
          </template>
        </template>
      </div>

      <!-- Actions -->
      <div class="settings-actions">
        <button class="btn-secondary" @click="saveSettings">
          Salvar Configurações
        </button>
        <button class="btn-primary" @click="handleExport" :disabled="uiStore.isBuilding">
          <span v-if="uiStore.isBuilding">Gerando...</span>
          <span v-else>Exportar Patcher (.exe)</span>
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
