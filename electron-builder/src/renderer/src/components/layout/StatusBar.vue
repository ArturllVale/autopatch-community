<script setup lang="ts">
import { useUiStore } from '../../stores/ui'

const uiStore = useUiStore()

function handlePreview() {
  // TODO: Open preview window
  uiStore.setStatus('Pré-visualização em desenvolvimento...')
}

function handleExport() {
  uiStore.setActiveTab('settings')
}
</script>

<template>
  <div class="status-bar">
    <!-- Left: Status message -->
    <div class="status-left">
      <span class="status-icon">✓</span>
      <span v-if="uiStore.statusMessage" class="status-message">
        {{ uiStore.statusMessage }}
      </span>
      <span v-else class="status-message">Pronto para configurar</span>
    </div>

    <!-- Center: Build progress -->
    <div v-if="uiStore.isBuilding" class="status-center">
      <div class="build-progress">
        <div class="progress-bar" :style="{ width: uiStore.buildProgress + '%' }"></div>
      </div>
      <span class="progress-text">{{ uiStore.buildProgress }}%</span>
    </div>

    <!-- Right: Action buttons -->
    <div class="status-right">
      <button class="status-btn preview-btn" @click="handlePreview">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/></svg>
        Pré-visualizar
      </button>
      <button class="status-btn export-btn" @click="handleExport">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M19.35 10.04C18.67 6.59 15.64 4 12 4 9.11 4 6.6 5.64 5.35 8.04 2.34 8.36 0 10.91 0 14c0 3.31 2.69 6 6 6h13c2.76 0 5-2.24 5-5 0-2.64-2.05-4.78-4.65-4.96zM14 13v4h-4v-4H7l5-5 5 5h-3z"/></svg>
        Gerar Patcher EXE
      </button>
    </div>
  </div>
</template>

<style scoped>
.status-bar {
  height: 48px;
  background-color: #1e1e1e;
  border-top: 1px solid #3e3e42;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 16px;
  font-size: 12px;
  color: #9d9d9d;
  flex-shrink: 0;
}

.status-left,
.status-center,
.status-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-icon {
  color: #4ec9b0;
}

.status-message {
  max-width: 400px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.build-progress {
  width: 120px;
  height: 10px;
  background-color: rgba(255, 255, 255, 0.1);
  border-radius: 5px;
  overflow: hidden;
}

.progress-bar {
  height: 100%;
  background-color: #4ec9b0;
  transition: width 0.2s;
}

.progress-text {
  font-size: 11px;
  min-width: 35px;
}

.status-right {
  gap: 12px;
}

.status-btn {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  font-size: 13px;
  cursor: pointer;
  transition: all 0.15s;
}

.status-btn svg {
  width: 16px;
  height: 16px;
}

.preview-btn {
  background: #2d2d30;
  border: 1px solid #3e3e42;
  color: #cccccc;
}

.preview-btn:hover {
  background: #3e3e42;
  color: #ffffff;
}

.export-btn {
  background: #0e639c;
  color: #ffffff;
}

.export-btn:hover {
  background: #1177bb;
}
</style>
