<script setup lang="ts">
import { useI18n } from 'vue-i18n'
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'
import LanguageSelector from '../common/LanguageSelector.vue'

const { t } = useI18n()
const projectStore = useProjectStore()
const uiStore = useUiStore()



// Função desativada temporariamente - modo HTML
// function selectHtmlMode() {
//   projectStore.updateConfig({ uiMode: 'html' })
//   uiStore.setActiveTab('html')
// }

async function handleOpenProject() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: t('sidebar.openProject'),
      filters: [{ name: t('sidebar.projectFilter'), extensions: ['approject', 'json'] }]
    })
    if (result) {
      const content = await window.electron.ipcRenderer.invoke('file:read', result)
      projectStore.fromJSON(content)
      projectStore.setProjectPath(result)
      uiStore.setStatus(t('status.projectLoaded', { path: result }))
    }
  } catch (e) {
    uiStore.setStatus(t('status.projectLoadError'))
    console.error(e)
  }
}

async function handleSaveProject() {
  try {
    let path = projectStore.project.path
    if (!path) {
      path = await window.electron.ipcRenderer.invoke('dialog:save-file', {
        title: t('sidebar.saveProject'),
        filters: [{ name: t('sidebar.projectFilter'), extensions: ['approject'] }],
        defaultPath: 'meu-patcher.approject'
      })
    }
    if (path) {
      await window.electron.ipcRenderer.invoke('file:write', {
        path,
        content: projectStore.toJSON()
      })
      projectStore.setProjectPath(path)
      projectStore.markSaved()
      uiStore.setStatus(t('status.projectSaved', { path }))
    }
  } catch (e) {
    uiStore.setStatus(t('status.projectSaveError'))
    console.error(e)
  }
}
</script>

<template>
  <div class="sidebar">
    <div class="sidebar-actions">
      <button class="action-btn" @click="handleOpenProject">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M20 6h-8l-2-2H4c-1.1 0-2 .9-2 2v12c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2zm0 12H4V6h5.17l2 2H20v10z"/></svg>
        {{ t('sidebar.openProject') }}
      </button>
      <button class="action-btn" @click="handleSaveProject">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M17 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V7l-4-4zm2 16H5V5h11.17L19 7.83V19zm-7-7c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3zM6 6h9v4H6z"/></svg>
        {{ t('sidebar.saveProject') }}
      </button>
    </div>

    <!-- Server Config Section -->
    <div class="sidebar-section">
      <div class="section-title">{{ t('sidebar.server') }}</div>

      <div class="config-field">
        <label>{{ t('sidebar.serverName') }}:</label>
        <input
          type="text"
          :value="projectStore.project.config.serverName"
          @input="projectStore.updateConfig({ serverName: ($event.target as HTMLInputElement).value })"
          :placeholder="t('sidebar.serverNamePlaceholder')"
        />
      </div>

      <div class="config-field">
        <label>{{ t('sidebar.patchlistUrl') }}:</label>
        <input
          type="text"
          :value="projectStore.project.config.patchListUrl"
          @input="projectStore.updateConfig({ patchListUrl: ($event.target as HTMLInputElement).value })"
          placeholder="https://yourserver.com/patchlist.txt"
        />
      </div>

      <div class="config-field">
        <label>{{ t('sidebar.patchlistFile') }}:</label>
        <input
          type="text"
          value="patchlist.txt"
          disabled
          placeholder="patchlist.txt"
        />
      </div>

      <div class="config-field">
        <label>{{ t('sidebar.patchesFolder') }}:</label>
        <input
          type="text"
          value="patches/"
          disabled
          placeholder="patches/"
        />
      </div>
    </div>

    <!-- Game Config Section -->
    <div class="sidebar-section">
      <div class="section-title">{{ t('sidebar.gameConfig') }}</div>

      <div class="config-field">
        <label>{{ t('sidebar.mainGrf') }}:</label>
        <input
          type="text"
          :value="projectStore.project.config.grfFiles[0] || 'data.grf'"
          @input="projectStore.updateConfig({ grfFiles: [($event.target as HTMLInputElement).value] })"
          placeholder="data.grf"
        />
      </div>

      <div class="config-field">
        <label>{{ t('sidebar.gameExe') }}:</label>
        <input
          type="text"
          :value="projectStore.project.config.clientExe"
          @input="projectStore.updateConfig({ clientExe: ($event.target as HTMLInputElement).value })"
          placeholder="ragexe.exe"
        />
      </div>

      <div class="config-checkbox">
        <input type="checkbox" id="closeOnStart" checked />
        <label for="closeOnStart">{{ t('sidebar.closeOnStart') }}</label>
      </div>
    </div>

    <!-- Language Selector Footer -->
    <div class="sidebar-footer">
      <LanguageSelector />
    </div>
  </div>
</template>

<style scoped>
.sidebar {
  width: 240px;
  background-color: #1e1e1e;
  border-right: 1px solid #3e3e42;
  display: flex;
  flex-direction: column;
  flex-shrink: 0;
  overflow-y: auto;
}

.sidebar-header {
  padding: 20px 16px;
  border-bottom: 1px solid #3e3e42;
}

.logo-text {
  font-size: 18px;
  font-weight: 600;
  color: #ffffff;
  margin: 0 0 4px 0;
}

.logo-subtitle {
  font-size: 11px;
  color: #6e6e6e;
  margin: 0;
}

.sidebar-actions {
  display: flex;
  gap: 8px;
  padding: 15px 16px;
  border-bottom: 1px solid #3e3e42;
}

.action-btn {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 8px;
  background: #2d2d30;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #cccccc;
  font-size: 11px;
  cursor: pointer;
  transition: all 0.15s;
}

.action-btn:hover {
  background: #3e3e42;
  color: #ffffff;
}

.action-btn svg {
  width: 14px;
  height: 14px;
}

.sidebar-section {
  padding: 16px;
  border-bottom: 1px solid #3e3e42;
}

.section-title {
  font-size: 11px;
  font-weight: 600;
  color: #0078d4;
  letter-spacing: 0.5px;
  margin-bottom: 12px;
}

.mode-card {
  display: flex;
  gap: 12px;
  padding: 12px;
  background: #2d2d30;
  border: 1px solid #3e3e42;
  border-radius: 6px;
  cursor: pointer;
  transition: all 0.15s;
  margin-bottom: 8px;
}

.mode-card:hover {
  border-color: #4e4e52;
}

.mode-card.active {
  background: #0e639c;
  border-color: #1177bb;
}

.mode-icon {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
  flex-shrink: 0;
}

.mode-icon svg {
  width: 18px;
  height: 18px;
  color: #cccccc;
}

.mode-card.active .mode-icon svg {
  color: #ffffff;
}

.mode-info {
  flex: 1;
  min-width: 0;
}

.mode-title {
  font-size: 13px;
  font-weight: 500;
  color: #ffffff;
  margin-bottom: 2px;
}

.mode-desc {
  font-size: 11px;
  color: #9d9d9d;
  line-height: 1.3;
}

.mode-card.active .mode-desc {
  color: rgba(255, 255, 255, 0.7);
}

.config-field {
  margin-bottom: 12px;
}

.config-field label {
  display: block;
  font-size: 11px;
  color: #9d9d9d;
  margin-bottom: 4px;
}

.config-field input {
  width: 100%;
  padding: 8px 10px;
  background: #2d2d30;
  border: 1px solid #3e3e42;
  border-radius: 4px;
  color: #cccccc;
  font-size: 12px;
}

.config-field input:focus {
  outline: none;
  border-color: #0078d4;
}

.config-field input:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.config-checkbox {
  display: flex;
  align-items: center;
  gap: 8px;
}

.config-checkbox input {
  width: 14px;
  height: 14px;
  accent-color: #0078d4;
}

.config-checkbox label {
  font-size: 12px;
  color: #cccccc;
}

.sidebar-footer {
  margin-top: auto;
  padding: 12px 16px;
  border-top: 1px solid #3e3e42;
}
</style>
