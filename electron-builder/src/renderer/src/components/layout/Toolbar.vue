<script setup lang="ts">
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'
import type { ElementType } from '../../types'

const projectStore = useProjectStore()
const uiStore = useUiStore()

function addElement(type: ElementType) {
  projectStore.addElement(type)
  uiStore.setStatus(`${type} adicionado`)
}

function deleteSelected() {
  if (projectStore.selectedElementId) {
    projectStore.removeElement(projectStore.selectedElementId)
    uiStore.setStatus('Elemento removido')
  }
}

async function selectBackground() {
  try {
    const result = await window.electron.ipcRenderer.invoke('dialog:open-file', {
      title: 'Selecionar Imagem de Fundo',
      filters: [{ name: 'Imagens', extensions: ['png', 'jpg', 'jpeg', 'bmp'] }]
    })
    if (result) {
      projectStore.setBackgroundImage(result)
      uiStore.setStatus(`Background: ${result}`)
    }
  } catch (e) {
    console.error(e)
  }
}

function refreshAll() {
  uiStore.refreshAll()
}

// Layer controls
function bringToFront() {
  if (projectStore.selectedElementId) {
    projectStore.bringToFront(projectStore.selectedElementId)
    uiStore.setStatus('Elemento trazido para frente')
  }
}

function sendToBack() {
  if (projectStore.selectedElementId) {
    projectStore.sendToBack(projectStore.selectedElementId)
    uiStore.setStatus('Elemento enviado para trás')
  }
}
</script>

<template>
  <div class="toolbar">
    <!-- Left: Element Tools -->
    <div class="toolbar-group">
      <span class="group-label">Adicionar:</span>
      <button class="tool-btn" @click="addElement('button')" title="Adicionar Botão">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M19 6H5c-1.1 0-2 .9-2 2v8c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V8c0-1.1-.9-2-2-2zm0 10H5V8h14v8z"/></svg>
        Botão
      </button>
      <button class="tool-btn" @click="addElement('label')" title="Adicionar Label">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M2.5 4v3h5v12h3V7h5V4h-13zm19 5h-9v3h3v7h3v-7h3V9z"/></svg>
        Label
      </button>
      <button class="tool-btn" @click="addElement('box')" title="Adicionar Box Translúcida">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M19 3H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2V5c0-1.1-.9-2-2-2zm0 16H5V5h14v14z"/><path d="M7 7h10v10H7z" fill-opacity="0.3"/></svg>
        Box
      </button>
      <button class="tool-btn" @click="addElement('image')" title="Adicionar Imagem">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z"/></svg>
        Imagem
      </button>
      <button class="tool-btn" @click="addElement('webview')" title="Adicionar WebView (iframe para conteúdo externo)">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm-1 17.93c-3.95-.49-7-3.85-7-7.93 0-.62.08-1.21.21-1.79L9 15v1c0 1.1.9 2 2 2v1.93zm6.9-2.54c-.26-.81-1-1.39-1.9-1.39h-1v-3c0-.55-.45-1-1-1H8v-2h2c.55 0 1-.45 1-1V7h2c1.1 0 2-.9 2-2v-.41c2.93 1.19 5 4.06 5 7.41 0 2.08-.8 3.97-2.1 5.39z"/></svg>
        WebView
      </button>
      <div class="separator"></div>
      <button class="tool-btn" @click="addElement('status')" title="Adicionar Status">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M20 2H4c-1.1 0-2 .9-2 2v18l4-4h14c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zm0 14H6l-2 2V4h16v12z"/></svg>
        Status
      </button>
      <button class="tool-btn" @click="addElement('percentage')" title="Adicionar Porcentagem">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M7.5 11C9.43 11 11 9.43 11 7.5S9.43 4 7.5 4 4 5.57 4 7.5 5.57 11 7.5 11zm0-5C8.33 6 9 6.67 9 7.5S8.33 9 7.5 9 6 8.33 6 7.5 6.67 6 7.5 6zM4.81 19h2.13l12-12h-2.13l-12 12zm11.69 1c1.93 0 3.5-1.57 3.5-3.5S18.43 13 16.5 13 13 14.57 13 16.5s1.57 3.5 3.5 3.5zm0-5c.83 0 1.5.67 1.5 1.5s-.67 1.5-1.5 1.5-1.5-.67-1.5-1.5.67-1.5 1.5-1.5z"/></svg>
        %
      </button>
      <div class="separator"></div>
      <button class="tool-btn" @click="selectBackground" title="Selecionar Background">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M21 19V5c0-1.1-.9-2-2-2H5c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h14c1.1 0 2-.9 2-2zM8.5 13.5l2.5 3.01L14.5 12l4.5 6H5l3.5-4.5z"/></svg>
        Background
      </button>
    </div>

    <!-- Center: Canvas Tools -->
    <div class="toolbar-group">
      <button
        class="tool-btn icon-only"
        :class="{ active: uiStore.showGrid }"
        @click="uiStore.toggleGrid"
        title="Mostrar Grid"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M20 2H4c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h16c1.1 0 2-.9 2-2V4c0-1.1-.9-2-2-2zM8 20H4v-4h4v4zm0-6H4v-4h4v4zm0-6H4V4h4v4zm6 12h-4v-4h4v4zm0-6h-4v-4h4v4zm0-6h-4V4h4v4zm6 12h-4v-4h4v4zm0-6h-4v-4h4v4zm0-6h-4V4h4v4z"/></svg>
      </button>
      <button
        class="tool-btn icon-only"
        :class="{ active: uiStore.snapToGrid }"
        @click="uiStore.toggleSnap"
        title="Snap to Grid"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M20 4v2h-8V4h8zm-8 4h8v2h-8V8zM4 8h4v6H4V8zm8 6h8v2h-8v-2zm0 4h8v2h-8v-2zM4 16h4v4H4v-4zM4 4h4v2H4V4z"/></svg>
      </button>
      <div class="separator"></div>
      <button class="tool-btn icon-only" @click="uiStore.zoomOut" title="Zoom Out">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14zM7 9h5v1H7z"/></svg>
      </button>
      <span class="zoom-label">{{ Math.round(uiStore.canvasZoom * 100) }}%</span>
      <button class="tool-btn icon-only" @click="uiStore.zoomIn" title="Zoom In">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M15.5 14h-.79l-.28-.27C15.41 12.59 16 11.11 16 9.5 16 5.91 13.09 3 9.5 3S3 5.91 3 9.5 5.91 16 9.5 16c1.61 0 3.09-.59 4.23-1.57l.27.28v.79l5 4.99L20.49 19l-4.99-5zm-6 0C7.01 14 5 11.99 5 9.5S7.01 5 9.5 5 14 7.01 14 9.5 11.99 14 9.5 14zm.5-7H9v2H7v1h2v2h1v-2h2V9h-2z"/></svg>
      </button>
      <button class="tool-btn icon-only" @click="uiStore.resetZoom" title="Reset Zoom">
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M12 5V1L7 6l5 5V7c3.31 0 6 2.69 6 6s-2.69 6-6 6-6-2.69-6-6H4c0 4.42 3.58 8 8 8s8-3.58 8-8-3.58-8-8-8z"/></svg>
      </button>
    </div>

    <!-- Right: Edit Tools -->
    <div class="toolbar-group">
      <!-- Layer controls -->
      <button
        class="tool-btn icon-only"
        @click="bringToFront"
        :disabled="!projectStore.selectedElementId"
        title="Trazer para Frente"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M3 3h8v8H3V3zm2 2v4h4V5H5zm8-2h8v8h-8V3zm2 2v4h4V5h-4zM3 13h8v8H3v-8zm2 2v4h4v-4H5zm8-2h8v8h-8v-8zm2 2v4h4v-4h-4z"/><path d="M9 9l6 6" stroke="currentColor" stroke-width="2"/></svg>
      </button>
      <button
        class="tool-btn icon-only"
        @click="sendToBack"
        :disabled="!projectStore.selectedElementId"
        title="Enviar para Trás"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M3 3h8v8H3V3zm2 2v4h4V5H5zm8-2h8v8h-8V3zm2 2v4h4V5h-4zM3 13h8v8H3v-8zm2 2v4h4v-4H5zm8-2h8v8h-8v-8zm2 2v4h4v-4h-4z" opacity="0.5"/><path d="M15 15l-6-6" stroke="currentColor" stroke-width="2"/></svg>
      </button>
      <div class="separator"></div>
      <!-- Refresh button -->
      <button
        class="tool-btn"
        @click="refreshAll"
        title="Recarregar Tudo (F5)"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M17.65 6.35C16.2 4.9 14.21 4 12 4c-4.42 0-7.99 3.58-7.99 8s3.57 8 7.99 8c3.73 0 6.84-2.55 7.73-6h-2.08c-.82 2.33-3.04 4-5.65 4-3.31 0-6-2.69-6-6s2.69-6 6-6c1.66 0 3.14.69 4.22 1.78L13 11h7V4l-2.35 2.35z"/></svg>
        Refresh
      </button>
      <div class="separator"></div>
      <button
        class="tool-btn danger"
        @click="deleteSelected"
        :disabled="!projectStore.selectedElementId"
        title="Deletar Selecionado"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>
        Deletar
      </button>
      <button
        class="tool-btn"
        @click="uiStore.togglePropertiesPanel"
        :class="{ active: uiStore.isPropertiesPanelOpen }"
        title="Painel de Propriedades"
      >
        <svg viewBox="0 0 24 24" fill="currentColor"><path d="M3 17v2h6v-2H3zM3 5v2h10V5H3zm10 16v-2h8v-2h-8v-2h-2v6h2zM7 9v2H3v2h4v2h2V9H7zm14 4v-2H11v2h10zm-6-4h2V7h4V5h-4V3h-2v6z"/></svg>
        Propriedades
      </button>
    </div>
  </div>
</template>

<style scoped>
.toolbar {
  height: 48px;
  background-color: #333333;
  border-bottom: 1px solid #3e3e42;
  display: flex;
  align-items: center;
  padding: 0 12px;
  gap: 8px;
  flex-shrink: 0;
}

.toolbar-group {
  display: flex;
  align-items: center;
  gap: 4px;
}

.toolbar-group:not(:last-child)::after {
  content: '';
  width: 1px;
  height: 24px;
  background-color: #3e3e42;
  margin-left: 8px;
}

.group-label {
  font-size: 11px;
  color: #9d9d9d;
  margin-right: 4px;
}

.tool-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 6px 10px;
  background: transparent;
  border: none;
  border-radius: 4px;
  color: #cccccc;
  font-size: 12px;
  cursor: pointer;
  transition: background-color 0.15s;
}

.tool-btn:hover:not(:disabled) {
  background-color: #3e3e42;
}

.tool-btn.active {
  background-color: #0e639c;
  color: white;
}

.tool-btn:disabled {
  opacity: 0.4;
  cursor: not-allowed;
}

.tool-btn.icon-only {
  padding: 6px;
}

.tool-btn svg {
  width: 18px;
  height: 18px;
  flex-shrink: 0;
}

.tool-btn.danger:hover:not(:disabled) {
  background-color: #c42b1c;
  color: white;
}

.separator {
  width: 1px;
  height: 20px;
  background-color: #3e3e42;
  margin: 0 4px;
}

.zoom-label {
  font-size: 11px;
  color: #9d9d9d;
  min-width: 40px;
  text-align: center;
}
</style>
