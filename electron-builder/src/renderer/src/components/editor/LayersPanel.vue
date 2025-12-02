<script setup lang="ts">
import { computed } from 'vue'
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'

const projectStore = useProjectStore()
const uiStore = useUiStore()

// Elements sorted by z-index (highest first)
const sortedElements = computed(() => {
  return [...projectStore.project.config.elements].sort((a, b) => (b.zIndex || 0) - (a.zIndex || 0))
})

function selectElement(id: string) {
  projectStore.selectElement(id)
}

function toggleVisibility(id: string) {
  const element = projectStore.project.config.elements.find(e => e.id === id)
  if (element) {
    projectStore.updateElement(id, { visible: element.visible === false ? true : false })
  }
}

function toggleLock(id: string) {
  const element = projectStore.project.config.elements.find(e => e.id === id)
  if (element) {
    projectStore.updateElement(id, { locked: !element.locked })
  }
}

function moveUp(id: string) {
  projectStore.moveLayerUp(id)
}

function moveDown(id: string) {
  projectStore.moveLayerDown(id)
}

function bringToFront(id: string) {
  projectStore.bringToFront(id)
}

function sendToBack(id: string) {
  projectStore.sendToBack(id)
}

function deleteElement(id: string) {
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
    default: return '▢'
  }
}

function getElementName(element: any) {
  return element.name || element.text || `${element.type} ${element.id.slice(-4)}`
}
</script>

<template>
  <div class="layers-panel" v-if="uiStore.isLayersPanelOpen">
    <div class="panel-header">
      <h3>📚 Camadas</h3>
      <button class="close-btn" @click="uiStore.toggleLayersPanel">✕</button>
    </div>
    
    <div class="layers-list">
      <div class="layer-info">
        <small>Arraste para reordenar ou use os controles</small>
      </div>
      
      <div
        v-for="element in sortedElements"
        :key="element.id"
        :class="['layer-item', { 
          selected: element.id === projectStore.selectedElementId,
          hidden: element.visible === false,
          locked: element.locked
        }]"
        @click="selectElement(element.id)"
      >
        <span class="layer-icon">{{ getTypeIcon(element.type) }}</span>
        <span class="layer-name">{{ getElementName(element) }}</span>
        <span class="layer-zindex">z:{{ element.zIndex || 0 }}</span>
        
        <div class="layer-actions">
          <button 
            class="layer-btn" 
            @click.stop="toggleVisibility(element.id)"
            :title="element.visible === false ? 'Mostrar' : 'Ocultar'"
          >
            {{ element.visible === false ? '👁️‍🗨️' : '👁️' }}
          </button>
          <button 
            class="layer-btn" 
            @click.stop="toggleLock(element.id)"
            :title="element.locked ? 'Destravar' : 'Travar'"
          >
            {{ element.locked ? '🔒' : '🔓' }}
          </button>
          <button class="layer-btn" @click.stop="moveUp(element.id)" title="Subir">▲</button>
          <button class="layer-btn" @click.stop="moveDown(element.id)" title="Descer">▼</button>
          <button class="layer-btn danger" @click.stop="deleteElement(element.id)" title="Excluir">🗑️</button>
        </div>
      </div>
      
      <div v-if="sortedElements.length === 0" class="empty-layers">
        <p>Nenhum elemento adicionado</p>
        <small>Use a barra de ferramentas para adicionar elementos</small>
      </div>
    </div>
    
    <!-- Quick actions -->
    <div class="layers-footer" v-if="projectStore.selectedElementId">
      <button @click="bringToFront(projectStore.selectedElementId!)" class="footer-btn">
        ⬆️ Frente
      </button>
      <button @click="sendToBack(projectStore.selectedElementId!)" class="footer-btn">
        ⬇️ Trás
      </button>
    </div>
  </div>
</template>

<style scoped>
.layers-panel {
  position: absolute;
  top: 100px;
  left: 20px;
  width: 280px;
  max-height: 400px;
  background-color: #252526;
  border: 1px solid #3e3e42;
  border-radius: 8px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.4);
  z-index: 1000;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 10px 12px;
  background-color: #2d2d30;
  border-bottom: 1px solid #3e3e42;
}

.panel-header h3 {
  margin: 0;
  font-size: 13px;
  font-weight: 600;
  color: #cccccc;
}

.close-btn {
  background: none;
  border: none;
  color: #9d9d9d;
  cursor: pointer;
  font-size: 14px;
  padding: 2px 6px;
  border-radius: 4px;
}

.close-btn:hover {
  background-color: #3e3e42;
  color: #ffffff;
}

.layers-list {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.layer-info {
  padding: 4px 8px;
  margin-bottom: 8px;
}

.layer-info small {
  color: #6e6e6e;
  font-size: 11px;
}

.layer-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 10px;
  background-color: #2d2d30;
  border: 1px solid transparent;
  border-radius: 4px;
  margin-bottom: 4px;
  cursor: pointer;
  transition: all 0.15s;
}

.layer-item:hover {
  background-color: #3e3e42;
}

.layer-item.selected {
  border-color: #0078d4;
  background-color: rgba(0, 120, 212, 0.2);
}

.layer-item.hidden {
  opacity: 0.5;
}

.layer-item.locked {
  background-color: rgba(255, 193, 7, 0.1);
}

.layer-icon {
  font-size: 14px;
  width: 20px;
  text-align: center;
}

.layer-name {
  flex: 1;
  font-size: 12px;
  color: #cccccc;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.layer-zindex {
  font-size: 10px;
  color: #6e6e6e;
  padding: 2px 4px;
  background-color: #3e3e42;
  border-radius: 3px;
}

.layer-actions {
  display: flex;
  gap: 2px;
  opacity: 0;
  transition: opacity 0.15s;
}

.layer-item:hover .layer-actions {
  opacity: 1;
}

.layer-btn {
  background: none;
  border: none;
  color: #9d9d9d;
  cursor: pointer;
  font-size: 11px;
  padding: 2px 4px;
  border-radius: 3px;
}

.layer-btn:hover {
  background-color: #4e4e52;
  color: #ffffff;
}

.layer-btn.danger:hover {
  background-color: #c42b1c;
}

.empty-layers {
  text-align: center;
  padding: 20px;
  color: #6e6e6e;
}

.empty-layers p {
  margin: 0 0 4px 0;
  font-size: 13px;
}

.empty-layers small {
  font-size: 11px;
}

.layers-footer {
  display: flex;
  gap: 8px;
  padding: 8px 12px;
  background-color: #2d2d30;
  border-top: 1px solid #3e3e42;
}

.footer-btn {
  flex: 1;
  padding: 6px 10px;
  background-color: #0e639c;
  border: none;
  border-radius: 4px;
  color: white;
  cursor: pointer;
  font-size: 11px;
}

.footer-btn:hover {
  background-color: #1177bb;
}
</style>
