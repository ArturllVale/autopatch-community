<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { useProjectStore } from '../../stores/project'
import { useUiStore } from '../../stores/ui'
import Toolbar from '../layout/Toolbar.vue'
import type { UIElement } from '../../types'

const projectStore = useProjectStore()
const uiStore = useUiStore()

const canvasRef = ref<HTMLDivElement | null>(null)
const isDragging = ref(false)
const dragElement = ref<UIElement | null>(null)
const dragStartX = ref(0)
const dragStartY = ref(0)
const elementStartX = ref(0)
const elementStartY = ref(0)

// Progress bar drag state
const isDraggingProgressBar = ref(false)
const progressBarStartX = ref(0)
const progressBarStartY = ref(0)
const progressBarElementStartX = ref(0)
const progressBarElementStartY = ref(0)

// Video control button drag state
const isDraggingVideoBtn = ref(false)
const videoBtnStartX = ref(0)
const videoBtnStartY = ref(0)
const videoBtnElementStartX = ref(0)
const videoBtnElementStartY = ref(0)

// Background image - use a ref to store the data URL
const backgroundDataUrl = ref<string>('')

// Watch for background path changes and load the image
async function loadBackgroundImage(path: string) {
  console.log('[BG DEBUG] loadBackgroundImage called with path:', path)
  if (!path) {
    console.log('[BG DEBUG] Path is empty, clearing background')
    backgroundDataUrl.value = ''
    return
  }
  try {
    console.log('[BG DEBUG] Invoking file:read-binary...')
    // Read the file and convert to base64 data URL
    const data = await window.electron.ipcRenderer.invoke('file:read-binary', path)
    console.log('[BG DEBUG] Received data:', data ? `${data.length} chars` : 'null')
    if (data) {
      const ext = path.split('.').pop()?.toLowerCase() || 'png'
      const mimeType = ext === 'jpg' || ext === 'jpeg' ? 'image/jpeg' : 
                       ext === 'bmp' ? 'image/bmp' : 'image/png'
      backgroundDataUrl.value = `data:${mimeType};base64,${data}`
      console.log('[BG DEBUG] Set backgroundDataUrl, length:', backgroundDataUrl.value.length)
    } else {
      console.log('[BG DEBUG] No data received!')
    }
  } catch (e) {
    console.error('[BG DEBUG] Failed to load background:', e)
    backgroundDataUrl.value = ''
  }
}

// Element image cache for image elements
const elementImageCache = ref<Record<string, string>>({})

// Load image for an element
async function loadElementImage(elementId: string, path: string) {
  if (!path || elementImageCache.value[elementId]) return
  try {
    const data = await window.electron.ipcRenderer.invoke('file:read-binary', path)
    if (data) {
      const ext = path.split('.').pop()?.toLowerCase() || 'png'
      const mimeType = ext === 'jpg' || ext === 'jpeg' ? 'image/jpeg' : 
                       ext === 'bmp' ? 'image/bmp' : 'image/png'
      elementImageCache.value[elementId] = `data:${mimeType};base64,${data}`
    }
  } catch (e) {
    console.error('[IMG] Failed to load element image:', e)
  }
}

// Watch elements for image changes
watch(
  () => projectStore.project.config.elements,
  (elements) => {
    for (const el of elements) {
      if (el.type === 'image' && el.backgroundImage) {
        loadElementImage(el.id, el.backgroundImage)
      }
    }
  },
  { deep: true, immediate: true }
)

// Watch for refresh
watch(
  () => uiStore.refreshCounter,
  () => {
    // Clear image cache and reload
    elementImageCache.value = {}
    if (projectStore.project.config.backgroundImagePath) {
      loadBackgroundImage(projectStore.project.config.backgroundImagePath)
    }
    // Reload element images
    for (const el of projectStore.project.config.elements) {
      if (el.type === 'image' && el.backgroundImage) {
        loadElementImage(el.id, el.backgroundImage)
      }
    }
  }
)

// Watch for changes in background path
watch(
  () => projectStore.project.config.backgroundImagePath,
  (newPath) => {
    console.log('[BG DEBUG] Watch triggered, newPath:', newPath)
    if (newPath) {
      loadBackgroundImage(newPath)
    } else {
      console.log('[BG DEBUG] newPath is falsy, clearing')
      backgroundDataUrl.value = ''
    }
  },
  { immediate: true }
)

// Combined canvas style
const combinedCanvasStyle = computed(() => {
  const borderRadius = projectStore.project.config.windowBorderRadius || 0
  const base = {
    width: `${projectStore.project.config.windowWidth}px`,
    height: `${projectStore.project.config.windowHeight}px`,
    transform: `scale(${uiStore.canvasZoom})`,
    transformOrigin: 'top left',
    borderRadius: borderRadius > 0 ? `${borderRadius}px` : '0',
    overflow: borderRadius > 0 ? 'hidden' : 'visible'
  } as Record<string, string>

  // Background
  if (backgroundDataUrl.value) {
    base.backgroundImage = `url("${backgroundDataUrl.value}")`
    base.backgroundSize = 'cover'
    base.backgroundPosition = 'center'
    console.log('[BG DEBUG] Combined style has backgroundImage set')
  }

  // Grid
  if (uiStore.showGrid) {
    const size = uiStore.gridSize
    const gridBg = `linear-gradient(to right, rgba(255,255,255,0.05) 1px, transparent 1px), linear-gradient(to bottom, rgba(255,255,255,0.05) 1px, transparent 1px)`
    if (base.backgroundImage) {
      base.backgroundImage = base.backgroundImage + ', ' + gridBg
      base.backgroundSize = 'cover, ' + `${size}px ${size}px, ${size}px ${size}px`
    } else {
      base.backgroundImage = gridBg
      base.backgroundSize = `${size}px ${size}px, ${size}px ${size}px`
    }
  }

  console.log('[BG DEBUG] Final combinedCanvasStyle:', Object.keys(base))
  return base
})

function getElementStyle(element: UIElement) {
  const baseStyle: Record<string, string> = {
    left: `${element.x}px`,
    top: `${element.y}px`,
    width: `${element.width}px`,
    height: `${element.height}px`,
    zIndex: `${element.zIndex || 0}`
  }

  // Button specific styles - use state colors
  if (element.type === 'button') {
    const normalState = element.states?.normal
    const bgColor = normalState?.backgroundColor || element.backgroundColor || '#0078d4'
    const textColor = normalState?.fontColor || element.fontColor || '#ffffff'
    const borderColor = normalState?.borderColor || element.borderColor || '#005a9e'
    
    baseStyle.backgroundColor = bgColor
    baseStyle.color = textColor
    baseStyle.borderColor = borderColor
    baseStyle.borderWidth = '2px'
    baseStyle.borderStyle = 'solid'
    baseStyle.borderRadius = '4px'
  }
  // Image element - use cached data URL
  else if (element.type === 'image') {
    const cachedImage = elementImageCache.value[element.id]
    if (cachedImage) {
      baseStyle.backgroundImage = `url("${cachedImage}")`
      baseStyle.backgroundSize = 'cover'
      baseStyle.backgroundPosition = 'center'
      baseStyle.backgroundRepeat = 'no-repeat'
    }
  }
  // Background color or image for other elements
  else if (element.backgroundImage) {
    baseStyle.backgroundSize = 'cover'
    baseStyle.backgroundPosition = 'center'
  } else if (element.backgroundColor) {
    baseStyle.backgroundColor = element.backgroundColor
  }

  // Box style
  if (element.type === 'box' && element.boxStyle) {
    const { fillColor, fillOpacity, borderColor, borderWidth, borderRadius } = element.boxStyle
    baseStyle.backgroundColor = fillColor
    baseStyle.opacity = `${(fillOpacity || 50) / 100}`
    baseStyle.borderColor = borderColor
    baseStyle.borderWidth = `${borderWidth || 1}px`
    baseStyle.borderStyle = 'solid'
    baseStyle.borderRadius = `${borderRadius || 0}px`
  }

  // WebView style
  if (element.type === 'webview' && element.webviewConfig) {
    const { backgroundColor, borderColor, borderWidth, borderRadius } = element.webviewConfig
    baseStyle.backgroundColor = backgroundColor || '#1e1e1e'
    baseStyle.borderColor = borderColor || '#333333'
    baseStyle.borderWidth = `${borderWidth || 1}px`
    baseStyle.borderStyle = 'solid'
    baseStyle.borderRadius = `${borderRadius || 8}px`
  }

  // Text styles
  if (element.fontColor) {
    baseStyle.color = element.fontColor
  }
  if (element.fontName) {
    baseStyle.fontFamily = element.fontName
  }
  if (element.fontSize) {
    baseStyle.fontSize = `${element.fontSize}px`
  }
  if (element.fontBold) {
    baseStyle.fontWeight = 'bold'
  }
  if (element.fontItalic) {
    baseStyle.fontStyle = 'italic'
  }
  
  // Text alignment
  if (element.textAlign) {
    baseStyle.textAlign = element.textAlign
    baseStyle.justifyContent = element.textAlign === 'left' ? 'flex-start' : 
                               element.textAlign === 'right' ? 'flex-end' : 'center'
  }
  if (element.textVerticalAlign) {
    baseStyle.alignItems = element.textVerticalAlign === 'top' ? 'flex-start' : 
                           element.textVerticalAlign === 'bottom' ? 'flex-end' : 'center'
  }

  // Effects
  if (element.effects) {
    const { opacity, borderRadius, shadow, glow } = element.effects
    
    if (opacity !== undefined && element.type !== 'box') {
      baseStyle.opacity = `${opacity / 100}`
    }
    if (borderRadius !== undefined) {
      baseStyle.borderRadius = `${borderRadius}px`
    }
    
    // Shadow and glow
    const shadows: string[] = []
    if (shadow?.enabled) {
      shadows.push(`${shadow.offsetX || 0}px ${shadow.offsetY || 4}px ${shadow.blur || 10}px ${shadow.color || '#000000'}`)
    }
    if (glow?.enabled) {
      shadows.push(`0 0 ${glow.intensity || 10}px ${glow.color || '#00ff00'}`)
    }
    if (shadows.length > 0) {
      baseStyle.boxShadow = shadows.join(', ')
    }
  }

  // Visibility
  if (element.visible === false) {
    baseStyle.opacity = '0.3'
    baseStyle.pointerEvents = 'none'
  }

  return baseStyle
}

function getElementClass(element: UIElement) {
  return {
    'canvas-element': true,
    [`element-${element.type}`]: true,
    selected: element.id === projectStore.selectedElementId,
    locked: element.locked === true,
    hidden: element.visible === false
  }
}

function selectElement(element: UIElement, event: MouseEvent) {
  event.stopPropagation()
  projectStore.selectElement(element.id)
}

function startDrag(element: UIElement, event: MouseEvent) {
  event.preventDefault()
  event.stopPropagation()

  projectStore.selectElement(element.id)
  uiStore.selectProgressBar(false)
  uiStore.selectVideoButton(false)
  
  isDragging.value = true
  dragElement.value = element
  dragStartX.value = event.clientX
  dragStartY.value = event.clientY
  elementStartX.value = element.x
  elementStartY.value = element.y
}

function onMouseMove(event: MouseEvent) {
  // Handle element dragging
  if (isDragging.value && dragElement.value) {
    const dx = (event.clientX - dragStartX.value) / uiStore.canvasZoom
    const dy = (event.clientY - dragStartY.value) / uiStore.canvasZoom

    let newX = elementStartX.value + dx
    let newY = elementStartY.value + dy

    // Snap to grid
    if (uiStore.snapToGrid) {
      newX = Math.round(newX / uiStore.gridSize) * uiStore.gridSize
      newY = Math.round(newY / uiStore.gridSize) * uiStore.gridSize
    }

    // Clamp to canvas bounds
    const maxX = projectStore.project.config.windowWidth - dragElement.value.width
    const maxY = projectStore.project.config.windowHeight - dragElement.value.height
    newX = Math.max(0, Math.min(maxX, newX))
    newY = Math.max(0, Math.min(maxY, newY))

    projectStore.moveElement(dragElement.value.id, newX, newY)
    return
  }

  // Handle progress bar dragging
  if (isDraggingProgressBar.value) {
    const dx = (event.clientX - progressBarStartX.value) / uiStore.canvasZoom
    const dy = (event.clientY - progressBarStartY.value) / uiStore.canvasZoom

    let newX = progressBarElementStartX.value + dx
    let newY = progressBarElementStartY.value + dy

    // Snap to grid
    if (uiStore.snapToGrid) {
      newX = Math.round(newX / uiStore.gridSize) * uiStore.gridSize
      newY = Math.round(newY / uiStore.gridSize) * uiStore.gridSize
    }

    // Clamp to canvas bounds
    const pb = projectStore.project.config.progressBar
    const maxX = projectStore.project.config.windowWidth - pb.width
    const maxY = projectStore.project.config.windowHeight - pb.height
    newX = Math.max(0, Math.min(maxX, newX))
    newY = Math.max(0, Math.min(maxY, newY))

    projectStore.updateProgressBar({ x: newX, y: newY })
  }

  // Handle video button dragging
  if (isDraggingVideoBtn.value) {
    const dx = (event.clientX - videoBtnStartX.value) / uiStore.canvasZoom
    const dy = (event.clientY - videoBtnStartY.value) / uiStore.canvasZoom

    let newX = videoBtnElementStartX.value + dx
    let newY = videoBtnElementStartY.value + dy

    // Snap to grid
    if (uiStore.snapToGrid) {
      newX = Math.round(newX / uiStore.gridSize) * uiStore.gridSize
      newY = Math.round(newY / uiStore.gridSize) * uiStore.gridSize
    }

    // Clamp to canvas bounds
    const btnSize = videoBtnConfig.value.size
    const maxX = projectStore.project.config.windowWidth - btnSize
    const maxY = projectStore.project.config.windowHeight - btnSize
    newX = Math.max(0, Math.min(maxX, newX))
    newY = Math.max(0, Math.min(maxY, newY))

    updateVideoBtnPosition(newX, newY)
  }
}

function onMouseUp() {
  isDragging.value = false
  dragElement.value = null
  isDraggingProgressBar.value = false
  isDraggingVideoBtn.value = false
}

function clearSelection() {
  projectStore.selectElement(null)
  uiStore.selectProgressBar(false)
  uiStore.selectVideoButton(false)
}

// Progress bar functions
function startDragProgressBar(event: MouseEvent) {
  event.preventDefault()
  event.stopPropagation()
  
  // Deselect elements and select progress bar
  projectStore.selectElement(null)
  uiStore.selectProgressBar(true)
  uiStore.selectVideoButton(false)
  
  isDraggingProgressBar.value = true
  progressBarStartX.value = event.clientX
  progressBarStartY.value = event.clientY
  progressBarElementStartX.value = projectStore.project.config.progressBar.x
  progressBarElementStartY.value = projectStore.project.config.progressBar.y
}

function selectProgressBar(event: MouseEvent) {
  event.stopPropagation()
  projectStore.selectElement(null)
  uiStore.selectProgressBar(true)
  uiStore.selectVideoButton(false)
}

// Video button config computed
const videoConfig = computed(() => {
  const vc = projectStore.project.config.videoBackground
  console.log('[VIDEO DEBUG] videoConfig:', vc)
  return vc
})
const videoBtnConfig = computed(() => videoConfig.value?.controlButton || {
  x: 740,
  y: 550,
  size: 50,
  backgroundColor: '#000000',
  iconColor: '#ffffff',
  borderColor: '#ffffff',
  borderWidth: 2,
  opacity: 50
})

// Video button style
const videoBtnStyle = computed(() => {
  const btn = videoBtnConfig.value
  return {
    left: `${btn.x}px`,
    top: `${btn.y}px`,
    width: `${btn.size}px`,
    height: `${btn.size}px`,
    backgroundColor: btn.backgroundColor,
    borderColor: btn.borderColor,
    borderWidth: `${btn.borderWidth}px`,
    opacity: btn.opacity / 100
  }
})

// Start dragging video button
function startDragVideoBtn(event: MouseEvent) {
  event.preventDefault()
  event.stopPropagation()
  
  projectStore.selectElement(null)
  uiStore.selectProgressBar(false)
  uiStore.selectVideoButton(true)
  
  isDraggingVideoBtn.value = true
  videoBtnStartX.value = event.clientX
  videoBtnStartY.value = event.clientY
  videoBtnElementStartX.value = videoBtnConfig.value.x
  videoBtnElementStartY.value = videoBtnConfig.value.y
}

// Select video button
function selectVideoBtn(event: MouseEvent) {
  event.stopPropagation()
  projectStore.selectElement(null)
  uiStore.selectProgressBar(false)
  uiStore.selectVideoButton(true)
}

// Update video button position
function updateVideoBtnPosition(x: number, y: number) {
  projectStore.updateConfig({
    videoBackground: {
      ...videoConfig.value!,
      controlButton: {
        ...videoBtnConfig.value,
        x,
        y
      }
    }
  })
}

function getElementLabel(element: UIElement) {
  const name = element.name || ''
  switch (element.type) {
    case 'button':
      return name || element.text || 'Botão'
    case 'status':
      return '📝 ' + (name || element.text || 'Status')
    case 'percentage':
      return '% ' + (name || element.text || '100%')
    case 'box':
      return '📦 ' + (name || 'Box')
    case 'image':
      return '🖼️ ' + (name || 'Imagem')
    case 'webview':
      return '🌐 ' + (name || element.webviewConfig?.url || 'WebView')
    default:
      return name || element.text || 'Label'
  }
}

// Delete selected element
function deleteSelectedElement() {
  if (projectStore.selectedElementId) {
    projectStore.removeElement(projectStore.selectedElementId)
  }
}

// Handle keyboard events
function handleKeyDown(event: KeyboardEvent) {
  if (event.key === 'Delete' || event.key === 'Backspace') {
    // Don't delete if user is typing in an input
    const target = event.target as HTMLElement
    if (target.tagName === 'INPUT' || target.tagName === 'TEXTAREA') return
    
    if (projectStore.selectedElementId) {
      event.preventDefault()
      deleteSelectedElement()
    }
  }
  // F5 to refresh
  if (event.key === 'F5') {
    event.preventDefault()
    uiStore.refreshAll()
  }
}

// Handle size preset selection
function handleSizePreset(event: Event) {
  const value = (event.target as HTMLSelectElement).value
  if (value) {
    const [width, height] = value.split('x').map(Number)
    projectStore.setWindowSize(width, height)
    uiStore.setStatus(`Tamanho alterado para ${width}×${height}`)
  }
  // Reset select
  (event.target as HTMLSelectElement).value = ''
}

onMounted(() => {
  document.addEventListener('mousemove', onMouseMove)
  document.addEventListener('mouseup', onMouseUp)
  document.addEventListener('keydown', handleKeyDown)
})

onUnmounted(() => {
  document.removeEventListener('mousemove', onMouseMove)
  document.removeEventListener('mouseup', onMouseUp)
  document.removeEventListener('keydown', handleKeyDown)
})
</script>

<template>
  <div class="canvas-wrapper">
    <!-- Toolbar with element tools -->
    <Toolbar />

    <!-- Tabs and Size Controls -->
    <div class="canvas-tabs">
      <span class="tab-title">🎨 Editor de Design</span>
      
      <!-- Patcher Size Controls -->
      <div class="size-controls">
        <span class="size-label">Tamanho:</span>
        <input 
          type="number" 
          class="size-input"
          :value="projectStore.project.config.windowWidth" 
          @input="projectStore.setWindowSize(Number(($event.target as HTMLInputElement).value), projectStore.project.config.windowHeight)"
          min="400" 
          max="1920"
          title="Largura"
        />
        <span class="size-x">×</span>
        <input 
          type="number" 
          class="size-input"
          :value="projectStore.project.config.windowHeight" 
          @input="projectStore.setWindowSize(projectStore.project.config.windowWidth, Number(($event.target as HTMLInputElement).value))"
          min="300" 
          max="1080"
          title="Altura"
        />
        <span class="size-unit">px</span>
        
        <!-- Preset sizes -->
        <select class="size-preset" @change="handleSizePreset($event)" title="Tamanhos predefinidos">
          <option value="">Presets</option>
          <option value="800x600">800×600</option>
          <option value="1024x768">1024×768</option>
          <option value="640x480">640×480</option>
          <option value="600x400">600×400</option>
        </select>
        
        <!-- Border Radius Control -->
        <span class="size-separator">|</span>
        <span class="size-label">Borda:</span>
        <input 
          type="number" 
          class="size-input radius-input"
          :value="projectStore.project.config.windowBorderRadius" 
          @input="projectStore.setWindowBorderRadius(Number(($event.target as HTMLInputElement).value))"
          min="0" 
          max="200"
          title="Arredondamento das bordas (0 = sem arredondamento)"
        />
        <span class="size-unit">px</span>
      </div>
      
      <!-- Delete button (only when there's a selection) -->
      <button 
        v-if="projectStore.selectedElementId"
        class="delete-btn"
        @click="deleteSelectedElement"
        title="Deletar elemento (Delete)"
      >
        🗑️ Deletar
      </button>
    </div>

    <!-- Design View -->
    <div class="canvas-container" @click="clearSelection">
      <div class="canvas-scroll">
        <div
          ref="canvasRef"
          class="design-canvas"
          :style="combinedCanvasStyle"
        >
          <!-- Elements -->
          <div
            v-for="element in projectStore.project.config.elements"
            :key="element.id"
            :class="getElementClass(element)"
            :style="getElementStyle(element)"
            @mousedown="startDrag(element, $event)"
            @click="selectElement(element, $event)"
          >
            <span class="element-label">{{ getElementLabel(element) }}</span>

            <!-- Resize handles (only for selected) -->
            <template v-if="element.id === projectStore.selectedElementId">
              <div class="resize-handle nw"></div>
              <div class="resize-handle ne"></div>
              <div class="resize-handle sw"></div>
              <div class="resize-handle se"></div>
            </template>
          </div>

          <!-- Progress Bar Preview (Draggable) -->
          <div
            :class="['progress-bar-preview', { selected: uiStore.isProgressBarSelected }]"
            :style="{
              left: projectStore.project.config.progressBar.x + 'px',
              top: projectStore.project.config.progressBar.y + 'px',
              width: projectStore.project.config.progressBar.width + 'px',
              height: projectStore.project.config.progressBar.height + 'px',
              backgroundColor: projectStore.project.config.progressBar.backgroundColor,
              borderColor: projectStore.project.config.progressBar.borderColor
            }"
            @mousedown="startDragProgressBar($event)"
            @click="selectProgressBar($event)"
          >
            <div
              class="progress-fill"
              :style="{
                width: '60%',
                backgroundColor: projectStore.project.config.progressBar.fillColor
              }"
            ></div>
            <span class="progress-label">📊 Barra de Progresso</span>
            
            <!-- Resize handles for progress bar (only when selected) -->
            <template v-if="uiStore.isProgressBarSelected">
              <div class="resize-handle nw"></div>
              <div class="resize-handle ne"></div>
              <div class="resize-handle sw"></div>
              <div class="resize-handle se"></div>
            </template>
          </div>

          <!-- Video Control Button Preview (Draggable) -->
          <div
            v-if="videoConfig?.enabled && videoConfig?.showControls"
            :class="['video-control-btn-preview', { selected: uiStore.isVideoButtonSelected }]"
            :style="videoBtnStyle"
            @mousedown="startDragVideoBtn($event)"
            @click="selectVideoBtn($event)"
          >
            <!-- Play/Pause Icon -->
            <svg viewBox="0 0 24 24" class="video-btn-icon" :style="{ fill: videoBtnConfig.iconColor }">
              <!-- Pause icon (two bars) -->
              <rect x="6" y="5" width="4" height="14" />
              <rect x="14" y="5" width="4" height="14" />
            </svg>
            <span class="video-btn-label">▶️ Play/Pause</span>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.canvas-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.canvas-tabs {
  display: flex;
  gap: 4px;
  padding: 8px 12px;
  background-color: #252526;
  border-bottom: 1px solid #3e3e42;
  align-items: center;
}

.tab-title {
  font-size: 14px;
  font-weight: 600;
  color: #cccccc;
}

/* Size controls */
.size-controls {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-left: 20px;
  padding-left: 16px;
  border-left: 1px solid #3e3e42;
}

.size-label {
  font-size: 11px;
  color: #9d9d9d;
}

.size-input {
  width: 60px;
  padding: 4px 6px;
  background-color: #3c3c3c;
  border: 1px solid #555;
  border-radius: 4px;
  color: #cccccc;
  font-size: 12px;
  text-align: center;
}

.size-input:focus {
  outline: none;
  border-color: #0078d4;
}

.size-x {
  font-size: 12px;
  color: #6e6e6e;
}

.size-unit {
  font-size: 11px;
  color: #6e6e6e;
}

.size-separator {
  color: #555;
  margin: 0 8px;
}

.radius-input {
  width: 50px;
}

.size-preset {
  padding: 4px 8px;
  background-color: #3c3c3c;
  border: 1px solid #555;
  border-radius: 4px;
  color: #cccccc;
  font-size: 11px;
  cursor: pointer;
}

.size-preset:focus {
  outline: none;
  border-color: #0078d4;
}

.delete-btn {
  margin-left: auto;
  padding: 6px 12px;
  background-color: #c42b1c;
  border: none;
  border-radius: 4px;
  color: white;
  cursor: pointer;
  font-size: 13px;
  transition: background-color 0.15s;
}

.delete-btn:hover {
  background-color: #e81123;
}

.canvas-container {
  flex: 1;
  background-color: #1e1e1e;
  overflow: auto;
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  padding: 40px;
}

.canvas-scroll {
  display: inline-block;
}

.design-canvas {
  position: relative;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
  border: 1px solid #3e3e42;
  background-color: #2d2d30;
  background-size: cover;
  background-position: center;
  background-repeat: no-repeat;
}

.canvas-element {
  position: absolute;
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: move;
  transition: all 0.15s;
  user-select: none;
  box-sizing: border-box;
}

.canvas-element:hover {
  box-shadow: 0 0 0 2px rgba(0, 120, 212, 0.5);
}

.canvas-element.selected {
  box-shadow: 0 0 0 2px #0078d4;
}

/* Button elements preserve their styled border */
.element-button {
  /* Styles come from getElementStyle */
}

.element-button:hover {
  filter: brightness(1.1);
}

.element-label {
  background-color: transparent;
  color: white;
  border: 1px dashed rgba(255, 255, 255, 0.3);
}

.element-status {
  background-color: rgba(0, 255, 128, 0.1);
  color: #00ff80;
  border: 1px dashed #00ff80;
}

.element-percentage {
  background-color: rgba(255, 200, 0, 0.1);
  color: #ffcc00;
  border: 1px dashed #ffcc00;
}

.element-box {
  background-color: rgba(156, 39, 176, 0.3);
  border: 2px dashed #9c27b0;
}

.element-box.selected {
  border: 2px solid #9c27b0;
}

.element-image {
  background-color: rgba(255, 87, 34, 0.2);
  border: 2px dashed #ff5722;
  background-size: cover;
  background-position: center;
}

.element-image.selected {
  border: 2px solid #ff5722;
}

/* Locked elements */
.canvas-element.locked {
  cursor: not-allowed;
  opacity: 0.7;
}

.canvas-element.locked::after {
  content: '🔒';
  position: absolute;
  top: 2px;
  right: 2px;
  font-size: 10px;
}

/* Hidden elements */
.canvas-element.hidden {
  opacity: 0.3;
  border-style: dotted !important;
}

.canvas-element.hidden::before {
  content: '👁️‍🗨️';
  position: absolute;
  top: 2px;
  left: 2px;
  font-size: 10px;
}

.element-label {
  font-size: 12px;
  pointer-events: none;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
}

/* Resize handles */
.resize-handle {
  position: absolute;
  width: 8px;
  height: 8px;
  background-color: #0078d4;
  border: 1px solid white;
}

.resize-handle.nw { top: -4px; left: -4px; cursor: nw-resize; }
.resize-handle.ne { top: -4px; right: -4px; cursor: ne-resize; }
.resize-handle.sw { bottom: -4px; left: -4px; cursor: sw-resize; }
.resize-handle.se { bottom: -4px; right: -4px; cursor: se-resize; }

/* Progress bar preview */
.progress-bar-preview {
  position: absolute;
  border: 2px solid transparent;
  border-radius: 2px;
  overflow: visible;
  cursor: move;
  transition: border-color 0.15s;
  user-select: none;
}

.progress-bar-preview:hover {
  border-color: rgba(0, 200, 83, 0.5);
}

.progress-bar-preview.selected {
  border-color: #00c853;
}

.progress-fill {
  height: 100%;
  transition: width 0.3s;
}

.progress-label {
  position: absolute;
  inset: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 10px;
  color: white;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
  pointer-events: none;
}

/* Video control button preview */
.video-control-btn-preview {
  position: absolute;
  border-radius: 50%;
  border-style: solid;
  cursor: move;
  transition: border-color 0.15s, box-shadow 0.15s;
  user-select: none;
  display: flex;
  align-items: center;
  justify-content: center;
  box-sizing: border-box;
  z-index: 9999;
}

.video-control-btn-preview:hover {
  box-shadow: 0 0 8px rgba(255, 165, 0, 0.6);
}

.video-control-btn-preview.selected {
  box-shadow: 0 0 0 2px #ff9800, 0 0 12px rgba(255, 165, 0, 0.8);
}

.video-btn-icon {
  width: 50%;
  height: 50%;
  pointer-events: none;
}

.video-btn-label {
  position: absolute;
  bottom: -20px;
  left: 50%;
  transform: translateX(-50%);
  white-space: nowrap;
  font-size: 10px;
  color: white;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.8);
  pointer-events: none;
  background: rgba(0, 0, 0, 0.6);
  padding: 2px 6px;
  border-radius: 4px;
}
</style>
